;FAT 32 driver for SD cards

 cblock
;24 bit variables (the upper byte is not needed, as we use low capacity cards
fat_begin_lba:3
cluster_begin_lba:3
sectors_per_fat:3
sectors_per_cluster
sectors_per_cluster_power
root_dir_first_cluster:3

;The file/folder name, used for opening files/folders etc.
entryname:.11
;The first cluster byte address for the entry
first_byte_addr:3
;Save point for SD_addr during deletes etc.
FAT_temp_addr:3
FAT_temp_addr2:3
FAT_temp_addr3:3
FAT_temp_var
FAT_temp_var2
FAT_newfile_size
FAT_newfile_attrib

;What the current cluster is (the attribute byte)
currentEntryState
 endc

 #define FAT_is_ARCHIVE currentEntryState,5
 #define FAT_is_DIR currentEntryState,4
 #define FAT_is_VOLUMELABEL currentEntryState,3
 #define FAT_is_SYSTEM currentEntryState,2
 #define FAT_is_HIDDEN currentEntryState,1
 #define FAT_is_RO currentEntryState,0

;////////////////////////////////////////////////////////
;Helper macros and functions
x2_24bit macro in
	bcf STATUS,C
	rlcf in+0,F
	rlcf in+1,F
	rlcf in+2,F
	endm

div2_24bit macro in
	bcf STATUS,C
	rrcf in+2,F
	rrcf in+1,F
	rrcf in+0,F
	endm

add_24bit macro a,b
	movf b+0,W
	addwf a+0,F
	movf b+1,W
	addwfc a+1,F
	movf b+2,W
	addwfc a+2,F
	endm

sub_24bit macro a,b
	movf b+0,W
	subwf a+0,F
	movf b+1,W
	subwfb a+1,F
	movf b+2,W
	subwfb a+2,F
	endm

sub_w_24bit macro a
	subwf a+0,F
	movlw .0
	subwfb a+1,F
	movlw .0
	subwfb a+2,F
	endm

add_w_24bit macro a
	addwf a+0,F
	movlw .0
	addwfc a+1,F
	movlw .0
	addwfc a+2,F
	endm

cp_24bit macro a,b
	movff a+0,b+0
	movff a+1,b+1
	movff a+2,b+2
	endm

swap_24bit macro a,b
	movf a+0,W
	movff b+0,a+0
	movwf b+0

	movf a+1,W
	movff b+1,a+1
	movwf b+1

	movf a+2,W
	movff b+2,a+1
	movwf b+2
	endm

cp_FSR_24bit macro a,b
	movff POSTINC#v(a),b+0
	movff POSTINC#v(a),b+1
	movff POSTINC#v(a),b+2
	endm

;////////////////////////////////////////////////////////
;This function is used when you load a cluster address and want a
;data address
convert_cluster_in_addr_no_offset
	;first subtract 2, as all cluster addresses start at 2
	movlw .2
	sub_w_24bit SD_addr
	
	;Now we multiply by the sectors per cluster
	movf sectors_per_cluster_power,W
	bz convert_cluster_in_addr_skip

	movwf PRODH	
convert_cluster_in_addr_loop
	x2_24bit SD_addr
	decfsz PRODH
	bra convert_cluster_in_addr_loop

convert_cluster_in_addr_skip
	return

convert_cluster_in_addr
	call convert_cluster_in_addr_no_offset
	;Finally we add the cluster_begin_lba
	add_24bit SD_addr, cluster_begin_lba
	;Multiply by two again for sectors to bytes
	x2_24bit SD_addr
	;We're done
	return
;//////////////////////////////////////////////////////////////
;this converts the byte address back to a cluster number
convert_byte_in_addr
	;Divide by two, to get the sector address
	div2_24bit SD_addr

	;subtract the cluster offset
	sub_24bit SD_addr, cluster_begin_lba

	;Now we divide by the sectors per cluster
	movf sectors_per_cluster_power,W
	bz convert_cluster_in_addr_skip

	movwf PRODH	
convert_byte_in_addr_loop
	div2_24bit SD_addr
	decfsz PRODH
	bra convert_byte_in_addr_loop

convert_byte_in_addr_skip
	;finally add 2, as all cluster addresses start at 2
	movlw .2
	add_w_24bit SD_addr
	return

;////////////////////////////////////////////////////////
;Setup a FAT partition
FAT_init
	call FAT_load_MBR
	xorlw .0
	bnz FAT_init_fail

	call FAT_parse_MBR
	xorlw .0
	bnz FAT_init_fail

	call FAT_parse_VolID
	xorlw .0
	bnz FAT_init_fail

FAT_init_fail
	return

;////////////////////////////////////////////////////////
;Load the MBR
FAT_load_MBR
	clrf SD_addr+2
	clrf SD_addr+1
	clrf SD_addr

	call SD_read
	xorlw .0
	bnz FAT_failed_to_read_MBR
	
	;Load the buffer
	movlw 0x09
	movwf FSR2H
	movlw 0xFE
	movwf FSR2L

	;Check for the signature
	movlw 0x55	
	xorwf POSTINC2,W
	bnz FAT_failed_MBR
	movlw 0xAA
	xorwf POSTINC2,W
	bnz FAT_failed_MBR
	retlw .0

;Error Messages
FAT_failed_to_read_MBR_text db "Failed to read MBR",0x00
FAT_failed_to_read_MBR
	LoadTable FAT_failed_to_read_MBR_text
	call blit_print_table_string
	retlw .1

FAT_failed_MBR_text db "MBR is corrupt",0x00
FAT_failed_MBR
	LoadTable FAT_failed_MBR_text
	call blit_print_table_string
	retlw .1


;////////////////////////////////////////////////////////
;Parse the MBR
;Partion entries are at 0x9[BE,CE,DE,EE] (16 bytes)
;Type is byte 0x04, can be 0x0B or 0x0C
FAT_parse_MBR
	;Load the first partition entry
	movlw 0x09
	movwf FSR2H
	movlw 0xC2
	movwf FSR2L

FAT_parse_MBR_loop
	movf INDF2,W	
	xorlw 0x0B
	bz FAT_parse_MBR_found_partition

	movf INDF2,W	
	xorlw 0x0C
	bz FAT_parse_MBR_found_partition

	movlw .16
	addwf FSR2L,W
	;Check we've not tested all the partitions
	bc FAT_parse_MBR_noFAT
	movwf FSR2L
	bra FAT_parse_MBR_loop

FAT_parse_MBR_found_partition
	movlw .4
	addwf FSR2L,F

	cp_FSR_24bit 2,fat_begin_lba
	cp_24bit fat_begin_lba,SD_addr
	x2_24bit SD_addr

	call SD_read
	xorlw .0
	bnz FAT_parse_MBR_noVolID
	retlw .0

;Error Messages
FAT_parse_MBR_noFAT_txt db "No FAT Partition",0x00
FAT_parse_MBR_noFAT
	LoadTable FAT_parse_MBR_noFAT_txt
	call blit_print_table_string
	retlw .1

FAT_parse_MBR_noVolID_txt db "Failed Reading VolID",0x00
FAT_parse_MBR_noVolID
	LoadTable FAT_parse_MBR_noVolID_txt
	call blit_print_table_string
	retlw .1


;////////////////////////////////////////////////////////
;Parse the Volume ID
;Checks for 0x0002 at 0x080B, and 0x02 at 0x0810 (2 FATs),
;Then loads the data
;Then checks for 55AA at the end of the block
FAT_parse_VolID
	;Load the first partition entry
	movlw 0x08
	movwf FSR2H
	movlw 0x0B
	movwf FSR2L

	movf POSTINC2,W
	xorlw 0x00
	bnz FAT_parse_VolID_corrupt
	movf POSTINC2,W
	xorlw 0x02
	bnz FAT_parse_VolID_corrupt

	;Zero the power
	clrf sectors_per_cluster_power
	;Sectors per cluster is loaded
	movf POSTINC2,W
	movwf sectors_per_cluster

	;Now we calculate which power of two it is
FAT_parse_VolID_power_loop
	rrcf WREG 
	btfsc STATUS,C
	bra FAT_parse_VolID_power_loop_cont

	incf sectors_per_cluster_power,F
	bra FAT_parse_VolID_power_loop

FAT_parse_VolID_power_loop_cont

	;Now load the number of reserved sectors and add it to the fat_begin_lba
	movf POSTINC2,W
	addwf fat_begin_lba+0,f
	movf POSTINC2,W
	addwfc fat_begin_lba+1,f
	movlw .0
	addwfc fat_begin_lba+2,f

	;Ensure there are only 2 FATS
	movf POSTINC2,W
	xorlw 0x02
	bnz FAT_parse_VolID_corrupt

	;Now load the number of sectors per fat
	movlw 0x24
	movwf FSR2L
	cp_FSR_24bit 2,sectors_per_fat

	;Make this into the cluster lba
	cp_24bit sectors_per_fat,cluster_begin_lba

	;Multiply by two
	x2_24bit cluster_begin_lba
	;Then add fat_begin_lba
	add_24bit cluster_begin_lba, fat_begin_lba
	
	;Now load the Root directory first cluster
	movlw 0x2C
	movwf FSR2L
	cp_FSR_24bit 2,root_dir_first_cluster

	;Check for the signature
	movlw 0x09
	movwf FSR2H
	movlw 0xFE
	movwf FSR2L

	movlw 0x55	
	xorwf POSTINC2,W
	bnz FAT_parse_VolID_corrupt

	movlw 0xAA
	xorwf POSTINC2,W
	bnz FAT_parse_VolID_corrupt

	retlw .0

;Error Messages
FAT_parse_VolID_corrupt_txt db "Corrupt VolID",0x00
FAT_parse_VolID_corrupt
	LoadTable FAT_parse_VolID_corrupt_txt
	call blit_print_table_string
	retlw .1

;//////////////////////////////////////////////////////////////
;Save the current cluster location as the start
FAT_save_first_cluster
	cp_24bit SD_addr,first_byte_addr
	return

;//////////////////////////////////////////////////////////////
;Load the entry start cluster
FAT_load_first_cluster
	cp_24bit first_byte_addr,SD_addr
	return

;//////////////////////////////////////////////////////////////
;Loads the root folder
FAT_load_root
	;It's a directory
	bsf FAT_is_DIR
	cp_24bit root_dir_first_cluster,SD_addr
	rcall FAT_save_first_cluster
	;The error message is still in W
	return

;/////////////////////////////////////////////////////////////
;Open entry
;Returns with a .0 if successful and the attribute byte of the entry in FSR2
 cblock
sector_count
 endc
FAT_get_entry
	;Always reset to the start of the entry
	rcall FAT_load_first_cluster

FAT_get_entry_cluster_start
	movff sectors_per_cluster,sector_count
	call convert_cluster_in_addr

FAT_get_entry_sector_loop
	call SD_read
	xorlw .0
	btfss STATUS,Z
	retlw .1
	
FAT_get_entry_sector_start
		movlw 0x08
		movwf FSR2H
		movlw 0x0B
		movwf FSR2L

FAT_get_entry_entry_loop
		;This filters FAT32 long names
		;This checks if the VolumeID bit is set, it ignores it if so
		movf INDF2,W
		andlw b'00001000' 
		bnz FAT_get_entry_next_entry

		;Get the first byte of the record
		movlw 0x0B
		subwf FSR2L,F
		movlw .0
		subwfb FSR2H,F
		;Check if the entry is deleted
		movf INDF2,W
		xorlw 0xE5 
		bnz FAT_get_entry_EOD_test
	
		;It's a deleted entry, go back to the attrib byte for the entry skip
		movlw 0x0B
		addwf FSR2L,F
		movlw .0
		addwfc FSR2H,F
		;Then jump to the next entry
		bra FAT_get_entry_next_entry

FAT_get_entry_EOD_test
		;Check if the entry is the end of the directory
		TSTFSZ INDF2,W
		bra FAT_open_end_skip
		retlw .2

FAT_open_end_skip
		;Now we need to compare the names
		;Load the name in FSR1
		movlw HIGH(entryname)
		movwf FSR1H
		movlw LOW(entryname)
		movwf FSR1L

		bcf STATUS,C ;//The carry bit is our error flag
		;Check for all 11 bytes
		local FAT_open_loop_ctr = .11
		while FAT_open_loop_ctr != .0
			movf POSTINC1,W
			xorwf POSTINC2,W
			btfss STATUS,Z
			bsf STATUS,C
FAT_open_loop_ctr = FAT_open_loop_ctr -.1
		endw
		
		;All that above should result in FSR2 being back at the attrib byte
		;Skip if the comparison failed
		bc FAT_get_entry_next_entry
		
		;Success, found the file
		retlw .0

FAT_get_entry_next_entry
		movlw .32
		addwf FSR2L,F
		movlw .0
		addwfc FSR2H,F

		;Check we've not exited the sector
		movlw 0x0A
		xorwf FSR2H,W
		bnz FAT_get_entry_entry_loop

	;We've exited the sector, next sector please
FAT_get_entry_next_sector
		;Increase the sector address
		movlw .2
		add_w_24bit SD_addr

		;Check its not past the last sector and loop
		decfsz sector_count
		bra FAT_get_entry_sector_loop

	;Now use the fat to get the next cluster, first, rewind to the start of the cluster
	;This multiplies by two to convert lba to byte
	bcf STATUS,C
	rlcf sectors_per_cluster,W
	sub_w_24bit SD_addr

	;Load the fat address and check it
	;Turn the byte address into a sector address
	call convert_byte_in_addr
	rcall FAT_get_FAT
	call SD_read
	cp_FSR_24bit 1,SD_addr

	rcall FAT_addr_check
	TSTFSZ WREG
	retlw .3

	goto FAT_get_entry_cluster_start

;/////////////////////////////////////////////////////////
;Uses the FAT table to look up the next cluster, 
;takes the cluster address in SD_addr
;;
;Returns the SD_addr of the FAT and the LSB of the entry in FSR1
 cblock
FAT_sector_offset
 endc 
FAT_get_FAT
	;The first 7 bits are the address of the FAT entry in the FAT sector
	;The other 17 bits are the half sector offsets to the start of the FAT table.

	;save the lowest 8 bits
	movff SD_addr+0,PRODH
	;Rotate right 8bits
	movff SD_addr+1,SD_addr+0
	movff SD_addr+2,SD_addr+1
	clrf SD_addr+2

	;Rotate left one bit
	x2_24bit SD_addr
	;Add the missing bit of information
	btfsc PRODH,7
	bsf SD_addr+0,0

	;Mask that bit off the stored byte
	bcf PRODH,7

	;Load the fat table
	add_24bit SD_addr, fat_begin_lba
	x2_24bit SD_addr
	
	;Set FSR1 to the FAT entry
	movf PRODH,W
	mullw .4
	movff PRODL,FSR1L
	movlw 0x08
	addwf PRODH,W
	movwf FSR1H
	return

;///////////////////////////////////////////////////////////////
;Validates the address (checks its not >= 0xFFFFF8)
FAT_addr_check
	;The lowest byte just needs to be greater or equal to F8
	movlw 0xF8
	xorwf SD_addr+0,W
	andlw b'11111000'
	bnz FAT_addr_check_fine

	movlw 0xFF
	xorwf SD_addr+1,W
	bnz FAT_addr_check_fine

	movlw 0xFF
	xorwf SD_addr+2,W
	bnz FAT_addr_check_fine

	;The highest byte should be 0xFX, where X is reserved

	retlw .1
FAT_addr_check_fine
	retlw .0
;/////////////////////////////////////////////////////////////
;Actually opens an entry in the current directory
FAT_open_entry
	;Check we're not in a file already
	btfss FAT_is_DIR
	retlw .1

	call FAT_get_entry
	TSTFSZ WREG
	retlw .2

	;Check and set the attributes
	bcf FAT_is_DIR
	movlw b'00010000'
	andwf INDF2,W
	btfss STATUS,Z
	bsf FAT_is_DIR
	;Now load the cluster address
	movlw .9
	addwf FSR2L,F
	movlw .0
	addwfc FSR2H,F
	movff POSTINC2, SD_addr+2
	movlw .5
	addwf FSR2L,F
	movlw .0
	addwfc FSR2H,F
	movff POSTINC2, SD_addr+0
	movff POSTINC2, SD_addr+1
	call convert_cluster_in_addr
	call FAT_save_first_cluster
	retlw .0

;/////////////////////////////////////////////////////////////
;Deletes the entry pointed to in SD_addr and FSR2
;Changes WREG, FSR2, FSR1, SD_addr
FAT_delete_entry
	;Check we're not in a file already
	btfss FAT_is_DIR
	retlw .1

	call FAT_get_entry
	TSTFSZ WREG
	retlw .2

	;//We're at the entry, now erase it
	;Load the first byte
	movlw 0x0B
	subwf FSR2L,F
	movlw .0
	subwfb FSR2H,F

	;Erase the directory entry
	movlw 0xE5
	movwf INDF2

	;Switch to FSR1
	movff FSR2H,FSR1H
	movff FSR2L,FSR1L
	;Save the directory record
	call SD_write
	TSTFSZ WREG
	retlw .3

	;Now get the cluster address
	movlw 0x14
	addwf FSR1L,F
	movlw .0
	addwfc FSR1H,F

	movff POSTINC1, SD_addr+2
	movlw .5
	addwf FSR1L,F
	movlw .0
	addwfc FSR1H,F
	movff POSTINC1, SD_addr+0
	movff POSTINC1, SD_addr+1

	;Get the entry in the fat table
	call FAT_get_FAT
	call SD_read
	TSTFSZ WREG
	retlw .4

	;Save the current FAT address
	cp_24bit SD_addr,FAT_temp_addr

FAT_delete_entry_loop
	;Store the next address while wiping it out of the FAT
	movff INDF1, SD_addr+0
	clrf POSTINC1
	movff INDF1, SD_addr+1
	clrf POSTINC1
	movff INDF1, SD_addr+2
	clrf POSTINC1

	;We're ignoring the top 4 bytes
	clrf POSTINC1

	;Check the value wasn't >= 0xXFFFF8, if it isn't then branch
	call FAT_addr_check
	xorlw .0
	bnz FAT_delete_entry_cache_handle

	;So we reached the end of the chain, write the table
	;Restore the FAT location
	cp_24bit FAT_temp_addr,SD_addr
	call FAT_delete_entry_update_FAT
	TSTFSZ WREG
	retlw .5

	retlw .0

FAT_delete_entry_cache_handle
	;Compare SD_addr and FAT_tmp_addr, if Eq. then loop, otherwise write the buffer to FAT_tmp_addr then loop

	;First get the FAT table location for the next cluster
	call FAT_get_FAT

	;Begin the comparison
	movf FAT_temp_addr+0,W
	CPFSEQ SD_addr+0
	bra FAT_delete_entry_cant_cache

	movf FAT_temp_addr+1,W
	CPFSEQ SD_addr+1
	bra FAT_delete_entry_cant_cache

	movf FAT_temp_addr+2,W
	CPFSEQ SD_addr+2
	bra FAT_delete_entry_cant_cache

	;We are already at the FAT table address, just loop without writing or reading
	bra FAT_delete_entry_loop

FAT_delete_entry_cant_cache

	swap_24bit SD_addr,FAT_temp_addr

	call FAT_delete_entry_update_FAT
	cp_24bit FAT_temp_addr,SD_addr
	call SD_read
	TSTFSZ WREG
	retlw .6

	;Now loop
	bra FAT_delete_entry_loop

FAT_delete_entry_update_FAT
	;Writes to the two FAT tables
	call SD_write
	TSTFSZ WREG
	retlw .7

	;Find the second FAT table and write it
	div2_24bit SD_addr	
	add_24bit SD_addr,sectors_per_fat
	x2_24bit SD_addr	
	call SD_write
	return

;/////////////////////////////////////////////////////////////////////////////////
;Gets (or makes) a blank file entry in the current directory
;entryname: The file name is expected to pre-confirmed as unique
;Returns the entry in FSR2, and SD_addr, already loaded
;But you must perform a SD_write to ensure the SD card is up to date, once you've 
;edited the entry
;Changes SD_addr, FSR2, FSR1, WREG, sector_count, FAT_temp_addr, FAT_temp_addr2
FAT_get_blank_entry
	;Check we're not in a file already
	btfss FAT_is_DIR
	retlw .1

	;Always reset to the start of the directory entry
	rcall FAT_load_first_cluster

FAT_get_blank_entry_clust_start
	movff sectors_per_cluster,sector_count
	call convert_cluster_in_addr

FAT_get_blank_entry_sector_loop
	call SD_read
	TSTFSZ WREG
	retlw .2
	
FAT_get_blank_entry_sector_start
		movlw 0x08
		movwf FSR2H
		movlw 0x00
		movwf FSR2L

FAT_get_blank_entry_entry_loop
		;Check if the entry is deleted or EOD
		movf INDF2,W
		;If it is the EOD, we need special treatment
		bz FAT_get_blank_entry_EOD_found

		xorlw 0xE5
		;If the entry is deleted, success!
		btfsc STATUS,Z
		retlw .0

		;Go to the next entry
		movlw .32
		addwf FSR2L,F
		movlw .0
		addwfc FSR2H,F

		;Check we've not exited the sector
		movlw 0x0A
		xorwf FSR2H,W
		bnz FAT_get_blank_entry_entry_loop

		;We've exited the sector, next sector please
		;Increase the sector address
		movlw .2
		add_w_24bit SD_addr

		;Check its not past the last sector and loop
		decfsz sector_count
		bra FAT_get_blank_entry_sector_loop

	;We're passed the last sector of the current cluster
	;Use the fat to get the next cluster, first, rewind to the start of the cluster
	;This multiplies by two to convert lba to byte
	bcf STATUS,C
	rlcf sectors_per_cluster,W
	sub_w_24bit SD_addr

	;Turn the byte address into a sector address
	call convert_byte_in_addr

	;Load the fat address and check it
	rcall FAT_get_FAT
	call SD_read
	TSTFSZ WREG
	retlw .3

	cp_FSR_24bit 1,SD_addr

	rcall FAT_addr_check
	TSTFSZ WREG
	;If the FAT says 0xFFFFFF, we've reached undefined behaviour!
	;There should always be an end directory entry
	retlw .4

	goto FAT_get_blank_entry_clust_start


;//The end of the directory is found in FSR2 and SD_addr, check the next entry
FAT_get_blank_entry_EOD_found
	;Check it's not the last entry in the sector
	movlw 0x09
	xorwf FSR2H,W
	bnz FAT_get_blank_entry_EOD_inc

	movlw 0xE0
	xorwf FSR2L,W
	bnz FAT_get_blank_entry_EOD_inc

	;It's the last entry in a sector
	;Check its not past the last sector of the cluster (sector_count is saved from the prev loop
	dcfsnz sector_count
	bra FAT_get_blank_entry_EOD_EOC

	;It's not past the last sector of the cluster, write a blank sector there and return to the previous sector 
	movlw .2
	add_w_24bit SD_addr
	call SD_write_blank
	TSTFSZ WREG	
	retlw .4	

	movlw .2
	sub_w_24bit SD_addr

	;mark the entry as blank
	movlw 0xE5
	movwf INDF2
	;No need to read, or save FSR2 as SD_write_blank does not munge those 
	retlw .0

FAT_get_blank_entry_EOD_inc
	;The EOD is not the last entry in the sector, just add a deleted entry followed by a EOD 
	;Mark the EOD entry as deleted
	movlw 0xE5
	movwf INDF2

	;Mark the next entry as EOD
	movlw .32
	clrf PLUSW2

	;Then return with the blank entry in FSR2
	retlw .0

FAT_get_blank_entry_EOD_EOC
	;Save the current addr
	cp_24bit SD_addr,FAT_temp_addr

	;The sector was the last sector in the cluster, and probably the last cluster, but we need to check
	;Turn the byte address into a cluster address
	;Only should subtract (sectors_per_cluster-1) sectors, as we didn't overflow
	movlw .1
	subwf sectors_per_cluster,W
	bcf STATUS,C
	rlcf WREG
	sub_w_24bit SD_addr
	call convert_byte_in_addr

	;Load the fat address and check it
	rcall FAT_get_FAT
	call SD_read
	TSTFSZ WREG
	retlw .5
	cp_FSR_24bit 1,SD_addr

	rcall FAT_addr_check
	TSTFSZ WREG
	;If the FAT says 0xFFFFFF we need to add a cluster on
	bra FAT_get_blank_entry_EODir

	;It's not the last sector! Blank the following sector 
	call convert_cluster_in_addr
	call SD_write_blank

	;Restore the original location
	cp_24bit FAT_temp_addr,SD_addr
	call SD_read
	TSTFSZ WREG
	retlw .6

	;Mark the entry as blank
	movlw 0x09
	movwf FSR2H
	movlw 0xE0
	movwf FSR2L

	movlw 0xE5
	movwf INDF2
	retlw .0

FAT_get_blank_entry_EODir
	;//FAT_temp_addr holds the last sector of the fat table, which we're to add a cluster to.
	;//Get a blank entry
	call FAT_get_blank_FAT
	TSTFSZ WREG
	retlw .5

	;//Terminate the FAT chain
	call FAT_terminate_FAT_entry
	TSTFSZ WREG	
	retlw .6
	
	;//Get the sector (FSR1 is set by FAT_terminate_FAT_entry)
	call FAT_get_owner_cluster

	;/Save it
	cp_24bit SD_addr,FAT_temp_addr2

	;//Blank the new memory ;For FSCK to pass it, it must all be wiped
	movff sectors_per_cluster,FAT_temp_var
FAT_get_blank_entry_EODir_loop
	call convert_cluster_in_addr
	call SD_write_blank
	
	;Next sector
	movlw .2
	add_w_24bit SD_addr
	decfsz FAT_temp_var,F
	bra FAT_get_blank_entry_EODir_loop

	;Now write the new cluster to this entry
	;Find the cluster FAT entry
	cp_24bit FAT_temp_addr,SD_addr
	movlw .1
	subwf sectors_per_cluster,W 
	bcf STATUS,C
	rlcf WREG
	sub_w_24bit SD_addr
	call convert_byte_in_addr

	;Get the FAT entry
	rcall FAT_get_FAT
	call SD_read
	TSTFSZ WREG
	retlw .6
	
	movff FAT_temp_addr2+0,POSTINC1
	movff FAT_temp_addr2+1,POSTINC1
	movff FAT_temp_addr2+2,POSTINC1
	movlw .0
	movwf POSTINC1

	call FAT_delete_entry_update_FAT

	;Restore the original location
	cp_24bit FAT_temp_addr,SD_addr
	call SD_read
	TSTFSZ WREG
	retlw .6

	;Mark the entry as blank
	movlw 0x09
	movwf FSR2H
	movlw 0xE0
	movwf FSR2L

	movlw 0xE5
	movwf INDF2
	retlw .0


;/////////////////////////////////////////////////////////////
;Returns with a blank FAT entry in FSR2, and SD_addr
;Changes SD_addr, WREG, FSR2, FAT_tmp_addr2
FAT_get_blank_FAT
	;Load the FAT table
	cp_24bit fat_begin_lba,SD_addr
	x2_24bit SD_addr

	;For the first part of the table we must start at the 3rd entry 
	call SD_read
	TSTFSZ WREG
	retlw .1

	movlw 0x08
	movwf FSR2H
	movlw 0x08
	movwf FSR2L
	bra FAT_get_blank_FAT_loop_2

FAT_get_blank_FAT_loop
	call SD_read
	TSTFSZ WREG
	retlw .2

	movlw 0x08
	movwf FSR2H
	clrf FSR2L
	
FAT_get_blank_FAT_loop_2
	;Check if the entry is blank
	movlw .0
	movf PLUSW2,W
	bnz FAT_get_blank_FAT_loop_2_skip
	movlw .1
	movf PLUSW2,W
	bnz FAT_get_blank_FAT_loop_2_skip
	movlw .2
	movf PLUSW2,W
	bnz FAT_get_blank_FAT_loop_2_skip
	movlw .3
	movf PLUSW2,W
	bnz FAT_get_blank_FAT_loop_2_skip
	;Found a blank fat entry, return
	retlw .0

FAT_get_blank_FAT_loop_2_skip
	;goto the next FAT entry
	movlw .4
	addwf FSR2L,F
	movlw .0
	addwfc FSR2H,F

	;Check if we need to go to the next sector
	movlw 0x0A
	xorwf FSR2H,W
	bnz FAT_get_blank_FAT_loop_2

	;We need to go to the next sector

	;Check it's not the last FAT sector
	cp_24bit SD_addr, FAT_temp_addr2
	div2_24bit FAT_temp_addr2
	sub_24bit FAT_temp_addr2,sectors_per_fat
	sub_24bit FAT_temp_addr2,fat_begin_lba


	movf FAT_temp_addr2+0,W
	bnz FAT_get_blank_FAT_next_sector
	movf FAT_temp_addr2+1,W
	bnz FAT_get_blank_FAT_next_sector
	movf FAT_temp_addr2+2,W
	bnz FAT_get_blank_FAT_next_sector

	;If FAT_temp_addr2 is zeros, then we've hit the next FAT and the disk is full
	retlw .4

FAT_get_blank_FAT_next_sector
	;It's not the last sector, so jump to the next
	movlw .2
	add_w_24bit SD_addr

	;It's ok, so loop
	bra FAT_get_blank_FAT_loop

;///////////////////////////////////////////////////////////////////////
;Ends the current FAT chain in SD_addr and FSR2. Restores both of those 
;afterwards and saves FSR2 in FSR1 
FAT_terminate_FAT_entry
	;Save the FSR (The save to FSR1 is expected by FAT_get_blank_entry, FSR1 must not change after this set)
	movff FSR2H,FSR1H
	movff FSR2L,FSR1L
	;Set the entry to "in use" 0x0FFFFFFF
	movlw 0xF8
	movwf POSTINC2
	movlw 0xFF
	movwf POSTINC2
	movlw 0xFF
	movwf POSTINC2
	movlw 0x0F
	movwf INDF2

	call FAT_delete_entry_update_FAT
	TSTFSZ WREG
	retlw .1

	;/Reset it to the first FAT after FAT_delete_entry_update_FAT
	div2_24bit SD_addr	
	sub_24bit SD_addr,sectors_per_fat
	x2_24bit SD_addr

	;Restore the FSR
	movff FSR1H,FSR2H
	movff FSR1L,FSR2L
	retlw .0

;//////////////////////////////////////////////////////////////////////
;This hooks into FAT_get_blank_FAT to actually fetch a whole chain
;WREG contains the number of blank clusters req.
;return: the cluster address of the first fat entry is in FAT_temp_addr
;changes: SD_addr, WREG, FSR2, FAT_temp_addr, FAT_temp_addr2, FAT_temp_var,FAT_temp_var2
FAT_get_blank_FAT_chain
;Walk the number of blank entries to find the end
	addlw -.1
	;Save the number of loops
	movwf FAT_temp_var2
	;First loop variable
	movwf FAT_temp_var

;As an optimisation we manually walk the first, then jump back into the loop of FAT_get_blank_FAT
	call FAT_get_blank_FAT
	TSTFSZ WREG
	retlw .1

FAT_get_blank_FAT_chain_loop
	call FAT_get_blank_FAT_loop_2_skip
	TSTFSZ WREG
	retlw .2

	decfsz FAT_temp_var
	bra FAT_get_blank_FAT_chain_loop

;Now we're at the end of the chain, terminate it, This stores the offset in FSR1
	call FAT_terminate_FAT_entry
	TSTFSZ WREG
	retlw .3

;Save the current place by calculating the cluster number
	call FAT_get_owner_cluster
	cp_24bit SD_addr, FAT_temp_addr

;Then recalculate the FAT address
	call FAT_get_FAT

	;Load the table and jump into the loop
	call SD_read
	TSTFSZ WREG
	retlw .2

	movff FSR1H,FSR2H
	movff FSR1L,FSR2L

	bra FAT_get_blank_FAT_chain_entloop_skip

FAT_get_blank_FAT_chain_secloop
	call SD_read
	TSTFSZ WREG
	retlw .2

	movlw 0x09
	movwf FSR2H
	movlw 0xFC
	movwf FSR2L
	
FAT_get_blank_FAT_chain_entloop
	;Check if the entry is blank
	movlw .0
	movf PLUSW2,W
	bnz FAT_get_blank_FAT_chain_entloop_skip
	movlw .1
	movf PLUSW2,W
	bnz FAT_get_blank_FAT_chain_entloop_skip
	movlw .2
	movf PLUSW2,W
	bnz FAT_get_blank_FAT_chain_entloop_skip
	movlw .3
	movf PLUSW2,W
	bnz FAT_get_blank_FAT_chain_entloop_skip

	;Found a blank fat entry, Write the previous cluster number
	movff FAT_temp_addr+0,POSTINC2
	movff FAT_temp_addr+1,POSTINC2
	movff FAT_temp_addr+2,POSTINC2
	clrf POSTINC2

	;Now calculate the cluster number of this FAT entry
	movlw .4
	subwf FSR2L,F
	movlw .0
	subwfb FSR2H,F

	movff FSR2H,FSR1H
	movff FSR2L,FSR1L
	call FAT_get_owner_cluster

	;Save it
	cp_24bit SD_addr, FAT_temp_addr
	;Then recalculate the FAT address
	;call convert_cluster_in_addr
	call FAT_get_FAT

	;Now decide if this is enough clusters
	decfsz FAT_temp_var2
	bra FAT_get_blank_FAT_chain_entloop_skip

	;We're done, save the FAT table
	call FAT_delete_entry_update_FAT
	TSTFSZ WREG
	retlw .139

	retlw .0

FAT_get_blank_FAT_chain_entloop_skip
	;goto the previous FAT entry
	movlw .4
	subwf FSR2L,F
	movlw .0
	subwfb FSR2H,F

	;Check if we need to go to the next sector
	movlw 0x07
	xorwf FSR2H,W
	bnz FAT_get_blank_FAT_chain_entloop

	;We need to go to the next sector, 
	;Write the changes so far
	call FAT_delete_entry_update_FAT
	TSTFSZ WREG
	retlw .140

	;/Reset it to the first FAT after FAT_delete_entry_update_FAT
	div2_24bit SD_addr	
	sub_24bit SD_addr,sectors_per_fat
	x2_24bit SD_addr

	;save the current address
	cp_24bit SD_addr, FAT_temp_addr2
	div2_24bit FAT_temp_addr2
	sub_24bit FAT_temp_addr2,fat_begin_lba

	;Check it's not the First FAT sector
	movf FAT_temp_addr2+0,W
	bnz FAT_get_blank_FAT_chain_nextsec
	movf FAT_temp_addr2+1,W
	bnz FAT_get_blank_FAT_chain_nextsec
	movf FAT_temp_addr2+2,W
	bnz FAT_get_blank_FAT_chain_nextsec

	;If FAT_temp_addr2 is zeros, then we've hit the top of the FAT and something has gone wrong
	retlw .10

FAT_get_blank_FAT_chain_nextsec
	;It's not the last sector, so jump to the prev sector
	movlw .2
	sub_w_24bit SD_addr

	;It's ok, so loop
	bra FAT_get_blank_FAT_chain_secloop

;//////////////////////////////////////////////////////////////////////
;This calculates the cluster belonging to a FAT entry stored in SD_addr and FSR1
;SD_addr is set so that a SD_read will load the cluster start sector
;
;Changes SD_addr, FSR1, WREG, PRODH
FAT_get_owner_cluster
	;The MS 17Bits of the cluster are in SD_addr, shifted left by one
	;and the last 7 are in (FSR1 - .0x0800)/4

	;Restore SD_addr to the upper 17bits of the cluster address
	div2_24bit SD_addr
	sub_24bit SD_addr, fat_begin_lba

	;No need to do the minus 0x08 from FSR1, just keep only the LSB and it's the same
	;Blank the lower two bits
	movlw b'11111100'
	andwf FSR1L,W
	rrncf WREG
	rrncf WREG
	;Now the 6 LSB bits of the cluster number is stored in WREG	

	;Bit 6 is bit 1 of FSR1H
	btfsc FSR1H,0
	bsf WREG,6
	
	;Bit7 is bit 0 of SD_addr
	btfsc SD_addr+0,0
	bsf WREG,7

	;Save this byte
	movwf PRODH
	
	;Rotate SD_addr right to get rid of bit 7 of the cluster address
	div2_24bit SD_addr

	;Now rotate left 8 bits, rotating in the stored 8 LSB 
	movff SD_addr+1,SD_addr+2
	movff SD_addr+0,SD_addr+1
	movff PRODH,SD_addr+0

	retlw .0

;///////////////////////////////////////////////////////////////////////
;Makes a new entry with a name of entryname and attributes of FAT_newfile_attrib, and of size
; WREG clusters
;The FAT_get_blank_entry entry pretty much changes every variable
FAT_new_entry
	;Blank FAT_temp_addr3 incase we have a file of size 0
	clrf FAT_temp_addr3+0
	clrf FAT_temp_addr3+1
	clrf FAT_temp_addr3+2

	;Now allocate the clusters if required
	movf FAT_newfile_size,W
	bz FAT_new_entry_write_entry

	call FAT_get_blank_FAT_chain
	TSTFSZ WREG
	retlw .115

	cp_24bit FAT_temp_addr,FAT_temp_addr3

FAT_new_entry_write_entry
	call FAT_get_blank_entry
	TSTFSZ WREG
	retlw .1

	;Write the file name (offset 0x00)
	movlw LOW(entryname+0)
	movwf FSR1L
	movlw HIGH(entryname+0)
	movwf FSR1H

	movlw .11
	movwf FAT_temp_var
FAT_new_entry_name_loop
	movff POSTINC1,POSTINC2	

	decfsz FAT_temp_var
	bra FAT_new_entry_name_loop

	;Write the attribute byte (offset 0x0B)
	movff FAT_newfile_attrib,POSTINC2

	clrf POSTINC2 ;Case information (leave as 0x00) (offset 0x0C)
	clrf POSTINC2 ;Create time (10 ms) (offset 0x0D)
	clrf POSTINC2 ;"........." b'MMMSSSSS' (offset 0x0E)
	clrf POSTINC2 ;"........." b'HHHHHMMM'
	clrf POSTINC2 ;Create date b'MMMDDDDD' (offset 0x10)
	clrf POSTINC2 ;"........." b'YYYYYYYM'
	clrf POSTINC2 ;Last access date b'MMMDDDDD' (offset 0x12)
	clrf POSTINC2 ;".............." b'YYYYYYYM' 
	
	;Cluster high bytes (offset 0x14) (0x00000000 is 0 size)
	movff FAT_temp_addr3+2,POSTINC2
	clrf POSTINC2

	clrf POSTINC2 ;Last modified time b'MMMSSSSS' (offset 0x16)
	clrf POSTINC2 ;"................" b'HHHHHMMM'
	clrf POSTINC2 ;Last modified date b'MMMDDDDD' (offset 0x18)
	clrf POSTINC2 ;"................" b'YYYYYYYM'

	;Cluster low bytes (offset 0x1A)
	movff FAT_temp_addr3+0,POSTINC2
	movff FAT_temp_addr3+1,POSTINC2

	movf FAT_newfile_size,W
	mulwf sectors_per_cluster

	;There are 512 bytes per sector, so the size is PROD << 9, so just rlf once then write the high bytes
	movlw .0
	bcf STATUS,C
	rlcf PRODL,F
	rlcf PRODH,F
	btfsc STATUS,C
	bsf WREG,0	

	;File size in bytes (offset 0x1C)
	clrf POSTINC2
	movff PRODL,POSTINC2	
	movff PRODH,POSTINC2	
	movwf POSTINC2

	call SD_write
	TSTFSZ WREG
	retlw .203

	movf FAT_newfile_size,W
	bz FAT_new_entry_name_return

	cp_24bit FAT_temp_addr3,SD_addr
	call convert_cluster_in_addr
	call SD_read
	TSTFSZ WREG
	retlw .202
FAT_new_entry_name_return
	retlw .0
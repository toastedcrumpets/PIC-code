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
padding:.12
entryname:.11
;The first cluster byte address for the entry
first_byte_addr:3
;Save point for SD_addr during deletes etc.
FAT_temp_addr:3
;What the current cluster is
;[7] : 1 folder 0 file
currentEntryState
 endc

 #define is_DIR currentEntryState,7

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
	movff POSTINC#v(a)+0,b+0
	movff POSTINC#v(a)+1,b+1
	movff POSTINC#v(a)+2,b+2
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
	bsf is_DIR
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
;Validates the address (checks its not 0xFFFFFF)
FAT_addr_check
	movlw 0xFF
	xorwf SD_addr+0,W
	bnz FAT_addr_check_fine

	movlw 0xFF
	xorwf SD_addr+1,W
	bnz FAT_addr_check_fine

	movlw 0xFF
	xorwf SD_addr+2,W
	bnz FAT_addr_check_fine

	retlw .1
FAT_addr_check_fine
	retlw .0
;/////////////////////////////////////////////////////////////
;Actually opens an entry in the current directory
FAT_open_entry
	;Check we're not in a file already
	btfss is_DIR
	retlw .1

	call FAT_get_entry
	TSTFSZ WREG
	retlw .2

	;Check and set the attributes
	bcf is_DIR
	movlw b'00010000'
	andwf INDF2,W
	btfss STATUS,Z
	bsf is_DIR
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
;Deletes the entry pointed to in FSR2
FAT_delete_entry
	nop
	nop

	call FAT_get_entry
	TSTFSZ WREG
	retlw .1

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
	retlw .2

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

	;Check the value wasn't 0xXFFFFF, if it isn't then branch
	;REMEMBER, in the FAT the upper 4 bits are reserved but we're ignoring the top 8 bits!
	movlw 0xFF
	CPFSEQ SD_addr+0
	bra FAT_delete_entry_cache_handle

	CPFSEQ SD_addr+1
	bra FAT_delete_entry_cache_handle

	CPFSEQ SD_addr+2
	bra FAT_delete_entry_cache_handle

	;So we reached the end of the chain, write the table
	;Restore the FAT location
	cp_24bit FAT_temp_addr,SD_addr
	call FAT_delete_entry_update_FAT
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
	;Now loop
	bra FAT_delete_entry_loop

FAT_delete_entry_update_FAT
	;Writes to the two FAT tables
	call SD_write
	;Find the second FAT table and write it
	div2_24bit SD_addr	
	add_24bit SD_addr,sectors_per_fat
	x2_24bit SD_addr	
	call SD_write
	return
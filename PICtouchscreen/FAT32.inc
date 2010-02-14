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
;What the current cluster is
;[7] : 1 folder 0 file
currentEntryState
 endc

 #define is_DIR currentEntryState,7

;////////////////////////////////////////////////////////
;Helper macros and functions
Sector_to_byte macro sector
	x2_24bit sector, SD_addr
	endm

x2_24bit macro in,out
	bcf STATUS,C
	rlcf in+0,W
	movwf out+0
	rlcf in+1,W
	movwf out+1
	rlcf in+2,W
	movwf out+2
	endm

div2_24bit macro in,out
	bcf STATUS,C
	rrcf in+2,W
	movwf out+2
	rrcf in+1,W
	movwf out+1
	rrcf in+0,W
	movwf out+0
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

mul_24bit_by2 macro a
	bcf STATUS,C
	endm

;////////////////////////////////////////////////////////
;This function is used when you load a cluster address and want a
;data address
 cblock
convert_cluster_tmp
 endc
convert_cluster_in_addr_no_offset
	;first subtract 2, as all cluster addresses start at 2
	movlw .2
	subwf SD_addr+0,F
	movlw .0
	subwfb SD_addr+1,F
	movlw .0
	subwfb SD_addr+2,F
	
	;Now we multiply by the sectors per cluster
	movf sectors_per_cluster_power,W
	bz convert_cluster_in_addr_skip

	movwf convert_cluster_tmp	
convert_cluster_in_addr_loop
	x2_24bit SD_addr,SD_addr

	decfsz convert_cluster_tmp
	bra convert_cluster_in_addr_loop

convert_cluster_in_addr_skip
	return

convert_cluster_in_addr
	call convert_cluster_in_addr_no_offset
	;Finally we add the cluster_begin_lba
	add_24bit SD_addr, cluster_begin_lba
	;Multiply by two again for sectors to bytes
	x2_24bit SD_addr,SD_addr
	;We're done
	return

;//////////////////////////////////////////////////////////////
;this converts the byte address back to a cluster number
convert_byte_in_addr
	;Divide by two, to get the sector address
	div2_24bit SD_addr,SD_addr

	;subtract the cluster offset
	sub_24bit SD_addr, cluster_begin_lba

	;Now we divide by the sectors per cluster
	movf sectors_per_cluster_power,W
	bz convert_cluster_in_addr_skip

	movwf convert_cluster_tmp	
convert_byte_in_addr_loop
	div2_24bit SD_addr,SD_addr

	decfsz convert_cluster_tmp
	bra convert_byte_in_addr_loop

convert_byte_in_addr_skip
	;finally add 2, as all cluster addresses start at 2
	movlw .2
	addwf SD_addr+0,F
	movlw .0
	addwfc SD_addr+1,F
	movlw .0
	addwfc SD_addr+2,F
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
	
	xorlw .0
	xorlw .0

	movlw "T"
	movwf entryname+.0
	movlw "E"
	movwf entryname+.1
	movlw "S"
	movwf entryname+.2
	movlw "T"
	movwf entryname+.3
	movlw "1"
	movwf entryname+.4
	movlw "0"
	movwf entryname+.5
	movlw "1"
	movwf entryname+.6
	movlw " "
	movwf entryname+.7
	movlw "T"
	movwf entryname+.8
	movlw "X"
	movwf entryname+.9
	movlw "T"
	movwf entryname+.10

	call FAT_load_root
	call FAT_open_entry
	xorlw .0
	bnz FAT_init_fail

	call SD_read
	xorlw .0
	xorlw .0

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

	;This scales the sector address to a byte address
	movff POSTINC2,fat_begin_lba+0
	movff POSTINC2,fat_begin_lba+1
	movff POSTINC2,fat_begin_lba+2

	Sector_to_byte fat_begin_lba

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
	movff POSTINC2,sectors_per_fat+0
	movff POSTINC2,sectors_per_fat+1
	movff POSTINC2,sectors_per_fat+2

	;Make this into the cluster lba
	movff sectors_per_fat+0,cluster_begin_lba+0
	movff sectors_per_fat+1,cluster_begin_lba+1
	movff sectors_per_fat+2,cluster_begin_lba+2

	bcf STATUS,C
	;Multiply by two
	rlcf cluster_begin_lba+0,F
	rlcf cluster_begin_lba+1,F
	rlcf cluster_begin_lba+2,F
	;Then add fat_begin_lba
	add_24bit cluster_begin_lba, fat_begin_lba
	
	;Now load the Root directory first cluster
	movlw 0x2C
	movwf FSR2L
	movff POSTINC2,root_dir_first_cluster+0
	movff POSTINC2,root_dir_first_cluster+1
	movff POSTINC2,root_dir_first_cluster+2

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
	movff SD_addr+2,first_byte_addr+2
	movff SD_addr+1,first_byte_addr+1
	movff SD_addr+0,first_byte_addr+0
	return

;//////////////////////////////////////////////////////////////
;Load the entry start cluster
FAT_load_first_cluster
	movff first_byte_addr+2,SD_addr+2
	movff first_byte_addr+1,SD_addr+1
	movff first_byte_addr+0,SD_addr+0
	return

;//////////////////////////////////////////////////////////////
;Loads the root folder
FAT_load_root
	;It's a directory
	bsf is_DIR

	movff root_dir_first_cluster+2,SD_addr+2
	movff root_dir_first_cluster+1,SD_addr+1
	movff root_dir_first_cluster+0,SD_addr+0
	rcall FAT_save_first_cluster
	;The error message is still in W
	return

;/////////////////////////////////////////////////////////////
;Open entry
 cblock
sector_count
 endc
FAT_open_entry
	;Check we're not in a file already
	btfss is_DIR
	retlw .1

	;Always reset to the start of the entry
	rcall FAT_load_first_cluster

FAT_open_entry_cluster_start
	movff sectors_per_cluster,sector_count
	call convert_cluster_in_addr

FAT_open_entry_sector_loop
	call SD_read
	xorlw .0
	btfss STATUS,Z
	retlw .3
	
	FAT_open_entry_sector_start
		movlw 0x08
		movwf FSR2H
		movlw 0x0B
		movwf FSR2L

	FAT_open_entry_entry_loop
		;This checks if the VolumeID bit is set, it ignores it if so
		movf INDF2,W
		andlw b'00001000' 
		bnz FAT_open_entry_next_entry

		;Now we need to compare the names
		movlw 0x0B
		subwf FSR2L,F
		movlw .0
		subwfb FSR2H,F

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
		bc FAT_open_entry_next_entry
		
		;Success, found the file
		;Check the attributes
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

	FAT_open_entry_next_entry
		movlw .32
		addwf FSR2L,F
		movlw .0
		addwfc FSR2H,F

		;Check we've not exited the sector
		movlw 0x0A
		xorwf FSR2H,W
		bnz FAT_open_entry_entry_loop

	;We've exited the sector, next sector please
	FAT_open_entry_next_sector
		;Increase the sector address
		movlw .2 ;A sector is 2 in byte adresses	
		addwf SD_addr+0,F
		movlw .0
		addwfc SD_addr+1,F
		movlw .0
		addwfc SD_addr+2,F

		;Check its not past the last sector and loop
		decfsz sector_count
		bra FAT_open_entry_sector_loop

	;Now use the fat to get the next cluster, first, rewind to the start of the cluster
	;This multiplies by two to convert lba to byte
	bcf STATUS,C
	rlcf sectors_per_cluster,W

	subwf SD_addr+0,F
	movlw .0
	subwfb SD_addr+1,F
	movlw .0
	subwfb SD_addr+2,F

	call FAT_follow_FAT

	;Now we need to check it's a valid lookup
	call FAT_addr_check
	xorlw .0
	bz FAT_open_entry_cluster_start

	;Failed to lookup the file
	retlw .1
;/////////////////////////////////////////////////////////
;Uses the FAT table to look up the next cluster
 cblock
FAT_sector_offset
 endc 
FAT_follow_FAT
	;Turn the byte address into a sector address
	call convert_byte_in_addr
	
	;The first 5 bits are the address of the FAT entry in the FAT sector
	;The other 19bits are the half sector offsets to the start of the FAT table.
	movf SD_addr+0,W
	andlw b'00011111'
	movwf FAT_sector_offset

	;Now divide by 5, to get the sector of the FAT required
	;TODO: fix this expensive piece of code, try using swaps
	div2_24bit SD_addr,SD_addr
	div2_24bit SD_addr,SD_addr
	div2_24bit SD_addr,SD_addr
	div2_24bit SD_addr,SD_addr
	div2_24bit SD_addr,SD_addr

	;Finally we add the fat_begin_lba
	add_24bit SD_addr, fat_begin_lba
	;Multiply by two again for sectors to bytes
	x2_24bit SD_addr, SD_addr

	;Load the fat table
	call SD_read
	
	;Open the buffer
	movf FAT_sector_offset,W
	mullw .4
	movff PRODL,FSR2L
	movlw 0x08
	addwf PRODH,W
	movwf FSR2H
	
	movff POSTINC2,SD_addr+0
	movff POSTINC2,SD_addr+1
	movff POSTINC2,SD_addr+2
	;Now exit and leave it to the caller to decide if a load is needed
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
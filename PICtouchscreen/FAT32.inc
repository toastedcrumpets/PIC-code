;FAT 32 driver for SD cards

;16 bit variables
 cblock
fat_begin_lba:4
cluster_begin_lba:4
sectors_per_cluster
root_dir_first_cluster:4
 endc

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

	;This scales the sector address to a byte address
	movff POSTINC2,fat_begin_lba+2
	bcf STATUS,C
	rlcf fat_begin_lba+2,W
	movwf SD_addr+2
	movff POSTINC2,fat_begin_lba+1
	rlcf fat_begin_lba+1,W
	movwf SD_addr+1
	movff POSTINC2,fat_begin_lba+0
	rlcf fat_begin_lba+0,W
	movwf SD_addr+0

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
;Checks for 0x0002 at 0x080B, and 0x02 at 0x0810,
;Then loads the data
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

	movff POSTINC2,sectors_per_cluster

	;Now load the number of reserved sectors and add it to the fat_begin_lba
	movf POSTINC2,W
	addwf fat_begin_lba+2,f
	movf POSTINC2,W
	addwfc fat_begin_lba+1,f
	movlw .0
	addwfc fat_begin_lba+0,f

	;final verification byte
	movf POSTINC2,W
	xorlw 0x02
	bnz FAT_parse_VolID_corrupt

	retlw .0

;Error Messages
FAT_parse_VolID_corrupt_txt db "Corrupt VolID",0x00
FAT_parse_VolID_corrupt
	LoadTable FAT_parse_VolID_corrupt_txt
	call blit_print_table_string
	retlw .1

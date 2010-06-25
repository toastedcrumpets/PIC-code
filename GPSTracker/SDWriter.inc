;Handler for printing to the SD card

 cblock
sector_counter
 endc

;Finds the end of the current string
SD_init_seek
	SD_load_buffer_FSR 0,0x0000
	movlw .1
	movwf sector_counter
	return

SD_write_char
	movwf POSTINC0

	SD_load_buffer_H + .2
	xorwf FSR0H,W
	bz SD_write_char_full_buf
	retlw .0

SD_write_char_full_buf
	;Buffer is full, write it to the file
	cp_24bit SD_addr, FAT_temp_addr

	call SD_write
	xorlw .0
	bnz SD_write_char_fail


SD_write_char_EOF_reentry
	;Fetch the next sector of the file
	call FAT_next_sector
	xorlw .0
	bz SD_write_char_next_sector
	xorlw .1
	bz SD_write_char_EOF

	bra SD_write_char_fail
	
SD_write_char_next_sector
	;Now we have the address of the next sector, save it
	cp_24bit SD_addr, FAT_temp_addr

	;Load the directory entry and update the size field
	;First update the file size in sectors
	incf sector_counter,f
	;Now open the directory entry
	call FAT_get_entry
	TSTFSZ WREG
	retlw .1

	;Goto the file size section
	movlw .17
	addwf FSR2L
	movlw .0
	addwfc FSR2H

	;Zero the first byte
	clrf POSTINC2
	;the second byte is the sector count times 2
	bcf STATUS,C
	rlcf sector_counter,w
	movwf POSTINC2
	
	clrf INDF2
	btfsc STATUS,C
	incf INDF2,f

	clrf PREINC2

	call SD_write
	TSTFSZ WREG
	retlw .1

	;Now relocate to the end of the file
	cp_24bit FAT_temp_addr, SD_addr
	SD_load_buffer_FSR 0,0x000

	retlw .0

SD_write_char_EOF
	;Grow the current file by a cluster
	call FAT_grow_file
	TSTFSZ WREG
	retlw .1

	;We should be back at the point where we need to proceed to the next sector, so jump back in!
	bra SD_write_char_EOF_reentry

SD_write_char_fail
	LED_FLASH_5hz
	bcf PIE1,RCIE
	retlw .1
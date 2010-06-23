;Handler for printing to the SD card

 cblock
RCBYTE,STATE
 endc

;Finds the end of the current string
SD_init_seek
	clrf STATE
	SD_load_buffer_FSR 0,0x0000
	return

SD_write_char
	;Read the data
	movf RCREG,W

	movwf RCBYTE
	xorlw 0xA2
	bz SD_write_start

	movf RCBYTE,W
	xorlw 0xA0
	bz SD_write_start2
	
	clrf STATE
	return

SD_write_start
	bsf STATE,0
	return

SD_write_start2
	btfsc STATE,0
	btg LED

	clrf STATE
	return










	movwf POSTINC0

	SD_load_buffer_H + .2
	xorwf FSR0H,W
	bz SD_write_char_full_buf
	retlw .0

SD_write_char_full_buf

	;Buffer is full, write it to the file
	call SD_write
	xorlw .0
	bnz SD_write_char_fail

	;Fetch the next sector of the file
	call FAT_next_sector
	xorlw .0
	bz SD_write_char_next_sector
	xorlw .1
	bnz SD_write_char_fail
	
	;Reached the end of the file, need to extend the file here
	bra SD_write_char_EOF
	
SD_write_char_next_sector
	SD_load_buffer_FSR 0,0x000
	retlw .0

SD_write_char_EOF
	LED_FLASH_5hz
	bcf PIE1,RCIE
	retlw .1

SD_write_char_fail
	LED_FLASH_5hz
	bcf PIE1,RCIE
	retlw .1
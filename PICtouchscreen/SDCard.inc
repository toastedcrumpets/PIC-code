;SD card driver
;#define SD_CS LATD,2
;#define SD_DI LATD,3
;#define SD_CLK LATD,4
;#define SD_DO PORTD,5

 __badram 0x0800-0x09ff

 cblock
SPI_timeout_block,SPI_timeout_block_2
SD_addr:3 ;Upper 3 bytes of the address (LSB cannot be set)
 endc

;////////////////////////////////////////////////////////
;Table of SD commands, bit details
;[47] start bit (a 0)
;[46] transmission bit (1= host to device)
;[45:40] Command index
;[39:8] Argument (E.g. address)
;[7:1] CRC7
;[0] end bit

;//No start bit (a 0), useful for generating clock cycles
SD_CMD_Blank db 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
;//Reset the card, also detects SPI mode if CS is low and is the first reset
SD_CMD0 db 0x40,0x00,0x00,0x00,0x00,0x95
;//High capacity mode select (off)
SD_CMD1 db 0x41,0x00,0x00,0x00,0x00,0xFF

;///////////////////////////////////////////////////////
;Error messages
CMD0_fail db "CMD0 failure",0x00
CMD1_fail db "CMD1 failure",0x00

;///////////////////////////////////////////////////
SD_init
	;//Clear the SPI clock
	bcf SD_CLK

	call Load_framebuffer
	;Deselect the card
	bsf SD_CS

	;Send some toggles to wake the card up
	LoadTable SD_CMD_Blank
	call SD_Send_Cmd
	LoadTable SD_CMD_Blank
	call SD_Send_Cmd

	;Reselect the card
	bcf SD_CS

	;Send CMD0
	LoadTable SD_CMD0
	call SD_Send_Cmd

	call SD_Recv_R1
	xorlw 0x01
	btfss STATUS,Z
	retlw .1

	clrf SPI_timeout_block

SD_init_CMD1_loop
	LoadTable SD_CMD1
	call SD_Send_Cmd
	call SD_Recv_R1
	
	;call W_2HEX_to_LCD
	;movf SD_SPI_recv,W

	xorlw 0x00
	btfsc STATUS,Z
	bra SD_init_end

	decfsz SPI_timeout_block
	bra SD_init_CMD1_loop

	retlw .2
	
SD_init_end
	;Deselect the card, set the clk high
	bsf SD_CS
	bsf SD_CLK
	retlw .0

;///////////////////////////////////////////////////
;Send a command stored in the table pointer (6 bytes long) (no sense in using a loop here)
SD_Send_Cmd
	TBLRD*+	
	movf TABLAT,W
	rcall SD_tranceive
	TBLRD*+	
	movf TABLAT,W
	rcall SD_tranceive
	TBLRD*+	
	movf TABLAT,W
	rcall SD_tranceive
	TBLRD*+	
	movf TABLAT,W
	rcall SD_tranceive
	TBLRD*+	
	movf TABLAT,W
	rcall SD_tranceive
	TBLRD*+	
	movf TABLAT,W
	rcall SD_tranceive
	return

;///////////////////////////////////////////////////
;Recieve loop for the confirmation byte
;R1 has the following format
;[7] = 0
;[6] Parameter error
;[5] Address error
;[4] erase sequence error
;[3] com crc error
;[2] illegal command
;[1] erase reset
;[0] idle

SD_Recv_R1
	clrf SPI_timeout_block

SD_Recv_R1_loop
	;/Transmit a dummy byte and check what's recieved	
	movlw 0xff
	call SD_tranceive
	
	;this just checks if the value is non-zero (some kind of response)
	addlw 0x80
	addlw 0x80
	btfsc STATUS,C
	return

	decfsz SPI_timeout_block
	bra SD_Recv_R1_loop
	
	;Timed out waiting for a response, return error
	retlw 0xff

;///////////////////////////////////////////////////
 cblock
SD_SPI_recv
 endc
SD_tranceive
	;Make the carry bit a 1
	bsf STATUS,C
	rlcf WREG

	;blank the recv buffer SD_SPI_recv
	clrf SD_SPI_recv
SD_tranceive_loop
	;Rising edge
	bsf	SD_CLK

	;Set the line state
	bsf	SD_DI
	btfss STATUS,C
	bcf	SD_DI

	;Falling edge
 	bcf	SD_CLK

	;Read the data and write the
	btfsc SD_DO
	bsf SD_SPI_recv,7
	rlncf SD_SPI_recv

	bcf STATUS,C
	rlcf WREG
	bnz	SD_tranceive_loop

	movf SD_SPI_recv,W
	return

;//////////////////////////////////////////////////////
;Read a block into memory
SD_read
	bsf SD_CS
	;Select the device
	bcf SD_CS

	;Start the command
	movlw 0xFF
	call SD_tranceive
	movlw 0x51
	call SD_tranceive

	;Transmit the address
	movf SD_addr+2,W
	call SD_tranceive
	movf SD_addr+1,W
	call SD_tranceive
	movf SD_addr,W
	call SD_tranceive
	movlw 0x00

	;The fake CRC
	call SD_tranceive
	movlw 0xFF
	call SD_tranceive

	;Wait for the confirmation byte
	call SD_Recv_R1

	xorlw 0x00
	btfss STATUS,Z
	retlw .1

	;Loop waiting for the transmission begin byte
	clrf SPI_timeout_block
SD_read_loop
	call SD_tranceive
	xorlw 0xFE
	bz SD_read_copy

	decfsz SPI_timeout_block
	bra SD_read_loop
	retlw .2

	;Begin the copy into the buffer
SD_read_copy
	;Load the buffer
	movlw 0x08
	movwf FSR2H
	clrf FSR2L

SD_read_copy_loop
	;This is purely for my scope
	movlw 0xFF
	rcall SD_tranceive
	movwf POSTINC2
	movf FSR2H,W
	xorlw 0x0A
	bnz SD_read_copy_loop

	movlw 0xFF
	call SD_tranceive
	movlw 0xFF
	call SD_tranceive

	;Deselect the card, set the clk high
	bsf SD_CS
	bsf SD_CLK
	retlw .0

;//////////////////////////////////////////////////////
;Write a block into memory
SD_write
	bsf SD_CS
	;Select the device
	bcf SD_CS

	;Start the command
	movlw 0xFF
	call SD_tranceive
	movlw 0x58
	call SD_tranceive

	;Transmit the address
	movf SD_addr+2,W
	call SD_tranceive
	movf SD_addr+1,W
	call SD_tranceive
	movf SD_addr,W
	call SD_tranceive
	movlw 0x00
	call SD_tranceive
	;The fake CRC
	movlw 0xFF
	call SD_tranceive

	;Wait for the confirmation byte
	call SD_Recv_R1

	xorlw 0x00
	btfss STATUS,Z
	retlw .1

	;Load the buffer
	movlw 0x08
	movwf FSR2H
	clrf FSR2L

	;Give the SD card a few cycles
	movlw 0xFF
	call SD_tranceive
	movlw 0xFF
	call SD_tranceive
	;Transmit the start byte
	movlw 0xFE
	call SD_tranceive

SD_write_loop
	movf POSTINC2,W
	rcall SD_tranceive

	movf FSR2H,W
	xorlw 0x0A
	bnz SD_write_loop

	;Send dummy CRC
	movlw 0xFF
	call SD_tranceive
	movlw 0xFF
	call SD_tranceive

	clrf SPI_timeout_block
SD_write_loop_wait
	call SD_tranceive
	
	;Check if the card has responded
	xorlw 0xE5
	btfsc STATUS,Z
	bra SD_write_busy		

	;otherwise we must wait!
	;Try delaying a little, this can be a long process
	call delay_20ms

	decfsz SPI_timeout_block	
	bra SD_write_loop_wait

	;Failed to recieve the confirmation
	retlw .2

SD_write_busy

	call SD_wait_till_free
	;Deselect the card, set the clk high
	bsf SD_CS
	bsf SD_CLK
	retlw .0

;/////////////////////////////////////////////////////
SD_wait_till_free
	clrf SPI_timeout_block_2
SD_wait_till_free_loop
	movlw 0xFF
	call SD_tranceive
	xorlw 0x00
	btfss STATUS,Z
	retlw .0
	call delay_20ms

	decfsz SPI_timeout_block_2
	bra SD_wait_till_free_loop

	retlw .1
;SD card driver
;#define SD_CS LATD,2
;#define SD_DI LATD,3
;#define SD_CLK LATD,4
;#define SD_DO PORTD,5

;The buffer of data must be 256bit aligned, 
;so you can only set the high ram byte


;This just helps you keep out of the memory without using
;indirect addressing
 __badram 0x0800-0x09ff

 cblock
SPI_timeout_block,SPI_timeout_block_2
SD_addr:3 ;LBA of the sector (sub sectors cannot be set)
SD_mode
 endc

#define SD_ver2.0 SD_mode,0 
#define SD_HC SD_mode,1 

;////////////////////////////////////////////////////////
;Table of SD commands, bit details
;[47] start bit (a 0)
;[46] transmission bit (1= host to device, 0= device to host)
;[45:40] Command index
;[39:8] Argument (E.g. address)
;[7:1] CRC7
;[0] end bit


;/////////////////////SD STANDARD COMMANDS
;//No start bit (a 0), useful for generating clock cycles
SD_CMD_Blank db 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
;//Reset the card, also detects SPI mode if CS is low and is the first reset
SD_CMD0 db 0x40,0x00,0x00,0x00,0x00,0x95
;//High capacity mode select (off)
;SD_CMD1 db 0x41,0x00,0x00,0x00,0x00,0xF9
;//This says the voltage is 2.7-3.6v (0x01), Check pattern (0xAA), CRC and stop bit (0xFF) 
SD_CMD8 db 0x48,0x00,0x00,0x01,0xAA,0x87

;//Obtains the OCR register of the card
SD_CMD55 db 0x77,0x00,0x00,0x00,0x00,0x65

;//Obtains the OCR register of the card
SD_CMD58 db 0x7A,0x00,0x00,0x00,0x00,0xFD

;//Disable CRC mode
;SD_CMD59 db 0x7B,0x00,0x00,0x00,0x00,0x91

;//////////////////////APPLICATION COMMANDS
;//Lets the card know that HCS is available on the host 
SD_ACMD41 db 0x69,0x40,0x00,0x00,0x00,0x77



;///////////////////////////////////////////////////////
SD_load_buffer_FSR macro a,offset
	SD_load_buffer_H +HIGH(offset)
	movwf FSR#v(a)H
	movlw LOW(offset)
	movwf FSR#v(a)L
	endm

SD_load_buffer_end_FSR macro a
	SD_load_buffer_H +1
	movwf FSR#v(a)H
	movlw 0xFF
	clrf FSR#v(a)L
	endm

SD_blank_buffer
	SD_load_buffer_FSR 2,0x000

SD_blank_buffer_loop
	movlw .0
	movwf POSTINC2

	SD_load_buffer_H + .2
	xorwf FSR2H,W
	bnz	SD_blank_buffer_loop

	return

SD_blank_toggles
	rcall SD_blank_toggles_byte
	rcall SD_blank_toggles_byte
	rcall SD_blank_toggles_byte
	rcall SD_blank_toggles_byte
	rcall SD_blank_toggles_byte
	rcall SD_blank_toggles_byte
	rcall SD_blank_toggles_byte
	rcall SD_blank_toggles_byte
	rcall SD_blank_toggles_byte
	rcall SD_blank_toggles_byte
	rcall SD_blank_toggles_byte

	goto SD_blank_toggles_byte

SD_blank_toggles_byte
	LoadTable SD_CMD_Blank
	goto SD_Send_Cmd

;///////////////////////////////////////////////////
SD_init
	;Deselect the card
	bsf SD_CS

	;//Clear the SPI clock
	bsf SD_CLK
	bsf SD_DI

	;Default values for the SD modes
	bcf SD_ver2.0
	bcf SD_HC

	;Send some toggles to wake the card up (it needs alot!)
	call SD_blank_toggles
	call SD_blank_toggles

	;Send CMD0, this switches into SPI mode if the CS is low!
	;So set CS low
	bcf SD_CS
	;And send
	LoadTable SD_CMD0
	call SD_Send_Cmd

	call SD_Recv_R1
	;Check if it was successful
	xorlw 0x01
	btfss STATUS,Z
	retlw .1

	;More toggles to let the initialisation occur
	call SD_blank_toggles

	;Now send CMD8 to test if it's a v2.00+ SD card (possible HC)
	LoadTable SD_CMD8
	call SD_Send_Cmd

	;We might recieve a R7 or a R1 response, both begin with an R1 byte!
	call SD_Recv_R1
	;Check the R1 command didn't timeout
	btfsc WREG,7
	retlw .1

	;Check if CM8 was recognised
	btfsc WREG,2
	;It's a v1.0 card, carry on with the initialisation
	bra SD_init_CMD58_1

	;It's a v2.0 card!
	bsf SD_ver2.0
	
	;Now recv the rest of the R7 response
	;The following bytes have the command version and reserved bits in them
	movlw 0xff
	call SD_tranceive
	movlw 0xff
	call SD_tranceive
	movlw 0xff
	call SD_tranceive
	;WREG now contains 4 reserved upper bits and 4 bits on the voltage
	andlw 0x0F
	xorlw 0x01 ;/Check for the correct voltage value 	
	tstfsz WREG	
	retlw .1

	;Now for the test pattern (0xAA)
	movlw 0xff
	call SD_tranceive
	xorlw 0xAA
	tstfsz WREG
	retlw .1

	;Ok, we have a v2.0 card, lets rejoin the v1 initialisation

SD_init_CMD58_1 ;Here we're faking being cared about the card's supported voltages
	;More toggles, the card needs lots of response time
	call SD_blank_toggles
	call SD_blank_toggles

	;Get the OCR register
	LoadTable SD_CMD58
	call SD_Send_Cmd
	call SD_Recv_R1
	xorlw 0x01
	tstfsz WREG
	retlw .1

	;We should recieve an R3 response, first byte is R1, next 4 are the OCR. bits
	movlw 0xff
	call SD_tranceive
	;[24:29] are reserved,[30] is the card capacity,[31] is the power up status bit!
	movlw 0xff
	call SD_tranceive
	movlw 0xff
	call SD_tranceive
	movlw 0xff
	call SD_tranceive
	;[0:14] are reserved, [15:23] are voltage info (ignored)

SD_init_ACMD41
	call SD_blank_toggles
	call SD_blank_toggles

	LoadTable SD_CMD55
	call SD_Send_Cmd
	call SD_Recv_R1
	nop
	xorlw 0x01
	tstfsz WREG
	retlw .1

	call SD_blank_toggles
	call SD_blank_toggles

	LoadTable SD_ACMD41
	call SD_Send_Cmd
	call SD_Recv_R1
	nop
	
	;If the card returns 0 then it's initialised
	xorlw 0x00
	bz SD_init_CMD58_2

	;If card is in idle state then we need to repeat this command until it is done
	xorlw 0x01
	bz SD_init_ACMD41

	;Otherwise we failed
	retlw .1

SD_init_CMD58_2
	;If the card is not a v2.0 card we can skip this step
	btfss SD_ver2.0
	bra SD_init_end

	;More toggles, the card needs lots of response time
	call SD_blank_toggles
	call SD_blank_toggles

	;Get the OCR register
	LoadTable SD_CMD58
	call SD_Send_Cmd
	call SD_Recv_R1
	tstfsz WREG
	retlw .1

	;We should recieve an R3 response, first byte is R1, next 4 are the OCR. bits
	movlw 0xff
	call SD_tranceive
	nop
	;[24:29] are reserved,[30] is the card capacity,[31] is the power up status bit!
	
	;If the power up status bit is clear, something bad happened
	btfss WREG,7
	retlw .1

	;Now check for high capacity mode
	btfsc WREG,6
	bsf SD_HC

	;And receieve the remainder of the available bytes
	movlw 0xff
	call SD_tranceive
	nop
	movlw 0xff
	call SD_tranceive
	nop
	movlw 0xff
	call SD_tranceive
	nop
	;[0:14] are reserved, [15:23] are voltage info (ignored)
	
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
	;Falling edge
	bcf	SD_CLK

	;Set the line state
	bsf	SD_DI
	btfss STATUS,C
	bcf	SD_DI
	nop 
	nop
	;Rising edge
 	bsf	SD_CLK
	nop
	nop
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
;This converts the LBA address in SD_addr to the format 
;the SD_card's controller understands 
;while transmitting it to the controller
;For HC cards it's a 32 bit expansion of the block address
;For non-HC its multiplied by the block size (512) to convert to a byte address ( SD_addr << 9)
SD_transmit_addr
	btfss SD_HC
	bra SD_transmit_addr_LC

	movlw 0x00
	call SD_tranceive

	movf SD_addr+2,W
	call SD_tranceive

	movf SD_addr+1,W
	call SD_tranceive

	movf SD_addr+0,W
	call SD_tranceive

	bra SD_transmit_wrapup

SD_transmit_addr_LC
	;Transmit the address
	bcf STATUS,C
	rlcf SD_addr+2,W
	btfsc SD_addr+1,7
	bsf WREG,0
	call SD_tranceive

	bcf STATUS,C
	rlcf SD_addr+1,W
	btfsc SD_addr+0,7
	bsf WREG,0
	call SD_tranceive

	bcf STATUS,C
	rlcf SD_addr+0,W
	call SD_tranceive

	;Last byte is not used 
	movlw 0x00
	call SD_tranceive

SD_transmit_wrapup
	;The fake CRC
	movlw 0xFF
	call SD_tranceive

	;Wait for the confirmation byte
	call SD_Recv_R1
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

	;Transmit the address while converting to a half sector address
	call SD_transmit_addr
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
	SD_load_buffer_FSR 2,0x000

SD_read_copy_loop
	;This is purely for my oscilliscope, 
	;keep the line high so I can see its not transmitting
	movlw 0xFF

	rcall SD_tranceive
	movwf POSTINC2

	SD_load_buffer_H+2
	xorwf FSR2H,W
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

	;Transmit the address while converting to a half sector address
	call SD_transmit_addr
	xorlw 0x00
	btfss STATUS,Z
	retlw .1

	;Load the buffer
	SD_load_buffer_FSR 2,0x000

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

	SD_load_buffer_H+2
	xorwf FSR2H,W
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

;//////////////////////////////////////////////////////
;Write a blank sector into SD memory
SD_write_blank
	bsf SD_CS
	;Select the device
	bcf SD_CS

	;Start the command
	movlw 0xFF
	call SD_tranceive
	movlw 0x58
	call SD_tranceive

	;Transmit the address while converting to a half sector address
	call SD_transmit_addr
	xorlw 0x00
	btfss STATUS,Z
	retlw .1

	;Give the SD card a few cycles
	movlw 0xFF
	call SD_tranceive
	movlw 0xFF
	call SD_tranceive
	;Transmit the start byte
	movlw 0xFE
	call SD_tranceive

	clrf PRODL
	clrf PRODH
SD_write_blank_loop
	movlw 0x00
	rcall SD_tranceive

	infsnz PRODL,F
	incf PRODH,F

	movlw 0x02	
	xorwf PRODH,W
	bnz SD_write_blank_loop

	;Send dummy CRC
	movlw 0xFF
	call SD_tranceive
	movlw 0xFF
	call SD_tranceive

	clrf SPI_timeout_block
SD_write_blank_loop_wait
	call SD_tranceive
	
	;Check if the card has responded
	xorlw 0xE5
	btfsc STATUS,Z
	bra SD_write_blank_busy		

	;otherwise we must wait!
	;Try delaying a little, this can be a long process
	call delay_20ms

	decfsz SPI_timeout_block	
	bra SD_write_blank_loop_wait

	;Failed to recieve the confirmation
	retlw .2

SD_write_blank_busy
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
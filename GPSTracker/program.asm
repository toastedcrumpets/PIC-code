 include <p18f14k50.inc>

;Main Program entry point
 org 0x00
	goto init

 cblock 0x00
LEDTMRH,LEDTMRL,LEDTMRSCALER, FILE_L, FILE_H, NMEA_chars_written, WREG_LOWPR_SAVE, STATUS_LOWPR_SAVE, LOGCounter:2
 endc


 ;GPS pins
 #define GPS_RXA    PORTB,7
 #define GPS_TXA    PORTB,5
 #define GPS_ON_OFF PORTC,0
 ;Following pin is NC
 ;#define GPS_OnePPS PORTB,4
 #define GPS_GPIO1  PORTB,4
 #define GPS_GPIO6  PORTC,1

 ;LEDs
 #define LED    PORTA,5 ;Blue LED
 #define LED2   PORTA,4 ;Green LED

 ;SD Card
 #define SD_CS PORTC,7
 #define SD_DI PORTC,6
 #define SD_CLK PORTC,3
 #define SD_DO PORTC,4
 #define SD_load_buffer_H movlw 0x01

 ;High priority interrupts (communication)
 org 0x000008
	call NMEA_data_rec
	retfie FAST


 ;Low priority interrupt (LED ticker)
 ;Divides the tick rate by 8 before applying it to the LED
 ;This code is used everywhere, so it must be state safe! Save everything you touch
 org 0x000018
	movff WREG, WREG_LOWPR_SAVE
	movff STATUS, STATUS_LOWPR_SAVE

	movff LEDTMRH,TMR3H ;SAFE, does not affect any status reg's
	movff LEDTMRL,TMR3L ;SAFE, does not affect any status reg's

	bcf PIR2,TMR3IF ;SAFE, does not affect any status reg's
	movlw b'00000111' ;UNSAFE, changes WREG
	andwf LEDTMRSCALER,F

	dcfsnz LEDTMRSCALER,F
	btg LED

	movff WREG_LOWPR_SAVE, WREG
	movff STATUS_LOWPR_SAVE, STATUS
	retfie

 include "macros.inc"
 include "SDCard.inc"
 include "24bitMacros.inc"
 include "FAT32.inc"
 include "NMEA.inc"

;Delays are on timer0, so the LED timer must be on TMR3
SET_LED_FLASH_RATE macro _count
	movlw	b'10110001'
	movwf	T3CON

	movlw	HIGH(0xffff-_count)
	movwf	LEDTMRH
	movlw	LOW(0xffff-_count)
	movwf	LEDTMRL

	bsf PIE2,TMR3IE
	endm

 #define OSC_SPEED .16000000
 ;4800 baud
 #define BRGVAL .833
; 57.6K
; #define BRGVAL .68

; Slower clock for monitoring the card
; #define OSC_SPEED .4000000
; ;4800 baud
; #define BRGVAL .208


 #define INSTR_SPEED (OSC_SPEED/.4)
 ;The 128 comes from the software divider (/8) and the hardware divider (/8)
 #define TMR3_SPEED (INSTR_SPEED/.64)

 ;Each interrupt is one half of a cycle
 ;Looking for a fix
 #define LED_FLASH_0.5hz SET_LED_FLASH_RATE (TMR3_SPEED)
 ;SD card error
 #define LED_FLASH_5hz   SET_LED_FLASH_RATE (TMR3_SPEED/.10)
 ;FAT error
 #define LED_FLASH_20hz  SET_LED_FLASH_RATE (TMR3_SPEED/.40)

 include "SDWriter.inc"

logfile db "GPSLOG  TXT"

init
	;//////////////////////////GPS init
	;Set the GPS boot flags right away
	bcf 0x12+GPS_GPIO6
	bcf GPS_GPIO6

	;Make this an input straight away
	bsf 0x12+GPS_GPIO1

	;//////////Analogue inputs disabled
	clrf ANSEL
	clrf ANSELH

	;Pull the On_off pin low, ready to pulse it
	bcf GPS_ON_OFF
	bcf 0x12+GPS_ON_OFF

	;Set the TRIS's for the inputs 
	bsf 0x12+GPS_TXA
	
	;/////////////////LED's Init
	bcf 0x12+LED
	bcf LED
	bcf 0x12+LED2
	bcf LED2
	;///Setup the oscillator
	movlw b'01110000'
	movwf OSCCON

	;//Let the Oscillator stabilise
	btfss OSCCON,IOFS
	bra $-.2

	;///////////////PORT SLEW RATE CONTROL (Reduce high freq noise)
	movlw b'00000111'
	movwf SLRCON

	;/////////////////Interrupts
	;Enable interrupt priorities
	bsf RCON,IPEN
	;Enable high priority interrupts (communication)
	bsf INTCON,GIEH
	;Enable low priority interrupts for now (LED timing)
	bsf INTCON,GIEL

	;//////////////////LED
	;Set the priority to low
	bcf IPR2,TMR3IP
	;Disable the LED timer
	bcf PIE2,TMR3IE

	;/////////////////SD card
	;Set the port directions
	bcf 0x12+SD_DI
	bcf 0x12+SD_CLK
	bsf 0x12+SD_DO 
	bcf 0x12+SD_CS

	LED_FLASH_20hz
soft_reset
	call SD_init
	TSTFSZ WREG
	bra soft_reset

	LED_FLASH_5hz

init_filesystem
	call FAT_init
	TSTFSZ WREG
	bra soft_reset

	;File name
	movlw '0'
	movwf entryname
	movwf entryname+.1
	movwf entryname+.2
	movwf entryname+.3
	movwf entryname+.4
	movwf entryname+.5
	movwf entryname+.6
	movwf entryname+.7

	;File extension
	movlw 'L'
	movwf entryname+.8
	movlw 'O'
	movwf entryname+.9
	movlw 'G'
	movwf entryname+.10

open_logfile
	movlw .1
	addwf entryname+7,F
	movlw ':'
	xorwf entryname+7,W
	bnz open_logfile_next	

	movlw '0'
	movwf entryname+7
	movlw .1
	addwf entryname+6,f
	movlw ':'
	xorwf entryname+6,W
	bnz open_logfile_next

	movlw '0'
	movwf entryname+6
	movlw .1
	addwf entryname+5,F

open_logfile_next
	call FAT_load_root

	;Try to open the file
	call FAT_open_entry
	xorlw .0
	bz open_logfile ;If it succeded, then loop to open a different file!

	;File Failed to open, create it!
	;Set the entry as a file
	clrf FAT_newfile_attrib

	movlw .1
	movwf FAT_newfile_size

	call FAT_new_entry
	xorlw .0
	bnz soft_reset

	;Now open the logfile and blank the first sector
	call FAT_open_entry
	xorlw .0
	bnz soft_reset

	call convert_cluster_in_addr
	SD_load_buffer_FSR 2,0x000
	call SD_blank_buffer
	
	call SD_write
	xorlw .0
	bnz soft_reset

	;Reopen the log file at the start
	call FAT_open_entry
	xorlw .0
	bnz soft_reset

	;So now we have a file open and we should load it and seek to its end
	call convert_cluster_in_addr
	call SD_init_seek

init_UART
	;Ready the NMEA recieve enable 
	call NMEA_init

	;Establish the UART
	bsf TRISB,5
	bcf TRISB,7
	;Switch on interrupts for USART
	bsf PIE1,RCIE
	bsf IPR1,RCIP ;High priority
	bsf TXSTA,TXEN
	bcf TXSTA,SYNC
	bsf RCSTA,SPEN

	;Fixed Baud
	bsf TXSTA,BRGH
	bsf BAUDCON,BRG16

	movlw HIGH(BRGVAL)
	movwf SPBRGH
	movlw LOW(BRGVAL)
	movwf SPBRG

	bsf  RCSTA,CREN

	;Stop the whole flashing thing
	bcf PIE2,TMR3IE
	bcf LED ;Set the blue fix light to off
	bsf LED2
	
main_restart
	bsf NMEA_RecvEnable
	call NMEA_reset_recv
	bcf NMEA_new_fix_data_flag
main
	;Here, we try to write any recieved fix to the SD card.
	btfsc NMEA_RecvEnable
	bra main

	btfss NMEA_new_fix_data_flag
	bra main_restart

	;We have a valid fix! write it to the SD card
	clrf NMEA_chars_written
sentance_loop
	;save the SD buffer location
	movff FSR0L, FILE_L
	movff FSR0H, FILE_H

	movlw LOW(NMEA_sentence)
	movwf FSR0L
	movlw HIGH(NMEA_sentence)
	movwf FSR0H

	movf NMEA_chars_written,w
	movf PLUSW0,W
	incf NMEA_chars_written,f

	movff FILE_L,FSR0L
	movff FILE_H,FSR0H

	call SD_write_char
	TSTFSZ WREG
	bra soft_reset

	decfsz Sentence_Bytes_Rec
	bra sentance_loop

	;Write the checksum
	movlw '*'
	call SD_write_char
	TSTFSZ WREG
	bra soft_reset

	movf NMEA_recv_checksum_H,w
	call SD_write_char
	TSTFSZ WREG
	bra soft_reset

	movf NMEA_recv_checksum_L,w
	call SD_write_char
	TSTFSZ WREG
	bra soft_reset

	;Write a newline
	movlw '\n'
	call SD_write_char
	TSTFSZ WREG
	bra soft_reset

	bra main_restart

 end
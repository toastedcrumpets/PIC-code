;Main Program entry point
 org 0x00
	goto init

 cblock 0x00
LEDTMRH,LEDTMRL
 endc

 ;GPS pins
 #define GPS_RXA    PORTC,2
 #define GPS_TXA    PORTB,5
 #define GPS_ON_OFF PORTC,0
 #define GPS_OnePPS PORTB,4
 #define GPS_GPIO1  PORTB,6
 #define GPS_GPIO6  PORTC,1

 ;LEDs
 #define LED    PORTA,5

 ;SD Card
 #define SD_CS PORTC,5
 #define SD_DI PORTC,4
 #define SD_CLK PORTC,6
 #define SD_DO PORTC,7
 #define SD_load_buffer_H movlw 0x01

 ;High priority interrupts (communication)
 org 0x000008
	movff RCREG,WREG
	
	retfie FAST


 ;Low priority interrupt (ticker for DAC)
 org 0x000018
	btg LED
	bcf PIR2,TMR3IF
	movff LEDTMRH,TMR3H
	movff LEDTMRL,TMR3L
	retfie

 include <p18f14k50.inc>
 include "macros.inc"
 include "SDCard.inc"
 include "24bitMacros.inc"
 include "FAT32.inc"
 include "NMEA.inc"

SET_LED_FLASH_RATE macro _prescaler, _count
	movlw	_prescaler
	movwf	T3CON

	movlw	HIGH(0xffff-_count)
	movwf	LEDTMRH
	movlw	LOW(0xffff-_count)
	movwf	LEDTMRL
	endm

 #define LED_FLASH_0.5hz SET_LED_FLASH_RATE b'10111101', .31250
 #define LED_FLASH_5hz   SET_LED_FLASH_RATE b'10111101', .3125
 #define LED_FLASH_20hz   SET_LED_FLASH_RATE b'10111101', .781

logfile db "GPSLOG  TXT"

init
	;//////////////////////////GPS init
	;Set the GPS boot flags right away
	bcf 0x12+GPS_GPIO6
	bcf GPS_GPIO6

	;Make this high straight away
	bsf 0x12+GPS_GPIO1

	;//////////Analogue inputs disabled
	clrf ANSEL
	clrf ANSELH

	;Pull the On_off pin low, ready to pulse it
	bcf GPS_ON_OFF
	bcf 0x12+GPS_ON_OFF

	;Setup the serial line, high for idle state
	bcf 0x12+GPS_RXA
	bsf GPS_RXA

	;Set the TRIS's for the inputs 
	bsf 0x12+GPS_OnePPS
	bsf 0x12+GPS_TXA

	;/////////////////LED
	bcf 0x12+LED
	bcf LED

	;/////////////////Interrupts
	;Enable interrupt priorities
	bsf RCON,IPEN
	;Enable high priority interrupts (communication)
	bsf INTCON,GIEH
	;Enable low priority interrupts for now (LED timing)
	bsf INTCON,GIEL

	;//////////////////LED
	LED_FLASH_0.5hz

	;Set the priority to low
	bcf IPR2,TMR3IP
	;Enable the LED timer
	bsf PIE2,TMR3IE

	;/////////////////SD card
	;Set the port directions
	bcf 0x12+SD_DI
	bcf 0x12+SD_CLK
	bsf 0x12+SD_DO
	bcf 0x12+SD_CS

	call SD_init
	xorlw 0x00
	bz init_filesystem

	;Try again
	call SD_init
	TSTFSZ WREG
	bra SD_fail

init_filesystem
	call FAT_init
	TSTFSZ WREG
	bra SD_fail
	
	call FAT_load_root

	LoadTable logfile
	call FAT_load_filename_TBLPTR

	call FAT_delete_entry
	;Dont care if it succeded or not, the file should be gone

	LoadTable logfile
	call FAT_load_filename_TBLPTR
	;Set the entry as a file
	clrf FAT_newfile_attrib

	movlw .2
	movwf FAT_newfile_size

	call FAT_new_entry
	xorlw .0
	bnz FAT_fail

init_UART
	call NMEA_init

main
	bra main

SD_fail
	LED_FLASH_5hz
	bra $

FAT_fail
	LED_FLASH_20hz
	bra $

 end
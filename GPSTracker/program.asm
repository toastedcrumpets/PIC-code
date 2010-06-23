 include <p18f14k50.inc>

;Main Program entry point
 org 0x00
	goto init

 cblock 0x00
LEDTMRH,LEDTMRL,LEDTMRSCALER
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
	call SD_write_char
	retfie FAST


 ;Low priority interrupt (LED ticker)
 ;Divides the tick rate by 8 before applying it to the LED
 org 0x000018
	movff LEDTMRH,TMR3H
	movff LEDTMRL,TMR3L

	bcf PIR2,TMR3IF
	movlw b'00000111'
	andwf LEDTMRSCALER,F

	dcfsnz LEDTMRSCALER,F
	btg LED

	retfie

 include "macros.inc"
 include "SDCard.inc"
 include "24bitMacros.inc"
 include "FAT32.inc"
; include "NMEA.inc"

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

 ;#define OSC_SPEED .1000000
 ;#define BRGVAL .50

 #define OSC_SPEED .16000000
 ;4800 baud
 #define BRGVAL .833
 ;57.6K
 ;#define BRGVAL .68

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
	
open_logfile
	call FAT_load_root

	LoadTable logfile
	call FAT_load_filename_TBLPTR
	call FAT_open_entry
	xorlw .0
	bz Log_File_Open

	;File Failed to open, must create it I guess
	LoadTable logfile
	call FAT_load_filename_TBLPTR
	;Set the entry as a file
	clrf FAT_newfile_attrib

	movlw .1
	movwf FAT_newfile_size

	call FAT_new_entry
	xorlw .0
	bnz FAT_fail

	;Now open the logfile and blank the first sector
	LoadTable logfile
	call FAT_load_filename_TBLPTR
	call FAT_open_entry
	xorlw .0
	bnz SD_fail

	call convert_cluster_in_addr
	SD_load_buffer_FSR 2,0x000
	call SD_blank_buffer
	
	call SD_write
	xorlw .0
	bnz SD_fail

	bra open_logfile

Log_File_Open
	;So now we have a file open and we should load it and seek to its end
	call convert_cluster_in_addr
	call SD_init_seek

init_UART
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

	bcf LED

	bsf LED2
main
	;TRY NOT TO DO ANYTHING HERE, OR FIX THE TIMER INTERRUPT (IT CORRUPTS WREG&STATUS)
	bra main

SD_fail
	LED_FLASH_5hz
	bsf LED2
	bra $

FAT_fail
	LED_FLASH_20hz
	bra $

 end
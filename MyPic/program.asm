;*****************************************************************************************
;*****************************************************************************************

 include	<p18f1320.inc>

 ; Global Variable declarations
 cblock 0x0000
NULL,w_temp
BitVar,ByteVar
  endc

 ; Reset Entry Point
 org 0x0000
  	goto INIT

 ; High Priority Interrupt Vector
 org 0x0008
	call NMEA_data_rec
	retfie FAST

 ; Low Priority Interrupt Vector
 org 0x0018
	retfie FAST

 ; Include Files
 ; Including the general macros
 include    "macros.inc"

 ; EA DOG LCD driver
 #define LCD_RS  PORTA,0
 #define LCD_SCL PORTA,1
 #define LCD_SI  PORTA,2
 #define LCD_3L5V
 include "LCD.inc"

 ; The menu driver
 include "MenuDriver.inc"

 ; The menu navigation driver
 #define Menu_btnUp PORTB,5
 #define Menu_btnDwn PORTB,0
 #define Menu_btnSel PORTA,4
 include "MenuDriver3Button.inc"

 ; The NMEA parser and serial driver
 include "NMEA.inc"

 include "NMEAMenu.inc"

INIT
	;Establish the timer
	movlw b'01100010'
	movwf OSCCON
INIT_CLK_LOCK
	btfss OSCCON,2
	bra INIT_CLK_LOCK

	;Sleep a little just incase you're being reprogrammed
	call delay_40ms
	;Also for the LCD display to boot
	call delay_40ms
	call delay_40ms
	call delay_40ms
	; Clear and Setup Outputs
	clrf TRISA
	clrf TRISB
	clrf PORTA
	clrf PORTB

	; Set all digital ports
	movlw b'11111111'
	movwf ADCON1

	; Establish inputs
	; Set a weak pull up on the PortB
	bcf INTCON2,NOT_RBPU

	; Inputs on PortB:0
	movlw b'00100001'
	movwf TRISB

	; Inputs on PortA:4,3
	movlw b'00011000'
	movwf TRISA

	call NMEA_init
	call LCD_INIT





	Load_Menu Main_Menu
	call Menu_Open
MAIN_LOOP
	call Menu_Ticker


	bra MAIN_LOOP

;Some functions for the menu to call
Return_Vector return


;*******************************************
;Main menu
 AddMenuItem "PIC Nav",Menu_Navigation_Init, Menu_Navigation
 db UnpackPLocation(Return_Vector)
 AddMenuItem "Navigate Mode",NMEA_Menu_Waypoint_init, NMEA_Menu_Waypoint
 db UnpackPLocation(Return_Vector)
 AddMenuItem "Fix:",NMEA_Menu_Fix_init, NMEA_Menu_Fix
 db UnpackPLocation(Return_Vector)
Main_Menu endCurrentMenu

 END
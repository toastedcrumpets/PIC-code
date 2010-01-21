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
	reset

 ; Low Priority Interrupt Vector
 org 0x0018
	reset

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

INIT
	;Sleep a little just incase you're being reprogrammed
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


INITLOOP
	call LCD_INIT
	Load_Menu Main_Menu
	call Menu_Open
MAIN_LOOP
	call Menu_Ticker
	bra MAIN_LOOP

;Some functions for the menu to call
Return_Vector return

Hello_World data "Hello, World!",0x00
Hello_World_Function
	call LCD_line_2
	LCDPrintString Hello_World	
	return

;*******************************************
;Main menu
 AddMenuItem "The hello world",Menu_Navigation_Init, Menu_Navigation
 db UnpackPLocation(Hello_World)

 AddMenuItem "A blank item",Menu_Navigation_Init,Menu_Navigation
 db UnpackPLocation(Return_Vector)

 AddMenuItem "Sub Menu",Menu_Navigation_Init,Menu_Navigation
 db UnpackPLocation(Menu_load_from_table),UnpackPLocation(Sub_Menu)

Main_Menu endCurrentMenu

;********************************************
;Sub menu
 AddMenuItem "A blank submenu",Menu_Navigation_Init,Menu_Navigation
 db UnpackPLocation(Return_Vector)

 AddMenuItem "Back",Menu_Navigation_Init,Menu_Navigation
 db UnpackPLocation(Menu_load_from_table),UnpackPLocation(Main_Menu)

Sub_Menu endCurrentMenu

 END
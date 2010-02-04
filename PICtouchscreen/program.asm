 PROCESSOR 18F4680
 CONFIG  OSC=HSPLL,FCMEN=OFF,IESO=OFF,PWRT=OFF,BOREN=OFF,BORV=2,WDT=OFF,WDTPS=32768,MCLRE=ON,LPT1OSC=OFF,PBADEN=OFF,XINST=OFF,LVP=OFF,STVREN=ON
 ;OSC=IRCIO67

 org 0x0000
	goto INIT

 cblock
w_temp
 endc

 include <p18f4680.inc>
 include "macros.inc"
 include "divide.inc"



 ;LCD display
 #define LCD_RS  LATB,2
 #define LCD_SCL LATB,3
 #define LCD_SI  LATB,4
 #define LCD_grfx
 include "grfxLCD.inc"

 ;Touch screen

 include "touchPanel.inc"
 include "framebuffer.inc"
 include "textBlit.inc"
 include "touchmenu.inc"

 cblock
sense_ctr
 endc

INIT
	;Setup the profiling pins
;	clrf TRISD

;BENCHPIN
;	btg LATD,1
;	bra BENCHPIN

	;//////////////////////////////////////
	;Setup the display
	;Set portB to output
	clrf TRISB

	;Pull down B5 to reset the display
	clrf LATB
	call delay_1ms

	;Enable the display
	bsf LATB,5
	call delay_1ms
	call LCD_INIT

	call LCD_clear

	;//////////////////////////////////////
	;Setup the touch panel
	call touch_init
	;Entry vector for the menu
	call Drawing_Mode_Init
MAIN
	call Menu_ticker
	bra MAIN

 include "DrawMode.inc"

 include "chartable.inc"
 end
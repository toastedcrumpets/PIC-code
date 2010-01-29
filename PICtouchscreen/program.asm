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

 cblock
sense_ctr
 endc

Program CODE

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
	call blank_framebuffer


	
MAIN
	call transmit_framebuffer_fast
	call touch_read_state
	call fast_normalise_coords


	btfss touched,0
	bra MAIN

	movff x_pos,pixel_x
	bcf STATUS,C
	rrcf pixel_x

	movff y_pos,pixel_y
	bcf STATUS,C
	rrcf pixel_y
	bcf STATUS,C
	rrcf pixel_y

	call fb_set_pixel

	bra MAIN

 cblock
bar_counter, bar_val, bar_piece
 endc


 end
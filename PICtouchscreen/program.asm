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

 cblock
sense_ctr
 endc
INIT
	;Start by setting the Int. oscillator to 4Mhz
	movlw b'01110010'
	movwf OSCCON

	;enable the PLL
	bsf OSCTUNE,6
	
	;Ensure the oscillator is stable before proceeding
INIT_STABLE_OSC
	btfss OSCCON,IOFS
	bra INIT_STABLE_OSC

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
	call transmit_framebuffer

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
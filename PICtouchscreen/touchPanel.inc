;EA DOG touch panel driver
 #define LCD_XP_TRIS TRISA,0  
 #define LCD_XN_TRIS TRISA,1  
 #define LCD_YP_TRIS TRISA,2  
 #define LCD_YN_TRIS TRISA,5

 #define LCD_XP_LAT LATA,0  
 #define LCD_XN_LAT LATA,1  
 #define LCD_YP_LAT LATA,2
 #define LCD_YN_LAT LATA,5

 ;When moved into ADCON0, the AD conversion is started
 #define LCD_XN_ANBITS b'00000101'
 #define LCD_XP_ANBITS b'00000001'

 #define LCD_YN_ANBITS b'00010001'
 #define LCD_YP_ANBITS b'00001001'

 cblock
x_min:2,x_range:2
y_min:2,y_range:2
x_pos:2, y_pos:2, touched
 endc

;////////////////////////////////////////////
touch_wait_for_touch
	call touch_check_touch
	btfss touched,0
	bra touch_wait_for_touch
	return
;////////////////////////////////////////////
touch_wait_for_release
	call touch_check_touch
	btfsc touched,0
	bra touch_wait_for_release
	return
;////////////////////////////////////////////
touch_init
	clrf touched
	call touch_reset_pins

	;Setup the ports, AN4-0
	movlw b'00001010'
	movwf ADCON1
	
	;Configure the aquisition & calc time
	movlw b'10000001'
	movwf ADCON2

	;Switch it on
	bsf ADCON0,GO_DONE

	;//////////////////////////////
	;Calibrate the screen

	;Arrow in the bottom left corner
	movlw .0
	call LCD_goto_column
	call LCD_goto_row
	movlw b'00000111'
	call LCD_data_send
	movlw b'00000001'
	call LCD_data_send
	movlw b'00000001'
	call LCD_data_send

	;Wait for release
	call touch_wait_for_release
touch_calibrate_step_1
	;Wait for touch
	call touch_wait_for_touch

	;Read the state
	call touch_read_state
	;Abort if contact was lost
	btfss touched,0
	bra touch_calibrate_step_1

	;Blank the arrow
	call LCD_clear

	;Save the minima
	movff x_pos,x_min
	movff y_pos,y_min
	movff x_pos+1,x_min+1
	movff y_pos+1,y_min+1

	;Wait for release
	call touch_wait_for_release

	;Draw another cross in the top right
	movlw .125
	call LCD_goto_column
	movlw .7
	call LCD_goto_row
	movlw b'10000000'
	call LCD_data_send
	movlw b'10000000'
	call LCD_data_send
	movlw b'11100000'
	call LCD_data_send

touch_calibrate_step_2
	;Wait for touch
	call touch_wait_for_touch

	;Read the state
	call touch_read_state
	;Abort if contact was lost
	btfss touched,0
	bra touch_calibrate_step_2

	;Blank the arrow
	call LCD_clear

	;Save the max
	movff x_pos,x_range
	movff y_pos,y_range
	movff x_pos+1,x_range+1
	movff y_pos+1,y_range+1

	;Generate the range
	movf x_min,W
	subwf x_range,F
	movf x_min+1,W
	subwfb x_range+1,F

	btfss STATUS,C
	bra touch_init

	movf y_min,W
	subwf y_range,F
	movf y_min+1,W
	subwfb y_range+1,F

	;Check for overflow, recalibrate if there's a problem
	btfss STATUS,C
	bra touch_init

	return

;//////////////////////////////////////////////////
;Reset the panel so there is no current draw
touch_reset_pins
	bsf LCD_XP_TRIS
	bsf LCD_XN_TRIS
	bsf LCD_YP_TRIS
	bsf LCD_YN_TRIS
	return

;//////////////////////////////////////////////////
;Check if the display was touched
touch_check_touch
	bsf touched,0
	;Set the X plates to charge +ve
	bsf LCD_XP_LAT
	bsf LCD_XN_LAT
	bcf LCD_XP_TRIS
	bcf LCD_XN_TRIS

	;Ground one Y pin 
	bcf LCD_YP_LAT
	bcf LCD_YP_TRIS

	;Place the other in the AD convertor
	bsf LCD_YN_TRIS

	;Set the analog input
	movlw LCD_YN_ANBITS
	movwf ADCON0

	;Aquire a reading
	call touch_get_reading

	;Check if it is non-zero in either set of bits
	movf ADRESH,W
	iorwf ADRESL,W
	bz touch_check_touch_fail

	;Re-charge the plate
	bcf LCD_XP_TRIS
	bcf LCD_XN_TRIS

	;Set the other pin to input	
	bsf LCD_YP_TRIS

	;Ground the other pin
	bcf LCD_YN_LAT
	bcf LCD_YN_TRIS

	movlw LCD_YP_ANBITS
	movwf ADCON0

	;Get a reading 
	call touch_get_reading
	movf ADRESH,W
	iorwf ADRESL,W
	bz touch_check_touch_fail

	return
touch_check_touch_fail
	bcf touched,0
	return

;//////////////////////////////////////////////////
 cblock
tmp_analog_reading
 endc
touch_get_reading
	;Wait on AD convertor
	bsf ADCON0,GO_DONE
touch_get_reading_loop
	btfsc ADCON0,GO_DONE
	bra touch_get_reading_loop
	;Turn off the touch panel
	call touch_reset_pins
	return

;//////////////////////////////////////////////////
touch_read_x
	call touch_check_touch
	bz touch_read_x_fail

	;Set the X plates to charge -ve to +ve
	bsf LCD_XP_LAT
	bcf LCD_XN_LAT
	bcf LCD_XP_TRIS
	bcf LCD_XN_TRIS

	;Set both Y pins to input
	bsf LCD_YN_TRIS
	bsf LCD_YP_TRIS

	;Set the analog input
	movlw LCD_YN_ANBITS
	movwf ADCON0

	;wait for aquisition to finish
	call touch_get_reading
	movff ADRESH, x_pos+1
	movff ADRESL, x_pos
	;movwf tmp_analog_reading
	
	;Establish the reverse gradient
	bsf LCD_XN_LAT
	bcf LCD_XP_LAT
	bcf LCD_XN_TRIS
	bcf LCD_XP_TRIS

	;Set the analog input
	movlw LCD_YP_ANBITS
	movwf ADCON0
	call touch_get_reading
	movf ADRESL
	addwf x_pos,F
	movf ADRESH
	addwfc x_pos+1,F
	rrcf x_pos+1,F
	rrcf x_pos,F

touch_read_x_fail
	return

;//////////////////////////////////////////////////
touch_read_y
	call touch_check_touch
	bz touch_read_y_fail

	;Set the Y plates to charge -ve to +ve
	bsf LCD_YP_LAT
	bcf LCD_YN_LAT
	bcf LCD_YP_TRIS
	bcf LCD_YN_TRIS

	;Set both Y pins to input
	bsf LCD_XN_TRIS
	bsf LCD_XP_TRIS

	;Set the analog input
	movlw LCD_XN_ANBITS
	movwf ADCON0

	;wait for aquisition to finish
	call touch_get_reading
	movff ADRESH, y_pos+1
	movff ADRESL, y_pos
	;movwf tmp_analog_reading
	
	;Establish the reverse gradient
	bsf LCD_YN_LAT
	bcf LCD_YP_LAT
	bcf LCD_YN_TRIS
	bcf LCD_YP_TRIS

	;Set the analog input
	movlw LCD_XP_ANBITS
	movwf ADCON0
	call touch_get_reading
	movf ADRESL
	addwf y_pos,F
	movf ADRESH
	addwfc y_pos+1,F
	rrcf y_pos+1,F
	rrcf y_pos,F
touch_read_y_fail
	return

;//////////////////////////////////////////////////
touch_read_state
	call touch_read_y
	btfss touched,0
	bra	not_touched

	call touch_read_x
	btfss touched,0
	bra	not_touched

	return
not_touched
	clrf x_pos
	clrf y_pos
	clrf x_pos+1
	clrf y_pos+1
	return

fast_normalise_coords
	;This changes the coords to 0-256 and checks the bounds
	btfss touched,0
	return

	;Check they're large enough
	movf x_min,W
	subwf x_pos,F
	movf x_min+1,W
	subwfb x_pos+1,F
	
	btfsc STATUS,N
	clrf x_pos

	movf y_min,W
	subwf y_pos,F
	movf y_min+1,W
	subwfb y_pos+1,F
	
	btfsc STATUS,N
	clrf y_pos

	;Now multiply by 255 and divide by the range
	;X first
	clrf NROA_H
	movlw .255
	movwf NROA_L
	movff x_pos,NROB_L
	movff x_pos+1,NROB_H
	call MUL_1616U
	
	movff x_range,DSOR_0
	movff x_range+1,DSOR_1
	call DIV_3216U_KEN
	movf QUOT_L,W
	tstfsz QUOT_H
	movlw .255
	movwf x_pos

	;Now Y
	clrf NROA_H
	movlw .255
	movwf NROA_L
	movff y_pos,NROB_L
	movff y_pos+1,NROB_H
	call MUL_1616U
	
	movff y_range,DSOR_0
	movff y_range+1,DSOR_1
	call DIV_3216U_KEN
	movf QUOT_L,W
	tstfsz QUOT_H
	movlw .255
	movwf y_pos
	return
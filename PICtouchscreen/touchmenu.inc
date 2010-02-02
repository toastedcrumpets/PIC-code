;TouchMenu

 __badram 0x0600-0x07FF

;//////////////////////////////////
Insert_Touch_value macro x, y, function
	local Val = x+y*16
	call Load_TouchMenu
	movlw #v(Val & H'FF')
	addwf FSR1L,F
	movlw #v(Val >>D'8' & H'FF')
	addwfc FSR1H,F
	movlw UPPER(function)
	movwf POSTINC1
	movlw HIGH(function)
	movwf POSTINC1
	movlw LOW(function)
	movwf POSTINC1
	endm

;///////////////////////////
;Just preps FSR1 for the touch table
Load_TouchMenu
	movlw 0x06
	movwf FSR1H
	clrf FSR1L
	return
;///////////////////////////
;Wipes the touch menu to no action
 cblock
clear_TouchMenu_loop_ctr
 endc

clear_TouchMenu
	rcall Load_TouchMenu
	movlw .128
	movwf clear_TouchMenu_loop_ctr

clear_TouchMenu_loop
	movlw UPPER(clear_TouchMenu_return_vec)
	movwf POSTINC1
	movlw HIGH(clear_TouchMenu_return_vec)
	movwf POSTINC1
	movlw LOW(clear_TouchMenu_return_vec)
	movwf POSTINC1
	
	decfsz clear_TouchMenu_loop_ctr
	bra clear_TouchMenu_loop

clear_TouchMenu_return_vec
	return

;///////////////////////////
;This executes the correct function for the normalised location
 cblock
tmp1
 endc

do_TouchMenu
	rcall Load_TouchMenu

	movf x_pos,W
	bcf STATUS,C
	rrcf WREG
	bcf STATUS,C
	rrcf WREG
	bcf STATUS,C
	rrcf WREG
	bcf STATUS,C
	rrcf WREG
	movwf tmp1

	movf y_pos,W
	bcf STATUS,C
	rrcf WREG

	andlw b'1110000'	
	addwf tmp1,W

	mullw .3
	movf PRODL,W
	addwf FSR1L,F

	movf PRODH,W
	addwfc FSR1H,F

	;Perform the jump
	movff POSTINC1,PCLATU
	movff POSTINC1,PCLATH
	movf POSTINC1,W
	movwf PCL

	return

Draw_Mode
	movff x_pos,pixel_x
	bcf STATUS,C
	rrcf pixel_x

	movff y_pos,pixel_y
	bcf STATUS,C
	rrcf pixel_y
	bcf STATUS,C
	rrcf pixel_y

	call fb_set_pixel
	return
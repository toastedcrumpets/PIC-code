;TouchMenu

 __badram 0x500-0x6FF

;///////////////////////////
;Just preps FSR1 for the touch table
Load_TouchMenu
	movlw 0x05
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
	movlw LOW(clear_TouchMenu_return_vec)
	movwf POSTINC1
	movlw HIGH(clear_TouchMenu_return_vec)
	movwf POSTINC1
	movlw UPPER(clear_TouchMenu_return_vec)
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

	andlw b'1111000'	
	addwf tmp1,W

	addlw .0

	return
;TouchMenu

 __badram 0x0600-0x07FF


;//////////////////////////////////
;Loads the table ptr address into the touch overlay in a row N times
 cblock
Insert_touch_ctr
 endc

Insert_Touch_value_N macro function, N
	movlw N
	movwf Insert_touch_ctr

	Insert_Touch_value function
	decfsz Insert_touch_ctr
	bra $-.14
	endm

;//////////////////////////////////
;Loads the table ptr address into the touch overlay
;function is the label of the function
Insert_Touch_value macro function
	movlw UPPER(function)
	movwf POSTINC1
	movlw HIGH(function)
	movwf POSTINC1
	movlw LOW(function)
	movwf POSTINC1
	endm
;////////////////////////////////
;Goes to a spot in the touch overlay
;x pos (0-15), y_pos (0-7)
 cblock
Insert_Touch_var_x,Insert_Touch_var_y
 endc
Touch_Goto_Entry
	call Load_TouchMenu

	;Calculate the overlay offset
	movf Insert_Touch_var_y,W
	mullw .16
	;Value is 0-7 so only low byte needed
	movf PRODL,W
	addwf Insert_Touch_var_x,W
	mullw .3

	;Goto the overlay location
	movf PRODL,W
	addwf FSR1L,F
	movf PRODH,W
	addwfc FSR1H,F
	return

;///////////////////////////
;Just preps FSR1 for the touch table
Load_TouchMenu
	movlw 0x06
	movwf FSR1H
	clrf FSR1L
	return
;//////////////////////////////////
;Fills the touch menu to one action
 cblock
fill_TouchMenu_loop_ctr, fill_touch_menu_vec_L,fill_touch_menu_vec_H,fill_touch_menu_vec_U
 endc

fill_TouchMenu
	rcall Load_TouchMenu
	movlw .128
	movwf fill_TouchMenu_loop_ctr

fill_TouchMenu_loop
	movf fill_touch_menu_vec_U,W
	movwf POSTINC1
	movf fill_touch_menu_vec_H,W
	movwf POSTINC1
	movf fill_touch_menu_vec_L,W
	movwf POSTINC1
	
	decfsz fill_TouchMenu_loop_ctr
	bra fill_TouchMenu_loop

clear_TouchMenu_ret_vec
	return

;/////////////////////////////////////////
;Sets the touch screen to no action
clear_TouchMenu
	movlw LOW(clear_TouchMenu_ret_vec)
	movwf fill_touch_menu_vec_L
	movlw HIGH(clear_TouchMenu_ret_vec)
	movwf fill_touch_menu_vec_H
	movlw UPPER(clear_TouchMenu_ret_vec)
	movwf fill_touch_menu_vec_U
	goto fill_TouchMenu

;////////////////////////////////////////
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


;///////////////////////////////////////////////////////////
;Menu loop ticker, handles touches and updates of the screen
Menu_ticker
	call transmit_framebuffer_fast
	call touch_read_state
	call fast_normalise_coords

	btfss touched,0
	return

	call do_TouchMenu
	return


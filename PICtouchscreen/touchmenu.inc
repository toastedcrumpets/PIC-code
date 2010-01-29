;TouchMenu

 __badram 0x500-0x6FF

;///////////////////////////
Load_TouchMenu
	movlw 0x05
	movwf FSR1H
	clrf FSR1L
	return
;///////////////////////////
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

;///
;Scraps
	;Print X bar
	;movlw .0
	;call LCD_goto_row
	;movff x_pos_norm,bar_val
	;bcf STATUS,C
	;rrcf bar_val
	;call print_bar

	;Print Y bar
	;movlw .7
	;call LCD_goto_row
	;movff y_pos_norm,bar_val
	;bcf STATUS,C
	;rrcf bar_val
	;call print_bar

print_bar
	;Set column to h00
	movlw .0
	call LCD_goto_column

	;Setup the counter	
	movlw .0
	movwf column_ctr
	incf bar_val
	incf bar_val
print_bar_loop_fill
	incf column_ctr,W
	movwf column_ctr
	sublw .129
	bz bar_return_vec

	movf  column_ctr,W
	cpfsgt bar_val
	bra print_bar_loop_blank_print

	movlw b'11111111'
	call LCD_data_send
	bra print_bar_loop_fill

print_bar_loop_blank
	incf column_ctr,f
	movf column_ctr,W
	sublw .129
	bz bar_return_vec

print_bar_loop_blank_print
	movlw b'00000000'
	call LCD_data_send
	bra print_bar_loop_blank

bar_return_vec
	return


;////////////////////////////////////////////////////////////
 cblock
x_pos_norm:2,y_pos_norm:2
 endc

normalise_coords
	;This changes the coords to 0-256 and checks the bounds
	btfss touched,0
	return

	movff x_pos,x_pos_norm
	movff x_pos+1,x_pos_norm+1
	movff y_pos,y_pos_norm
	movff y_pos+1,y_pos_norm+1

	;Check they're large enough
	movf x_min,W
	subwf x_pos_norm,F
	movf x_min+1,W
	subwfb x_pos_norm+1,F
	
	btfsc STATUS,N
	bra normalise_min_fail_x_test

	movf y_min,W
	subwf y_pos_norm,F
	movf y_min+1,W
	subwfb y_pos_norm+1,F
	
	btfsc STATUS,N
	bra normalise_min_fail_y_test

	;Now confirm they're not too big
	movf x_pos_norm,W
	subwf x_range,W
	movf x_pos_norm+1,W
	subwfb x_range+1,W
	
	btfsc STATUS,N
	bra normalise_range_fail_x_test

	movf y_pos_norm,W
	subwf y_range,W
	movf y_pos_norm+1,W
	subwfb y_range+1,W

	btfsc STATUS,N
	bra normalise_range_fail_y_test

	;Now multiply by 255 and divide by the range
	;X first
	clrf NROA_H
	movlw .255
	movwf NROA_L
	movff x_pos_norm,NROB_L
	movff x_pos_norm+1,NROB_H
	call MUL_1616U
	
	movff x_range,DSOR_0
	movff x_range+1,DSOR_1
	call DIV_3216U_KEN
	movff QUOT_L,x_pos_norm

	;Now Y
	clrf NROA_H
	movlw .255
	movwf NROA_L
	movff y_pos_norm,NROB_L
	movff y_pos_norm+1,NROB_H
	call MUL_1616U
	
	movff y_range,DSOR_0
	movff y_range+1,DSOR_1
	call DIV_3216U_KEN
	movff QUOT_L,y_pos_norm
	;call DIV_3216U_ATFN
	
	;movff RES0,x_pos_norm

	return

normalise_min_fail_x_test
	movff x_pos,x_min
	movff x_pos+1,x_min+1
	bra normalise_fail

normalise_min_fail_y_test
	movff y_pos,y_min
	movff y_pos+1,y_min+1
	bra normalise_fail

normalise_range_fail_x_test
	movff x_pos_norm,x_range
	movff x_pos_norm+1,x_range+1
	bra normalise_fail

normalise_range_fail_y_test
	movff y_pos_norm,y_range
	movff y_pos_norm+1,y_range+1

normalise_fail
	bcf touched,0
	clrf x_pos
	clrf y_pos
	return
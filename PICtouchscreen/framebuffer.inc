;Framebuffer Driver for the Graphical LCD

 __badram 0x200-0x4ff

#define fb_columns .128
#define fb_half_columns .64
#define fb_rows .8 ;*8=64 pixels
#define fb_dbl_row_bits .15

 cblock
fb_column_ctr, fb_row_ctr
 endc

;/////////////////////////////////////////
Load_framebuffer
	movlw 0x02
	movwf FSR0H
	movlw 0x00
	movwf FSR0L
	return

;/////////////////////////////////////////
blank_framebuffer
	call Load_framebuffer

	clrf fb_row_ctr
blank_framebuffer_row_loop
	clrf fb_column_ctr
	movlw fb_rows
	xorwf fb_row_ctr,W
	bz blank_framebuffer_end
	incf fb_row_ctr
blank_framebuffer_col_loop
	movlw fb_columns
	xorwf fb_column_ctr,W
	bz blank_framebuffer_row_loop

	clrf POSTINC0

	incf fb_column_ctr
	bra blank_framebuffer_col_loop	

blank_framebuffer_end
	return


;/////////////////////////////////////////
;Goes to the row specified in fb_row_ctr
;Assumes a Load_framebuffer has already occured
 cblock
fb_row_num
 endc

fb_goto_row
	movwf fb_row_num
	btfsc fb_row_num,0
	bsf FSR0L,7

	rrcf fb_row_num,W
	addwf FSR0H,F
	return

fb_goto_col
	andlw b'01111111'
	addwf FSR0L,F
	return

;/////////////////////////////////////////
;Transmits a single row, as a whole screen is too long
transmit_framebuffer
	call Load_framebuffer

	;Increment the row number and keep in 0-7
	incf fb_row_ctr,W	
	andlw b'00000111'
	movwf fb_row_ctr
	call LCD_goto_row

	;Modify the FSR to the right row
	movf fb_row_ctr,W
	call fb_goto_row

	;Goto the first column 
	movlw .0
	call LCD_goto_column

	;Reset the column counter
	movlw fb_columns
	movwf fb_column_ctr
transmit_framebuffer_col_loop
	movf POSTINC0,W
	call LCD_data_send

	dcfsnz fb_column_ctr
	return

	bra transmit_framebuffer_col_loop

;/////////////////////////////////////////
 cblock
pixel_x, pixel_y, pixel_mask, tmp_var
 endc

fb_set_pixel
	;Bit mashing to get the correct pixel
	movlw b'00000001'
	btfsc pixel_y,1
	movlw b'00000100'

	movwf pixel_mask

	bcf STATUS,C
	btfsc pixel_y,0
	rlcf pixel_mask,F

	btfsc pixel_y,2
	swapf pixel_mask,F

	;Now get the row
	bcf STATUS,C
	rrcf pixel_y
	bcf STATUS,C
	rrcf pixel_y
	bcf STATUS,C
	rrcf pixel_y

	call Load_framebuffer
	;Goto the row
	btfsc pixel_y,0
	bsf FSR0L,7
	rrcf pixel_y,W
	addwf FSR0H,F

	;Print the pixel at the column
	movf pixel_x,W
	movf PLUSW0,W
	iorwf pixel_mask,F
	movf pixel_x,W
	movff pixel_mask, PLUSW0	

	return
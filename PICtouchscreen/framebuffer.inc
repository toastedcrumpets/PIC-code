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
fb_goto_row
	btfsc WREG,0
	bsf FSR0L,7

	bcf STATUS,C
	rrcf WREG,W
	addwf FSR0H,F
	return

fb_goto_col
	andlw b'01111111'
	addwf FSR0L,F
	return

transmit_framebuffer
	rcall transmit_framebuffer_row
	rcall transmit_framebuffer_row
	rcall transmit_framebuffer_row
	rcall transmit_framebuffer_row
	rcall transmit_framebuffer_row
	rcall transmit_framebuffer_row
	rcall transmit_framebuffer_row
	rcall transmit_framebuffer_row
	return	

;/////////////////////////////////////////
;Transmits a single row, as a whole screen is too long
transmit_framebuffer_row
	call Load_framebuffer

	;Increment the row number and keep in 0-7
	incf fb_row_ctr,W	
	andlw b'00000111'
	movwf fb_row_ctr
	call LCD_goto_row

	;Modify the FSR to the right row
	movf fb_row_ctr,W
	rcall fb_goto_row

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
	call Load_framebuffer

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

	;Now goto the row
	rlncf pixel_y
	swapf pixel_y,W
	rcall fb_goto_row

	;Print the pixel at the column
	movf pixel_x,W
	rcall fb_goto_col
	movf pixel_mask,W
	iorwf INDF0,F

	return
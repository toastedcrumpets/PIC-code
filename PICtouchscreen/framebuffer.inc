;Framebuffer Driver for the Graphical LCD

 __badram 0x200-0x4ff

#define fb_columns .128
#define fb_half_columns .64
#define fb_rows .8 ;*8=64 pixels
#define fb_dbl_row_bits .15

;/////////////////////////////////////////
Load_framebuffer
	movlw 0x02
	movwf FSR0H
	clrf FSR0L
	return

;/////////////////////////////////////////
 cblock
fb_row_ctr
 endc

blank_framebuffer
	call Load_framebuffer
	clrf fb_row_ctr

blank_framebuffer_row_loop
	movlw fb_rows
	xorwf fb_row_ctr,W
	bz blank_framebuffer_end
	incf fb_row_ctr
	
	rcall blank_framebuffer_line
	bra blank_framebuffer_row_loop

blank_framebuffer_end
	return

;/////////////////////////////////////////
blank_framebuffer_line
	;goto the 0th column
	movlw b'10000000'
	andwf FSR0L
	movlw .128
blank_framebuffer_loop
	clrf POSTINC0
	dcfsnz WREG
	return
	bra blank_framebuffer_loop

;/////////////////////////////////////////
;Goes to the row specified in fb_row_ctr
;Assumes a Load_framebuffer has already occured
fb_goto_row
	;Make sure the range is correct
	andlw b'00000111'
	;Now go there
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
 cblock
fb_trans_row_ctr
fb_trans_col_ctr
 endc
transmit_framebuffer_row
	call Load_framebuffer

	;Increment the row number and keep in 0-7
	incf fb_trans_row_ctr,W	
	andlw b'00000111'
	movwf fb_trans_row_ctr
	call LCD_goto_row

	;Modify the FSR to the right row
	movf fb_trans_row_ctr,W
	rcall fb_goto_row

	;Goto the first column 
	movlw .0
	call LCD_goto_column

	;Reset the column counter
	movlw fb_columns
	movwf fb_trans_col_ctr
transmit_framebuffer_col_loop
	movf POSTINC0,W
	call LCD_data_send

	dcfsnz fb_trans_col_ctr
	return

	bra transmit_framebuffer_col_loop

transmit_framebuffer_fast
	call Load_framebuffer

	;Increment the row number and keep in 0-7
	incf fb_trans_row_ctr,W	
	andlw b'00001111'
	movwf fb_trans_row_ctr
	andlw b'00000111'
	call LCD_goto_row

	;Modify the FSR to the right row
	movf fb_trans_row_ctr,W
	rcall fb_goto_row

	;Goto the first column 
	movlw .0
	btfsc fb_trans_row_ctr,3
	movlw .64
	call LCD_goto_column

	movlw .0
	btfsc fb_trans_row_ctr,3
	movlw .64
	call fb_goto_col
	;Reset the column counter
	movlw fb_half_columns
	movwf fb_trans_col_ctr
transmit_framebuffer_fast_col_loop
	movf POSTINC0,W
	call LCD_data_send

	dcfsnz fb_trans_col_ctr
	return

	bra transmit_framebuffer_fast_col_loop

;/////////////////////////////////////////
 cblock
pixel_x, pixel_y, pixel_mask
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
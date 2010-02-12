;Commands for text blitting

;////////////////////////////////////////////////////////////////////
;Expects the char number in W
;And the framebuffer in the right location
 cblock
char_num
 endc

blit_char
	movwf char_num
	LoadTable(char_table)
	movf char_num,W
	mullw .4

	movf PRODL,W
	addwf TBLPTRL,F
	movf PRODH,W
	addwfc TBLPTRH,F
	movlw .0
	addwfc TBLPTRU,F

	;Load the number of cols per character
	TBLRD*+
	movff TABLAT, char_num
	;Now goto the character
	LoadTableFromTABLAT

blit_char_loop
	TBLRD*+
	movff TABLAT,POSTINC0
	dcfsnz char_num
	return
	bra blit_char_loop


blit_char_box
	movwf char_num
	LoadTable(char_table)
	movf char_num,W
	mullw .4

	movf PRODL,W
	addwf TBLPTRL,F
	movf PRODH,W
	addwfc TBLPTRH,F
	movlw .0
	addwfc TBLPTRU,F

	;Load the number of cols per character
	TBLRD*+
	movff TABLAT, char_num
	;Now goto the character
	LoadTableFromTABLAT

blit_char_box_loop
	TBLRD*+
	movf TABLAT,W
	iorlw b'10000001'
	movwf POSTINC0
	dcfsnz char_num
	return
	bra blit_char_box_loop

 cblock
tbl_tmp_L,tbl_tmp_H,tbl_tmp_U
 endc

blit_print_table_string
	TBLRD*+	
	movf	TABLAT,W

	btfsc	STATUS,Z
	return

	movff TBLPTRL, tbl_tmp_L
	movff TBLPTRH, tbl_tmp_H
	movff TBLPTRU, tbl_tmp_U

	movlw .32
	subwf TABLAT,W
	;movlw .33
	rcall blit_char
	movlw .0
	movwf POSTINC0

	movff tbl_tmp_L,TBLPTRL
	movff tbl_tmp_H,TBLPTRH
	movff tbl_tmp_U,TBLPTRU

	bra	blit_print_table_string

blit_box_print_table_string
	movlw b'01111110'
	movwf POSTINC0
	movlw b'10000001'
	movwf POSTINC0

blit_box_print_table_string_loop
	TBLRD*+	
	movf	TABLAT,W

	btfsc	STATUS,Z
	bra blit_box_print_table_string_ret

	movff TBLPTRL, tbl_tmp_L
	movff TBLPTRH, tbl_tmp_H
	movff TBLPTRU, tbl_tmp_U

	movlw .32
	subwf TABLAT,W
	;movlw .33
	rcall blit_char_box
	movlw b'10000001'
	movwf POSTINC0

	movff tbl_tmp_L,TBLPTRL
	movff tbl_tmp_H,TBLPTRH
	movff tbl_tmp_U,TBLPTRU

	bra	blit_box_print_table_string_loop

blit_box_print_table_string_ret
	movlw b'01111110'
	movwf POSTINC0
	return

;************************************************************************
;Translate W to a 2 word hex code, and write that hex code to the screen
W_2HEX_to_LCD
	movwf	w_temp
	swapf	w_temp,W		;First Number
	call	W_1HEX_to_LCD
	movf	w_temp,W		;second_number
	call	W_1HEX_to_LCD
	return

;************************************************************************
;Translate the lower nibble of W as a hex code and write it to the screen
W_1HEX_to_LCD				
	andlw	0x0f			;Mask the lower nibble
	addlw	-.10			;Test if A-F

	btfsc	STATUS,C	;Skip next if it is a number
	addlw	.7			;Add the ascii offset between numbers and characters

	addlw	.26			;Get it to at least the numbers (16) plus the previous subtract (9)
	call	blit_char
	movlw b'00000000'
	movwf POSTINC0
	return
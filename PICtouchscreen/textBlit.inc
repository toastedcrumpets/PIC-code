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
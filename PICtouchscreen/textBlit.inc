;Commands for text blitting

extern char_table

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
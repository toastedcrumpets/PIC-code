;Numerical input
 cblock
BIN_hi,BIN_lo
BCD_0,BCD_1,BCD_2,BCD_3,BCD_4
 endc

Numerical_input_Handler
	call b16_d5

	movlw LOW(BCD_0)
	movwf FSR0L
	movlw HIGH(BCD_0)
	movwf FSR0H

	swapf x_pos,W
	rlncf WREG
	andlw b'00011111'

	bnz increment_LSD

	sublw .4
	addwf FSR0L,F
	clrf WREG
	addwfc FSR0H,F

	incf INDF0,W
	movwf INDF0

	xorlw .4
	btfsc STATUS,Z
	clrf INDF0

increment_LSD
	sublw .4
	addwf FSR0L,F
	clrf WREG
	addwfc FSR0H,F

	incf INDF0,W
	movwf INDF0

	xorlw .10
	btfsc STATUS,Z
	clrf INDF0

Numerical_input_Handler_save
	call d5_b16

	;Make sure this isn't toggled multiple times
	call touch_wait_for_release

	goto draw_numerical_input
	
add_numerical_input_touch
	movlw .4
	movwf Insert_Touch_var_y

	movlw .0
	movwf Insert_Touch_var_x

	call Touch_Goto_Entry
	Insert_Touch_value_N Numerical_input_Handler,.3

	return


draw_numerical_input
	call Load_framebuffer
	movlw .4
	call fb_goto_row
	movlw .0
	call fb_goto_col

	movf BCD_4,W
	addlw 0x10
	call blit_char
	clrf POSTINC0

	movf BCD_3,W
	addlw 0x10
	call blit_char
	clrf POSTINC0

	movf BCD_2,W
	addlw 0x10
	call blit_char
	clrf POSTINC0

	movf BCD_1,W
	addlw 0x10
	call blit_char
	clrf POSTINC0

	movf BCD_0,W
	addlw 0x10
	call blit_char
	clrf POSTINC0


	return


;/////////////////////////////////////////////////////
b16_d5
;Takes hex number in NumH:NumL  Returns decimal in ;TenK:Thou:Hund:Tens:Ones
;written by John Payson

;input
;=A3*163 + A2*162 + A1*161 + A0*160
;=A3*4096 + A2*256 + A1*16 + A0
NumH            EQU BIN_hi        ;A3*16+A2
NumL            EQU BIN_lo	;A1*16+A0
;share variables
;=B4*104 + B3*103 + B2*102 + B1*101 + B0*100
;=B4*10000 + B3*1000 + B2*100 + B1*10 + B0
TenK            EQU BCD_4      ;B4
Thou            EQU BCD_3		;B3
Hund            EQU BCD_2		;B2
Tens            EQU BCD_1		;B1
Ones            EQU BCD_0		;B0

 SWAPF   NumH,W
 IORLW B'11110000'
 MOVWF   Thou
       ADDWF   Thou,F
       ADDLW   0xE2
       MOVWF   Hund
       ADDLW   0x32
       MOVWF   Ones

       MOVF    NumH,W
       ANDLW   0x0F
       ADDWF   Hund,F
       ADDWF   Hund,F
       ADDWF   Ones,F
       ADDLW   0xE9
       MOVWF   Tens
       ADDWF   Tens,F
       ADDWF   Tens,F

       SWAPF   NumL,W
       ANDLW   0x0F
       ADDWF   Tens,F
       ADDWF   Ones,F

       RLCF     Tens,F
       RLCF     Ones,F
       COMF   Ones,F
       RLCF     Ones,F

       MOVF    NumL,W
       ANDLW   0x0F
       ADDWF   Ones,F
       RLCF     Thou,F

       MOVLW   0x07
       MOVWF   TenK
       MOVLW   0x0A
Lb1:
       DECF    Tens,F
       ADDWF   Ones,F
       BTFSS   STATUS,0
       GOTO   Lb1
Lb2:
       DECF    Hund,F
       ADDWF   Tens,F
       BTFSS   STATUS,0
 GOTO   Lb2
Lb3:
       DECF    Thou,F
       ADDWF   Hund,F
       BTFSS   STATUS,0
       GOTO   Lb3
Lb4:
       DECF    TenK,F
       ADDWF   Thou,F
       BTFSS   STATUS,0
       GOTO   Lb4

	return

;///////////////////////////////////////////////////////
d5_b16
    movf  BCD_1,W        ; (BCD_1 + BCD_3) * 2
    addwf BCD_3,W
    movwf BIN_lo
    rlcf   BIN_lo,F

    swapf BCD_2,W        ; + BCD_2 * 16 + BCD_2
    addwf BCD_2,W
    addwf BIN_lo,F

    rlcf   BCD_4,W        ; + (BCD_4 * 2 + BCD_3) * 256
    addwf BCD_3,W
    movwf BIN_hi

    rlcf   BIN_lo,F     ; * 2
    rlcf   BIN_hi,F

    swapf BCD_3,W        ; - BCD_3 * 16
    subwf BIN_lo,F
    btfss STATUS,C
    decf  BIN_hi,F

    swapf BCD_2,W        ; + BCD_2 * 16 + BCD_1
    addwf BCD_1,W
    addwf BIN_lo,F
    btfsc STATUS,C
    incf  BIN_hi,F

    swapf BCD_4,W        ; + BCD_4 * 16 + BCD_0
    addwf BCD_0,W

    rlcf   BIN_lo,F     ; * 2
    rlcf   BIN_hi,F

    addwf BIN_lo,F
    btfsc STATUS,C
    incf  BIN_hi,F

    movf  BCD_4,W        ; - BCD_4 * 256
    subwf BIN_hi,F

    swapf BCD_4,W        ; + BCD_4 * 16 * 256 * 2
    addwf BIN_hi,F
    addwf BIN_hi,F

    return            ; Q.E.D.

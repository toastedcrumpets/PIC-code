; Table handling macros


 cblock
L_tmp,H_tmp,U_tmp
 endc

LoadTableFromTABLAT macro
	TBLRD*+
	movff TABLAT,L_tmp
	TBLRD*+
	movff TABLAT,H_tmp
	TBLRD*+
	movff TABLAT,TBLPTRU
	movff H_tmp,TBLPTRH
	movff L_tmp,TBLPTRL
	endm

GotoFuncFromTABLAT macro
	movff TABLAT,L_tmp
	TBLRD+*
	movff TABLAT,PCLATH
	TBLRD+*
	movff TABLAT,PCLATU
	movf  L_tmp,W
	movwf PCL
	endm

;Delay generation macros
generate_delay_code macro _prescaler, _count
	movlw	_prescaler	
	movwf	T0CON		
	movlw	_count		
	movwf	TMR0L		
	bcf	INTCON,TMR0IF	
	btfss	INTCON,TMR0IF
	bra	$-.2
	return
	endm

;Delay code generators
delay_20ms generate_delay_code b'11000111', -.156
delay_1ms  generate_delay_code b'11000010', -.249
delay_15us generate_delay_code b'11001000', -.21

 cblock
D0,D1,D2,D3,D4
NUMLO,NUMHI
 endc
dec2bin16
    movf  D1,W        ; (D1 + D3) * 2
    addwf D3,W
    movwf NUMLO
    rlcf   NUMLO,F

    swapf D2,W        ; + D2 * 16 + D2
    addwf D2,W
    addwf NUMLO,F

    rlcf   D4,W        ; + (D4 * 2 + D3) * 256
    addwf D3,W
    movwf NUMHI

    rlcf   NUMLO,F     ; * 2
    rlcf   NUMHI,F

    swapf D3,W        ; - D3 * 16
    subwf NUMLO,F
    btfss STATUS,C
    decf  NUMHI,F

    swapf D2,W        ; + D2 * 16 + D1
    addwf D1,W
    addwf NUMLO,F
    btfsc STATUS,C
    incf  NUMHI,F

    swapf D4,W        ; + D4 * 16 + D0
    addwf D0,W

    rlcf   NUMLO,F     ; * 2
    rlcf   NUMHI,F

    addwf NUMLO,F
    btfsc STATUS,C
    incf  NUMHI,F

    movf  D4,W        ; - D4 * 256
    subwf NUMHI,F

    swapf D4,W        ; + D4 * 16 * 256 * 2
    addwf NUMHI,F
    addwf NUMHI,F

    return            ; Q.E.D.

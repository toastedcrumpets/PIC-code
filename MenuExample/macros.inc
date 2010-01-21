; Table handling macros


 cblock
L_tmp,H_tmp,U_tmp
 endc

LoadTableFromTABLAT macro
	movff TABLAT,L_tmp
	TBLRD+*
	movff TABLAT,H_tmp
	TBLRD+*
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
delay_40ms generate_delay_code b'11000111', -.156
delay_2ms  generate_delay_code b'11000010', -.249
delay_30us generate_delay_code b'11001000', -.21
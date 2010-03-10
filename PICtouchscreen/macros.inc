; Table handling macros


 cblock
L_tmp,H_tmp,U_tmp,tmpvar
 endc

;Sets up the table ptr for a fixed address
LoadTable macro _table
	movlw LOW (_table) 
	movwf TBLPTRL	
	movlw HIGH (_table)	
	movwf TBLPTRH 
	movlw UPPER (_table)
	movwf TBLPTRU 
	endm

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

;/////////////////////////////////////////////////////////
;EEPROM accessing

EEPROM_read macro addr, output
	movlw HIGH(addr)
	movwf EEADRH
	movlw LOW(addr)
	movwf EEADR
	bcf EECON1, EEPGD ;Point to data memory
	bcf EECON1, CFGS; Access EEPROM
	bsf EECON1, RD ;Start read
	movff EEDATA, output
	endm

EEPROM_write macro addr, input
	btfsc EECON1,WRERR
	bra $-.2
	bcf PIR2,EEIF

	movlw HIGH(addr)
	movwf EEADRH
	movlw LOW(addr)
	movwf EEADR
	movf input,W
	movwf EEDATA
	bcf EECON1, EEPGD
	bcf EECON1, CFGS
	bsf EECON1, WREN

	movlw 55h
	movwf EECON2
	movlw 0AAh
	movwf EECON2
	bsf EECON1, WR

	bcf EECON1,WREN
	endm

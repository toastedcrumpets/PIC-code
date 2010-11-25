;Some useful macros

;Sets up the table ptr for a fixed address
LoadTable macro _table
	movlw LOW (_table) 
	movwf TBLPTRL	
	movlw HIGH (_table)	
	movwf TBLPTRH 
	movlw UPPER (_table)
	movwf TBLPTRU 
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
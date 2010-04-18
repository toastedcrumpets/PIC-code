;UART debug header

UART_init
	;Setup the baud rate 115.2K4
	bcf TXSTA,SYNC
	bcf TXSTA,BRGH
	bsf BAUDCON,BRG16
	bcf TXSTA,TX9
	
	bsf TRISB,5
	bsf TRISB,7

	;115.2k
	clrf SPBRGH
	movlw .24
	movwf SPBRG

	;9.6k
	;movlw HIGH(.311)
	;movwf SPBRGH
	;movlw LOW(.311)
	;movwf SPBRG

	;Enable the UART
	bsf RCSTA,SPEN

	;Turn on transmission
	bsf TXSTA,TXEN
	return
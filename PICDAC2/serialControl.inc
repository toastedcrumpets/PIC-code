;Serial control interface
 include "SerialCommands.inc"


;////////////////////////////////////////
;Determine what the controller PIC wants.
	;Clear the interrupt and load the byte
process_serial_data
	bcf PIR1,RCIF
	movf RCREG,W

	btfsc RCSTA,0
	bra process_data
	
;///Process the command byte
process_command
	;Store the command byte
	movwf PRODH
	
	;Check the checksum
	xorlw 0xFF ;Full bit invert
	swapf WREG
	subwf PRODH,W
	bnz process_command_fail

	;Locate the entry in the command table and jump to it
	;First get the command number
	movf PRODH,W
	andlw 0x0F

	mullw .4
	
	;Now check it is valid by subtracting the number of commands plus one and branching on no carry
	addlw -NCommandsPlusOne
	bnn process_command_fail

	movlw LOW(Command_table)
	addwf PRODL,W
	movwf TBLPTRL

	movlw HIGH(Command_table)
	addwfc PRODH,W
	movwf TBLPTRH

	clrf TBLPTRU
	movlw UPPER(Command_table)
	addwfc TBLPTRU,F

	TBLRD*+
	movff TABLAT,PCLATU
	TBLRD*+
	movff TABLAT,PCLATH
	TBLRD*
	movf  TABLAT,W
	movwf PCL

process_command_fail
	retfie FAST

;///Process the data byte
process_data
	retfie FAST


;////////////////////////////////////////
serial_control_init
	;Setup the pins for the UART
	bsf TRISB,4
	bsf TRISB,1

	bsf ADCON1,5
	bsf ADCON1,6

	;Setup the UART to the FuncGen device
	;Enable the UART
	BSF RCSTA,SPEN
	BCF TXSTA,SYNC 	;Asynchronous mode
	
	;9bit mode
	BSF RCSTA,RX9
	BSF TXSTA,TX9

	;//Baud rate
	;High speed
	BSF TXSTA,BRGH
	;16bit clocking
	BSF BAUDCTL,BRG16

	;115.2 serial clock available
	clrf SPBRGH
	movlw .86
	movwf SPBRG

	;//Transmit Mode On
	BSF TXSTA,TXEN
	;//Recieve Mode On
	BSF RCSTA,CREN

	;//Establish the interrupt
	BSF PIE1,RCIE

	return
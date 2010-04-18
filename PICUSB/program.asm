;Main Program entry point

org 0x00
goto init


 include <p18f14k50.inc>
 include "USB.inc"
 include "UARTDebug.inc"

init

	call UART_init

	;Init LED
	bcf TRISC,3
	bsf LATC,3

	;Init USB
	call USB_init

main

	btfss PIR1,TXIF
	bra $-.2
	movlw 'H'
	movwf TXREG
	nop

	btfss PIR1,TXIF
	bra $-.2
	movlw 'E'
	movwf TXREG
	nop

	btfss PIR1,TXIF
	bra $-.2
	movlw 'L'
	movwf TXREG
	nop

	btfss PIR1,TXIF
	bra $-.2
	movlw 'L'
	movwf TXREG
	nop


	btfss PIR1,TXIF
	bra $-.2
	movlw 'O'
	movwf TXREG
	bra main

 end
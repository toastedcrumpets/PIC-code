;Main Program entry point

org 0x00
goto init


 include <p18f14k50.inc>
 include "USB.inc"
 include "UARTDebug.inc"

init

	;call UART_init

	;Init LED
	bcf TRISA,5
	bsf LATA,5

	bcf TRISA,0
	bsf LATA,0

	bcf TRISA,1
	bsf LATA,1

	;Init USB
	;call USB_init

main
	;movlw 0xAA
	;clrf STATUS
	;UART_print_string(Test_string)
	;UART_print_string(Test_string)
	;UART_print_reg_HEX(STATUS)
	;UART_endl
	;UART_print_reg_HEX(STATUS)
	;UART_endl
	;UART_print_reg_BIN(STATUS)
	;UART_endl
	;UART_print_reg_BIN(STATUS)

	bra $

;Test_string db "Testing the console output\n",0x00

 end
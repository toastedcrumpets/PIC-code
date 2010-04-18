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
	movlw 0xAA
	UART_print_string(Test_string)
	UART_print_string(Test_string)
	UART_print_reg_HEX(WREG)
	UART_print_reg_HEX(WREG)
	UART_print_reg_BIN(WREG)
	UART_print_reg_BIN(WREG)

	bra $

Test_string db "Testing the console output ",0x00

 end
 include <p18f14k50.inc>

LED_GREEN macro
	bsf PORTC,3
	bcf PORTC,6
 endm

LED_RED macro
	bcf PORTC,3
	bsf PORTC,6
 endm

LED_OFF macro
	bcf PORTC,3
	bcf PORTC,6
 endm

 cblock 0x00
FLAGS ;State of the system
NewButtonState
OldButtonState
 endc

 #define ErrorState FLAGS,0

 #define OUTPUT_RELAY LATA,5
 #define FAN LATC,5
 #define FAN_ON LATC,5
 #define VOLT_LIMIT PORTB,4
 #define AMP_LIMIT PORTA,3
 #define OUTPUT_BUTTON PORTA,0
 #define OTHER_BUTTON PORTA,1

;Main Program entry point
 org 0x00
	goto init

 ;High priority interrupts
 org 0x000008
	nop
 	retfie FAST

 ;Low priority interrupt (Buttons)
 org 0x000018
	
	btfss INTCON,RABIF ;Check it's a button change 
	bra next_low_interrupt

	bcf INTCON,RABIF ;clear the interrupt

	;Check for state change
	movf PORTA,W
	andlw b'00000011'
	movwf NewButtonState ;Save the new state
	
	xorwf OldButtonState,W ;Check for changes (in W)
	;Save the new state as the old state
	movff NewButtonState,OldButtonState

	movwf NewButtonState ;NewButtonState now holds if a button changed

	;Check if we're in an error state, we don't operate the output in an errored state
	btfsc ErrorState
	bra check_other_button
	
	;Now check for state change of the Output Button
	btfss NewButtonState,0
	bra check_other_button

	;The output button has changed state
	;Check it's low (depression)
	btfsc OldButtonState,0
	bra check_other_button
	
	;The output button has been freshly pressed
	;Check if the output is already on
	btfss OUTPUT_RELAY
	bra switch_output_on

	bcf OUTPUT_RELAY
	LED_OFF
	bra check_other_button

switch_output_on
	bsf OUTPUT_RELAY
	LED_GREEN

check_other_button
	;check for a state change
	btfss NewButtonState,1
	bra next_low_interrupt	

	;State change, check for depression
	btfsc OldButtonState,1
	bra next_low_interrupt

	btfss ErrorState
	bra no_error

	;Exit from an error state
	bcf ErrorState
	LED_OFF
	bra next_low_interrupt
	
no_error
	;We have no error, so lets fake one
	bcf OUTPUT_RELAY
	bsf ErrorState
	LED_RED

next_low_interrupt	

 	retfie FAST

 #define OSC_SPEED .1000000

 #define LCD_RS LATB,7
 #define LCD_SCL LATB,6
 #define LCD_SI LATC,7
 #define LCD_CS LATB,5
 #define LCD_3L5V
 include "LCD.inc"

Welcome_Message db "Warming Up...",0x0

init
	;///Setup the oscillator
	movlw b'00110000'
	movwf OSCCON

	clrf ANSEL
	clrf ANSELH
	movlw b'11111111'
	movwf TRISA
	movlw b'00011111'
	movwf TRISB
	movlw b'01111111'	
 	movwf TRISC
	
	;Set the output relays off
	bcf OUTPUT_RELAY
	bcf TRISA,5 ;Now make it an output

	;Set the Fan on
	bcf TRISC,5
	bsf FAN

	;Set the LCD into unselected
	bsf LCD_CS
	
	;Enable weak pull ups
	bcf INTCON2,NOT_RABPU
	;Enable weak pull ups on the button pins
	bsf WPUA,0
	bsf WPUA,1

	;enable interrupt on change for the pins (required for input!)
	bsf IOCA,0
	bsf IOCA,1
	bcf UCON,USBEN

	;Setup the button LED
	LED_OFF
	bcf TRISC,3
	bcf TRISC,6

	;Setup the state of the system
	bcf ErrorState
	movwf b'00000011' ;Button lines were high
	movwf OldButtonState

	;Setup interrupt on change for the button pins
	movlw b'00000011'
	movwf IOCA
	
	bsf INTCON,RABIE ;Enable IOC on portA/B
	bcf INTCON2,RABIP ;Low priority portA/B IOC

	bcf INTCON,RABIF ;Kill the fake startup interrupt
	;Enable priority-level interrupts
	bsf RCON,IPEN
	;Enable low-priority interrupts
	bsf INTCON,GIEL
	bsf INTCON,GIE

	;//Let the Oscillator stabilise
	btfss OSCCON,IOFS
	bra $-.2

	;Select the LCD
	bcf LCD_CS
	call LCD_INIT
	bsf LCD_CS

	bcf LCD_CS
	call LCD_clear
	LCDPrintString Welcome_Message
	bsf LCD_CS

	;Wait loop
	movlw .50
startup_wait_loop
	call delay_40ms
	decfsz WREG
 	bra startup_wait_loop


main_loop
	rcall REDRAW_LCD
	
	bra main_loop


;;;;;;;;;;;;;;;;LCD Redraw Function;;;;;;;;;;;;;;;;;;;;;;
Line_1 db "0.000 V  0.00 W",0x0
Line_2 db "1.000 A  0.00 R",0x0

REDRAW_LCD
	bcf LCD_CS
	call LCD_line_1
	movlw ' '
	btfsc VOLT_LIMIT
	movlw b'01111110'
	call LCD_data_send

	LCDPrintString Line_1

	call LCD_line_2
	movlw ' '
	btfsc AMP_LIMIT
	movlw b'01111110'
	call LCD_data_send

	LCDPrintString Line_2

	bsf LCD_CS
	return

 end
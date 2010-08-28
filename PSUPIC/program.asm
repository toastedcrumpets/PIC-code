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
OldAmpLimitState
VoltReading:2
AmpReading:2
WattReading:4
 endc

 #define ErrorState FLAGS,0
 #define EFuse_ON FLAGS,1
 #define AMP_LIMIT_VAL FLAGS,2
 #define AMP_LIMIT_VAL_MASK b'00000100'


 #define OUTPUT_RELAY LATA,5
 #define TRANSFORMER_RELAY LATC,4
 #define FAN_ON LATC,5
 #define VOLT_LIMIT PORTB,4
 #define AMP_LIMIT_PORT PORTA,3
 #define OUTPUT_BUTTON PORTA,0
 #define OTHER_BUTTON PORTA,1
 #define SHUTDOWN PORTA,4

;Main Program entry point
 org 0x00
	goto init

 ;High priority interrupts
 org 0x000008
	nop
 	return

 ;Low priority interrupt (Buttons)
 org 0x000018
	
	btfss INTCON,RABIF ;Check it's a button change 
	bra next_low_interrupt

	bcf INTCON,RABIF ;clear the interrupt

	;First, poll the value of the amp limiting port
	btfsc AMP_LIMIT_VAL
	bra Not_Efuse

	;We're in current limitation, check for the efuse
	btfss EFuse_ON
	bra Not_Efuse

	;Check the output is even on
	btfss OUTPUT_RELAY
	bra Not_Efuse

	;Ok, Efuse has to blow now
	bsf SHUTDOWN
	bcf OUTPUT_RELAY
	bsf ErrorState
	LED_RED

	bra Not_Efuse

Not_Efuse

	;Check for button state change
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
	bcf SHUTDOWN
	LED_OFF
	bra next_low_interrupt
	
no_error
	;We have no error, so toggle the e-fuse enable
	btg EFuse_ON

next_low_interrupt	

 	retfie FAST

 #define OSC_SPEED .16000000

 #define LCD_RS LATB,7
 #define LCD_SCL LATB,6
 #define LCD_SI LATC,7
 #define LCD_CS LATB,5
 #define LCD_3L5V
 include "LCD.inc"

Welcome_Message db "Warming Up...",0x0

init
	;///Setup the oscillator
	movlw b'01110000'
	movwf OSCCON

	clrf ANSEL
	clrf ANSELH
	movlw b'11001111'
	movwf TRISA
	movlw b'00011111'
	movwf TRISB
	movlw b'01001111'	
 	movwf TRISC
	
	;Set the output relays off
	bcf OUTPUT_RELAY

	;Set the Fan on
	bsf FAN_ON

	;Turn off the shutdown 
	bcf SHUTDOWN

	;Switch to the low voltage tapping of the transformer
	bcf TRANSFORMER_RELAY

	;Set the LCD into unselected
	bsf LCD_CS
	
	;Enable weak pull ups
	bcf INTCON2,NOT_RABPU
	;Enable weak pull ups on the button pins
	bsf WPUA,0
	bsf WPUA,1
	bsf WPUA,3

	;Disable the efuse at first
	bcf EFuse_ON
	bcf AMP_LIMIT_VAL
	clrf OldAmpLimitState

	;enable interrupt on change for the pins (required for input!)
	bsf IOCA,0
	bsf IOCA,1
	bsf IOCA,3
	bcf UCON,USBEN

	;Setup the button LED
	LED_OFF
	bcf TRISC,3
	bcf TRISC,6

	;Setup the state of the system
	bcf ErrorState
	movwf b'00000011' ;Button lines were high
	movwf OldButtonState

	;Setup the FVR
	bsf REFCON0,FVR1EN ;Turn it on

	;Setup the ADC convertor, establish analogue pins
	bsf TRISC,0 ;Voltage Sense Line
	bsf ANSEL,ANS4 ;....
	bsf TRISC,1 ;Current Sense Line
	bsf ANSEL,ANS5 ;......
	clrf ADCON1 ;Select VSS and VDD as references	
	bsf ADCON0,0  ;Turn ADC on
	movlw b'10100110' ;[7]right justified,[5:3] ACQT=8 TAD, [2:0] ADCS=Fosc/64
	movwf ADCON2

	;Set the vref of the adc to FVR
	bsf ADCON1,3
	bcf ADCON1,2

	movlw b'10110000' ;4.096v voltage reference
	movwf REFCON0

	;//Let the Oscillator stabilise
	btfss OSCCON,IOFS
	bra $-.2

	;Now wait for the LCD to startup
	call delay_16ms
	call delay_16ms
	call delay_16ms
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
	call delay_16ms
	decfsz WREG
 	bra startup_wait_loop

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

main_loop
	rcall REDRAW_LCD
	
	bra main_loop

;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;Here we fake an interrupt on change feature for the amp limit port
FAKE_RA3_IOC
	movf FLAGS,W
	bcf AMP_LIMIT_VAL
	btfsc AMP_LIMIT_PORT
	bsf AMP_LIMIT_VAL
	xorwf FLAGS,W 
	andlw AMP_LIMIT_VAL_MASK

	btfss STATUS,Z
	bsf INTCON,RABIF ;raise the interrupt
	return

;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;;;;;;;;;;;;;;;;LCD Redraw Function;;;;;;;;;;;;;;;;;;;;;;
Line_1a  db "V  ",0x0
Line_1b  db "A",0x0
Line_2  db "W",0x0
Line_3a db "EFuse:",0x0
Line_3b1 db "   Blown!",0x0
Line_3b2 db "   Normal"0x0

REDRAW_LCD
	rcall FAKE_RA3_IOC

	btfsc AMP_LIMIT_PORT
	movlw 0x01


	bcf LCD_CS
	call LCD_line_1
	movlw ' '
	btfss VOLT_LIMIT
	movlw b'01111110'
	call LCD_data_send

	rcall Volt_Read

	LCDPrintString Line_1a

	movlw ' '
	btfss AMP_LIMIT_VAL
	movlw b'01111110'
	call LCD_data_send

	rcall Amp_Read

	LCDPrintString Line_1b

	;Calculate the wattage
	movf VoltReading+0,w
	mulwf AmpReading+0

	movff PRODH,WattReading+1
	movff PRODL,WattReading+0

	movf VoltReading+1,w
	mulwf AmpReading+1
	movff PRODH,WattReading+3
	movff PRODL,WattReading+2

	;Now the cross bits
	movf VoltReading+0,w
	mulwf AmpReading+1
	movf PRODL,w
	addwf WattReading+1,f
	movf PRODH,w
	addwfc WattReading+2,f
	clrf WREG
	addwfc WattReading+3,f

	movf VoltReading+1,w
	mulwf AmpReading+0
	movf PRODL,w
	addwf WattReading+1,f
	movf PRODH,w
	addwfc WattReading+2,f
	clrf WREG
	addwfc WattReading+3,f

	;Now we have centi-V times centi Amps or 10^-4 A
	;We have 5 digits to display, and up to 300 W, so we would like centi-Watts
	;We must divide by 100

	movff WattReading+0,ADC_AVG+0
	movff WattReading+1,ADC_AVG+1
	movff WattReading+2,ADC_AVG+2

	;Shift right 3 times
	bcf STATUS,C
	rrcf WattReading+2,f
	rrcf WattReading+1,f
	rrcf WattReading+0,f
	bcf STATUS,C
	rrcf WattReading+2,f
	rrcf WattReading+1,f
	rrcf WattReading+0,f
	bcf STATUS,C
	rrcf WattReading+2,f
	rrcf WattReading+1,f
	rrcf WattReading+0,f

	;Add Temporary
	movf 	ADC_AVG+0,w
	addwf	WattReading+0,f
	movf 	ADC_AVG+1,w
	addwfc	WattReading+1,f
	movf 	ADC_AVG+2,w
	addwfc	WattReading+2,f

	;Shift right 2 times
	bcf STATUS,C
	rrcf WattReading+2,f
	rrcf WattReading+1,f
	rrcf WattReading+0,f
	bcf STATUS,C
	rrcf WattReading+2,f
	rrcf WattReading+1,f
	rrcf WattReading+0,f

	;Add Temporary
	movf 	ADC_AVG+0,w
	addwf	WattReading+0,f
	movf 	ADC_AVG+1,w
	addwfc	WattReading+1,f
	movf 	ADC_AVG+2,w
	addwfc	WattReading+2,f

	;shift right 7 times
	rlcf WattReading+0,f
	rlcf WattReading+1,f
	rlcf WattReading+2,f

	movff WattReading+1,WattReading+0
	movff WattReading+2,WattReading+1
	;movff WattReading+3,WattReading+2

	;Now write it on the screen
	call LCD_line_2

	movff WattReading,BIN_L
	movff WattReading+1,BIN_H

	call	BIN_to_BCD
	movf	R0,W
	call	W_1HEX_to_LCD
	movf	R1,W
	call	W_2HEX_to_LCD
	movlw   '.'
	call    LCD_data_send
	movf	R2,W
	call	W_2HEX_to_LCD


	LCDPrintString Line_2

	call LCD_line_3
	LCDPrintString Line_3a

	movlw 'N'
	btfsc EFuse_ON
	movlw 'Y'
	call LCD_data_send

	btfss ErrorState
	bra EFuse_AOK
	;EFuse has blown!
	LCDPrintString Line_3b1
	bsf LCD_CS
	return

EFuse_AOK
	;EFuse is ok
	LCDPrintString Line_3b2
	bsf LCD_CS
	return


;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;ADC Read Function
ADC_READ
	bsf ADCON0,GO

ADC_READ_LOOP
	rcall FAKE_RA3_IOC
	btfsc ADCON0,GO
	bra ADC_READ_LOOP

	return

#include "24bitMacros.inc"

 cblock
ADC_AVG:.3, ADC_Counter
 endc

;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;ADC Read Function
ADC_FULL_READ
	clrf ADC_Counter
	clrf ADC_AVG
	clrf ADC_AVG+.1
	clrf ADC_AVG+.2

ADC_FULL_READ_LOOP
	rcall ADC_READ
	add_16bit_to_24bit ADRESL,ADC_AVG
	incfsz ADC_Counter,f
	bra ADC_FULL_READ_LOOP

	;Now multiply depending on the voltage reference used
	bcf STATUS,C
	rlcf ADC_AVG,f
	rlcf ADC_AVG+.1,f
	rlcf ADC_AVG+.2,f

ADC_FULL_READ_END
	movff ADC_AVG+.1,ADC_AVG
	movff ADC_AVG+.2,ADC_AVG+.1

	;Reset the vref of the ADC to vss
	bcf ADCON1,3
	bcf ADCON1,2

	return

;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;ADC read voltage

Volt_Read
	;Select the Voltage line
	movlw b'00010001'
	movwf ADCON0
	
	rcall ADC_READ
	;Check and set the state of the transformer tappings
	movf ADRESH,W

	btfsc STATUS,Z
	bcf TRANSFORMER_RELAY

	andlw b'11111110'

	btfss STATUS,Z
	bsf TRANSFORMER_RELAY ;Voltage is less than 

	;Now obtain an averaged reading
	rcall ADC_FULL_READ

	movff ADC_AVG+0, VoltReading+0
	movff ADC_AVG+1, VoltReading+1

	;Now scale the result to the calibration curve V=0.049 + 0.02014 * ADC_AVG

	;shift right 6 times
	bcf STATUS,C
	rlcf	VoltReading+0, f
	rlcf	VoltReading+1, f
	bcf STATUS,C
	rlcf	VoltReading+0, f
	rlcf	VoltReading+1, f
	movff VoltReading+1,VoltReading+0
	clrf VoltReading+1
	
	;Shift temporary left 1 once
	bcf STATUS,C
	rlcf	ADC_AVG+0, f
	rlcf	ADC_AVG+1, f
	
	;Add together (2 + 1/64) = 2.015625 (error = 0.0806%)
	movf 	ADC_AVG+0,w
	addwf	VoltReading+0,f
	movf 	ADC_AVG+1,w
	addwfc	VoltReading+1,f

	;Now add the offset of ~50mv (again, no real point here)
	;movlw .1
	;addwf VoltReading+0,f
	;clrf WREG
	;addwfc VoltReading+1,f

	movff VoltReading,BIN_L
	movff VoltReading+1,BIN_H

	call	BIN_to_BCD
	movf	R1,W
	call	W_2HEX_to_LCD
	movlw   '.'
	call    LCD_data_send
	movf	R2,W
	call	W_2HEX_to_LCD

	return

;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;ADC read amperage
Amp_Read
	;Select the Voltage line
	movlw b'00010101'
	movwf ADCON0
	
	;Read and check the range
	rcall ADC_FULL_READ
	
	movff ADC_AVG+0, AmpReading+0
	movff ADC_AVG+1, AmpReading+1

	;Shift right once
	bcf STATUS,C
	rrcf	AmpReading+1, f
	rrcf	AmpReading+0, f

	;Add together
	movf 	ADC_AVG+0,w
	addwf	AmpReading+0,f
	movf 	ADC_AVG+1,w
	addwfc	AmpReading+1,f

	;Shift right three times
	rrcf	AmpReading+1, f
	rrcf	AmpReading+0, f
	bcf STATUS,C
	rrcf	AmpReading+1, f
	rrcf	AmpReading+0, f
	bcf STATUS,C
	rrcf	AmpReading+1, f
	rrcf	AmpReading+0, f

	;Subtract temporary
	movf 	ADC_AVG+0,w
	subwf	AmpReading+0,f
	movf 	ADC_AVG+1,w
	subwfb	AmpReading+1,f
	

	;Shift right three times
	rlcf    AmpReading+1, w
	rrcf	AmpReading+1, f
	rrcf	AmpReading+0, f
	rlcf    AmpReading+1, w
	rrcf	AmpReading+1, f
	rrcf	AmpReading+0, f
	rlcf    AmpReading+1, w
	rrcf	AmpReading+1, f
	rrcf	AmpReading+0, f

	;Add temporary
	movf 	ADC_AVG+0,w
	addwf	AmpReading+0,f
	movf 	ADC_AVG+1,w
	addwfc	AmpReading+1,f
	
	;shift right twice
	bcf STATUS,C
	rrcf	AmpReading+1, f
	rrcf	AmpReading+0, f
	bcf STATUS,C
	rrcf	AmpReading+1, f
	rrcf	AmpReading+0, f

	;Add temporary
	movf 	ADC_AVG+0,w
	addwf	AmpReading+0,f
	movf 	ADC_AVG+1,w
	addwfc	AmpReading+1,f

	;shift right once
	rrcf	AmpReading+1, f
	rrcf	AmpReading+0, f

	;Add the offset of 10 mA (no real need to add it, it's a little misleading!
	;movlw .1
	;addwf AmpReading+0,f
	;clrf WREG
	;addwfc AmpReading+1,f

	movff AmpReading,BIN_L
	movff AmpReading+1,BIN_H

	call	BIN_to_BCD
	movf	R1,W
	call	W_2HEX_to_LCD
	movlw   '.'
	call    LCD_data_send
	movf	R2,W
	call	W_2HEX_to_LCD
	return

 end
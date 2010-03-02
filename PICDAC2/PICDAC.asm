;PICDAC
	include "P18F1320.INC"

 PROCESSOR 18F1320
	CONFIG  OSC=HSPLL,FSCM=OFF,IESO=OFF,PWRT=OFF,BOR=OFF,WDT=OFF,MCLRE=ON,LVP=OFF

 org 0x000000
	goto INIT

 ;High priority interrupts (communication)
 org 0x000008
	goto process_serial_data



 ;Low priority interrupt (ticker for DAC)
 org 0x000018
	;call sine_table
	call sine_interp

	swapf WREG,W
	movwf DAC_BYTE_H1
	andlw 0xF0
	movwf DAC_BYTE_L1

	call TRANSMIT_DAC_WORD

    ;Make a frequency step. Note, the frequency step doesn't necessarily have to be
    ;constant. E.g. it could be changed as part of a PLL algorithm.

    movf    fstep_lo,W
    addwf   f_lo,F
    movf    fstep_hi,W
    addwfc   f_hi,F

	;Reset the timer interrupt
	bcf PIR1,TMR2IF
	retfie
	

 cblock 0x00
	f_hi,f_lo
	fstep_lo,fstep_hi
 endc

	;//////Sine wave generators
	include "sineInterpolate.inc"
	include "sineTable.inc"

	;//////Command processor
	include "serialControl.inc"

	;//////DAC SPI controller
	#define DAC_CS LATA,1
	#define DAC_CLK LATA,0
	#define DAC_SDI1 LATB,3
	#define DAC_SDI2 LATB,2
	#define DAC_nLDAC LATA,4
	include "DACSPI.inc"

INIT
	;Setup the pins for the DAC's
	bcf TRISA,1
	bcf TRISA,0
	bcf TRISA,4
	bcf TRISB,2
	bcf TRISB,3

	call DAC_init

	call serial_control_init

	;Setup the counter
    ;Initialize the step in frequency
    MOVLW   b'00010000'
    MOVWF   fstep_lo        
    CLRF    fstep_hi
    
    CLRF    f_hi            ;Start off at 0 degrees
    CLRF    f_lo

	;Setup the timer
	movlw b'00000100' ;[6:3] postscale, [2]tmr on, [1:0] prescaler
	movwf T2CON
	
	;40Mhz clock, 10Mhz tick rate, allow 250 cycles per update to give a 40kHz sampling rate
	movlw .249
	movwf PR2

	bsf PIE1,TMR2IE ;Disable timer interrupts (DAC paused)
	bcf IPR1,TMR2IP ;Make it low priority

	;Enable the high and low priority interrupts
	bsf RCON,IPEN

	;Enable high priority interrupts (communication)
	bsf INTCON,GIEH

	;Enable low priority interrupts for now (DAC timing)
	bsf INTCON,GIEL

MAIN
	btfsc INTCON,TMR0IE
	bra MAIN

	;/DAC is paused! do anything you like

	bra MAIN
end
;PICDAC
	include "P18F1320.INC"

 PROCESSOR 18F1320
	CONFIG  OSC=HSPLL,FSCM=OFF,IESO=OFF,PWRT=OFF,BOR=OFF,WDT=OFF,MCLRE=ON,LVP=OFF

 ;///////////////////////////////VARIABLES
 cblock 0x00
	f_upper,f_high,f_low
	fstep_upper,fstep_high,fstep_low
	mid_DAC
 endc


 org 0x000000
	goto INIT

 ;High priority interrupts (communication)
 org 0x000008
	goto process_serial_data



 ;Low priority interrupt (ticker for DAC)
 org 0x000018
	bsf mid_DAC,0
	call sine_table
	;call sine_interp

	swapf WREG,W
	movwf DAC_BYTE_H1
	andlw 0xF0
	movwf DAC_BYTE_L1

	call sine_table

	swapf WREG,W
	movwf DAC_BYTE_H2
	andlw 0xF0
	movwf DAC_BYTE_L2

	call TRANSMIT_DAC_WORD

    ;Make a frequency step. Note, the frequency step doesn't necessarily have to be
    ;constant. E.g. it could be changed as part of a PLL algorithm.

    movf    fstep_low,W
    addwf  f_low,F
    movf    fstep_high,W
    addwfc   f_high,F
    movf    fstep_upper,W
    addwfc   f_upper,F

	;Reset the timer interrupt
	bcf PIR1,TMR2IF
	clrf mid_DAC
	retfie
	
	;//////Sine wave generators
	include "sineInterpolate.inc"
	include "sineTable.inc"


	;//////DAC SPI controller
	#define DAC_CS LATA,1
	#define DAC_CLK LATA,0
	#define DAC_SDI1 LATB,3
	#define DAC_SDI2 LATB,2
	#define DAC_nLDAC LATA,4
	include "DACSPI.inc"

	;//////Command processor
	include "serialControl.inc"

INIT
	;Setup the pins for the DAC's
	bcf TRISA,1
	bcf TRISA,0
	bcf TRISA,4
	bcf TRISB,2
	bcf TRISB,3

	call DAC_init

	call serial_control_init


	;Setup the timer
	movlw b'00000100' ;[6:3] postscale (off), [2]tmr on, [1:0] prescaler (off)
	movwf T2CON
	
	;40Mhz clock, 10Mhz tick rate
	;To simplify the calculation of increment size given a freq, we use 239 steps here
	;Sample rate is now 41841.00418
	movlw .238
	movwf PR2
	;Now to calculate the step size for a given freq, just multiply the freqency by 100.2438656
	;The frequency can then be adjusted in the 100th's by just adding 1-100.
	
	;Setup the counter and step size
    ;Initialize the step in frequency for a 20mhz wave
	movlw  0xD5
	movwf  fstep_low
    MOVLW  0x07
    MOVWF   fstep_high     
    CLRF    fstep_upper
	
	;Start off at 0 degrees in phase
	CLRF  f_low    
    CLRF  f_upper            
    CLRF  f_high

	;Clear the mid_DAC flag that makes sure DAC commands don't overlap
	clrf mid_DAC

	;////Interrupts
	bsf PIE1,TMR2IE ;Disable timer interrupts (DAC paused)
	bcf IPR1,TMR2IP ;Make it low priority

	;Enable the high and low priority interrupts
	bsf RCON,IPEN

	;Enable high priority interrupts (communication)
	bsf INTCON,GIEH

	;Enable low priority interrupts for now (DAC timing)
	bsf INTCON,GIEL

OUTPUT_LOOP
	btfsc PIE1,TMR2IE
	bra OUTPUT_LOOP

	;/DAC is paused! do anything you like but disable the low priority interrupts first
	bcf INTCON,GIEL

	;Zero the DAC outputs first
	clrf DAC_BYTE_L1
	clrf DAC_BYTE_H1
	clrf DAC_BYTE_L2
	clrf DAC_BYTE_H2
	call TRANSMIT_DAC_WORD
	
	;Re-enable the low priority interrupts
	bsf INTCON,GIEL
PAUSE_LOOP
	btfss PIE1,TMR2IE
	bra PAUSE_LOOP

	bra OUTPUT_LOOP
end
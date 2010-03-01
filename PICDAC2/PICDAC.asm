;PICDAC
	include "P18F1320.INC"

 PROCESSOR 18F1320
	CONFIG  OSC=HSPLL,FSCM=OFF,IESO=OFF,PWRT=OFF,BOR=OFF,WDT=OFF,MCLRE=ON,LVP=OFF

 org 0x000000
	goto INIT

 ;High priority interrupts (communication
 org 0x000008
	reset

 ;Low priority interrupt (ticker for DAC)
 org 0x000018
	;Clear the flag of the timer
	bcf INTCON,TMR0IF

	;Next tick of the function clock
	call Sine_interp_tick
	retfie
	

	include "sine.inc"


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
	call Sine_interp_init

	;Setup the timer
	;bsf T0CON,TMR0ON ;Timer on
	;bsf T0CON,T08BIT ;8-bit mode
	;bcf T0CON,T0CS ;Timer gets its clock internally
	;bsf T0CON,PSA  ;Turn off prescaler
	
	;Set the prescaler to 1:2
	;movlw b'11111000'
	;andwf T0CON,F

	;bsf INTCON,TMR0IE ;Enable timer interrupts
	;bcf INTCON2,TMR0IP ;Make it low priority

	;Enable the high and low priority interrupts
	;bsf RCON,IPEN

	;Enable high priority interrupts (communication)
	;bsf INTCON,GIEH

	;Disable low priority interrupts for now (DAC timing and transmission)
	;bsf INTCON,GIEL

MAIN
	call Sine_interp_tick
	bra MAIN

end
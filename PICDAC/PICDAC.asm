;PICDAC
	include "P18F1320.INC"

 PROCESSOR 18F1320
    __CONFIG  _CONFIG1H, _IESO_OFF_1H & _FSCM_OFF_1H & _INTIO2_OSC_1H
    __CONFIG  _CONFIG2L, _PWRT_OFF_2L & _BOR_OFF_2L & _BORV_27_2L
    __CONFIG  _CONFIG2H, _WDT_OFF_2H & _WDTPS_32K_2H
    __CONFIG  _CONFIG3H, _MCLRE_ON_3H
    __CONFIG  _CONFIG4L, _DEBUG_OFF_4L & _LVP_OFF_4L & _STVR_ON_4L

    __CONFIG  _CONFIG5L, _CP0_OFF_5L & _CP1_OFF_5L
    __CONFIG  _CONFIG5H, _CPB_OFF_5H & _CPD_OFF_5H
    __CONFIG  _CONFIG6L, _WRT0_OFF_6L & _WRT1_OFF_6L
    __CONFIG  _CONFIG6H, _WRTC_OFF_6H & _WRTB_OFF_6H & _WRTD_OFF_6H
    __CONFIG  _CONFIG7L, _EBTR0_OFF_7L & _EBTR1_OFF_7L
    __CONFIG  _CONFIG7H, _EBTRB_OFF_7H

 org 0x0000
	goto INIT

	#define DAC_CS LATB,5
	#define DAC_CLK LATA,3
	#define DAC_SDI LATB,0
	#define DAC_nLDAC LATB,1

INIT
	movlw b'01110000'
	movwf OSCCON
	clrf TRISA
	clrf TRISB
	
	;Reset data entry for the DAC
	bsf DAC_CS
	
	;Make the DAC auto latch
	bcf DAC_nLDAC

	clrf DAC_BYTE_H
	clrf DAC_BYTE_L

MAIN
	infsnz DAC_BYTE_L,F
	incf DAC_BYTE_H,F
skipinc
	call TRANSMIT_DAC_WORD
	bra MAIN


;////////////////////////////////////
 cblock
DAC_BYTE_L,DAC_BYTE_H
 endc
TRANSMIT_DAC_WORD
	;Prime the clock
	bcf DAC_CLK
	;CS down for data send
	bcf DAC_CS

	movf DAC_BYTE_H,W
	andlw b'00001111'
	iorlw b'01110000'
	rcall TRANSMIT_DAC_BYTE

	movf DAC_BYTE_L,W
	rcall TRANSMIT_DAC_BYTE

	;Latch the data in	
	bsf	DAC_CS
	return
;////////////////////////////////////
TRANSMIT_DAC_BYTE
	;Make the carry bit a 1
	bsf STATUS,C
	rlcf WREG
DAC_loop
	bsf	DAC_SDI
	btfss STATUS,C
	bcf	DAC_SDI

	bsf	DAC_CLK
 	bcf	DAC_CLK

	bcf STATUS,C
	rlcf WREG
	bnz	DAC_loop
	return

end
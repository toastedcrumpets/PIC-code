;PICDAC
	include "P18F1320.INC"

 PROCESSOR 18F1320
    __CONFIG  _CONFIG1H, _IESO_OFF_1H & _FSCM_OFF_1H & _HSPLL_OSC_1H
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

	#define DAC_CS LATA,1
	#define DAC_CLK LATA,0
	#define DAC_SDI1 LATB,2
	#define DAC_SDI2 LATB,3
	#define DAC_nLDAC LATA,4

INIT
	clrf TRISA
	clrf TRISB
	
	;Reset data entry for the DAC
	bsf DAC_CS
	
	;Make the DAC auto latch
	bcf DAC_nLDAC

	clrf DAC_BYTE_H1
	clrf DAC_BYTE_L1
	
	clrf DAC_BYTE_L2
	movlw b'00001000'
	movwf DAC_BYTE_H2
	;clrf DAC_BYTE_L2
MAIN
	infsnz DAC_BYTE_L1,F
	incf DAC_BYTE_H1,F

	infsnz DAC_BYTE_L2,F
	incf DAC_BYTE_H2,F

	call TRANSMIT_DAC_WORD
	bra MAIN


;////////////////////////////////////
 cblock
DAC_BYTE_L1,DAC_BYTE_H1
DAC_BYTE_L2,DAC_BYTE_H2
DAC_OPP
 endc
TRANSMIT_DAC_WORD
	;Prime the clock
	bcf DAC_CLK
	;CS down for data send
	bcf DAC_CS

	movf DAC_BYTE_H2,W
	andlw b'00001111'
	iorlw b'01110000'
	movwf DAC_OPP

	movf DAC_BYTE_H1,W
	andlw b'00001111'
	iorlw b'01110000'
	rcall TRANSMIT_DAC_BYTE

	movff DAC_BYTE_L2,DAC_OPP
	movf DAC_BYTE_L1,W
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
	bsf	DAC_SDI1
	bsf	DAC_SDI2

	btfss STATUS,C
	bcf	DAC_SDI1

	btfss DAC_OPP,7
	bcf	DAC_SDI2

	bsf	DAC_CLK
 	bcf	DAC_CLK

	rlncf DAC_OPP
	bcf STATUS,C
	rlcf WREG
	bnz	DAC_loop
	return

end
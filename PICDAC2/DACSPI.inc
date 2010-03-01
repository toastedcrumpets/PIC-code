;DAC SPI control
 cblock
DAC_BYTE_L1,DAC_BYTE_H1
DAC_BYTE_L2,DAC_BYTE_H2
DAC_OPP
 endc

;////////////////////////////////////
DAC_init
	;Reset data entry for the DAC
	bsf DAC_CS
	
	;Make the DAC auto latch
	bcf DAC_nLDAC

	clrf DAC_BYTE_H1
	clrf DAC_BYTE_L1
	
	clrf DAC_BYTE_L2
	clrf DAC_BYTE_H2
	return

;////////////////////////////////////
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
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
;The loop is unrolled and the whole function inlined for maximum speed
TRANSMIT_DAC_BYTE macro
	local bitnum = .8
	while bitnum != .0
		bsf	DAC_SDI1
		bsf	DAC_SDI2

		btfss WREG,#v(bitnum)-.1
		bcf	DAC_SDI1

		btfss DAC_OPP,#v(bitnum)-.1
		bcf	DAC_SDI2

		bsf	DAC_CLK
 		bcf	DAC_CLK
bitnum = bitnum - .1
	endw
	endm

TRANSMIT_DAC_NIBBLE macro
	local bitnum = .4
	while bitnum != .0
		bsf	DAC_SDI1
		bsf	DAC_SDI2

		btfss WREG,#v(bitnum)-.1
		bcf	DAC_SDI1

		btfss DAC_OPP,#v(bitnum)-.1
		bcf	DAC_SDI2

		bsf	DAC_CLK
 		bcf	DAC_CLK
bitnum = bitnum - .1
	endw
	endm

;////////////////////////////////////
TRANSMIT_DAC_WORD
	;Prime the clock
	bcf DAC_CLK
	;CS down for data send
	bcf DAC_CS

	movff DAC_BYTE_H2,DAC_OPP
	movf DAC_BYTE_H1,W
	;Now transmit 0
	bcf	DAC_SDI1
	bcf	DAC_SDI2

	bsf	DAC_CLK
 	bcf	DAC_CLK

	;Then 111
	bsf	DAC_SDI1
	bsf	DAC_SDI2

	bsf	DAC_CLK
	bcf	DAC_CLK
	bsf	DAC_CLK
	bcf	DAC_CLK
	bsf	DAC_CLK
	bcf	DAC_CLK

	;Transmit the final 4 bytes
	TRANSMIT_DAC_NIBBLE

	movff DAC_BYTE_L2,DAC_OPP
	movf DAC_BYTE_L1,W
	TRANSMIT_DAC_BYTE

	;Latch the data in	
	bsf	DAC_CS
	return


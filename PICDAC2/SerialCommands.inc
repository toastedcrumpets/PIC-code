;Command functions

#define jump(_function) db UPPER(_function),HIGH(_function),LOW(_function),0x00

 cblock
cmd_timeout
 endc

;///////////////////////////////////////////////////////////////
;Command table
#define NCommands .6
Command_table
	jump(cmd_0_output_toggle)
	jump(cmd_1_freq_down)
	jump(cmd_2_freq_up)
	jump(cmd_3_recv_step)
	jump(cmd_4_recv_DAC)
	jump(cmd_5_output_off)

;///////////////////////////////////////////////////////////////
;A method to wait for the next byte
command_wait_next_byte
	;Wait 255 loops for the next byte 
	movlw 0xFF
	movwf cmd_timeout
command_wait_next_byte_loop
	nop
	nop	
	dcfsnz cmd_timeout
	retlw .1
	
	btfss PIR1,RCIF
	bra command_wait_next_byte_loop

	retlw .0

;///////////////////////////////////////////////////////////////
cmd_0_output_toggle
	;Toggle the timer interrupt
	btg PIE1,TMR2IE 

	retfie FAST

;///////////////////////////////////////////////////////////////
cmd_1_freq_down
	;Go down two steps to check for zero
	movlw .2
	subwf fstep_low,F
	movlw .0
	subwfb fstep_high,F
	subwfb fstep_upper,F
	
	btfsc STATUS,C
	bra cmd_2_freq_up

	;It's wrapped, just clear the registers
	clrf fstep_low
	clrf fstep_high
	clrf fstep_upper
	
	;Now we let it continue to the increment command

;///////////////////////////////////////////////////////////////
cmd_2_freq_up
	incfsz fstep_low
	bra cmd_2_freq_up_ret

	infsnz fstep_high
	incf   fstep_upper

cmd_2_freq_up_ret
	retfie FAST



;///////////////////////////////////////////////////////////////
cmd_3_recv_step
	;Get the next 3 bytes as the step value and retfie on failure
	call command_wait_next_byte
	TSTFSZ WREG
	retfie	

	movf RCREG,fstep_low
	bcf PIR1,RCIF

	call command_wait_next_byte
	TSTFSZ WREG
	retfie	

	movff RCREG,fstep_high
	bcf PIR1,RCIF

	call command_wait_next_byte
	TSTFSZ WREG
	retfie	

	movff RCREG,fstep_upper
	bcf PIR1,RCIF

	retfie

;///////////////////////////////////
cmd_4_recv_DAC
	;Check the DAC isn't doing something
	TSTFSZ mid_DAC
	retfie

	;Start the data transfer to the DAC
	;Prime the clock
	bcf DAC_CLK
	;CS down for data send
	bcf DAC_CS

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

	;Load the high nibbles, First check for data
	movlw 0xFF
	dcfsnz WREG
	bra cmd_4_recv_DAC_finish	
	btfss PIR1,RCIF
	bra $-.6

	movff RCREG,DAC_OPP
	bcf PIR1,RCIF
	swapf DAC_OPP,W

	;And send them
	TRANSMIT_DAC_NIBBLE

	;Now load the low bytes
	movlw 0xFF
	dcfsnz WREG
	bra cmd_4_recv_DAC_finish	
	btfss PIR1,RCIF
	bra $-.6
	
	movff RCREG,DAC_BYTE_L1
	bcf PIR1,RCIF

	movlw 0xFF
	dcfsnz WREG
	bra cmd_4_recv_DAC_finish	
	btfss PIR1,RCIF
	bra $-.6
	
	;Send the lower bytes
	movff RCREG,DAC_OPP
	bcf PIR1,RCIF

	movf DAC_BYTE_L1,W
	TRANSMIT_DAC_BYTE

	;Latch the data in/end the transmission
cmd_4_recv_DAC_finish
	bsf	DAC_CS
	retfie

;///////////////////////////////////////////////////////////////
cmd_5_output_off
	;Toggle the timer interrupt off
	bcf PIE1,TMR2IE
	retfie FAST
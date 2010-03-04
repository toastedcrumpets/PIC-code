;Command functions

#define jump(_function) db UPPER(_function),HIGH(_function),LOW(_function),0x00

 cblock
cmd_timeout
 endc

;///////////////////////////////////////////////////////////////
;Command table
#define NCommands .4
Command_table
	jump(cmd_0_output_toggle)
	jump(cmd_1_freq_down)
	jump(cmd_2_freq_up)
	jump(cmd_3_recv_step)

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
	btg INTCON,TMR0IE

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

	bcf PIR1,RCIF
	movf RCREG,W
	movwf fstep_low

	call command_wait_next_byte
	TSTFSZ WREG
	retfie	

	bcf PIR1,RCIF
	movf RCREG,W
	movwf fstep_high

	call command_wait_next_byte
	TSTFSZ WREG
	retfie	

	bcf PIR1,RCIF
	movf RCREG,W
	movwf fstep_upper

	retfie
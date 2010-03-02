;Command functions

#define jump(_function) db UPPER(_function),HIGH(_function),LOW(_function),0x00

#define NCommandsPlusOne .3
;Command table
Command_table
	jump(cmd_0_output_toggle)
	jump(cmd_1_freq_down)
	jump(cmd_2_freq_up)

cmd_0_output_toggle
	;Toggle the timer interrupt
	btg PIE1,TMR2IE 
	btg INTCON,TMR0IE

	retfie FAST

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

cmd_2_freq_up
	incfsz fstep_low
	bra cmd_2_freq_up_ret

	infsnz fstep_high
	incf   fstep_upper

cmd_2_freq_up_ret
	retfie FAST

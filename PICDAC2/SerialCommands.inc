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
	subwf fstep_lo
	movlw .0
	subwfb fstep_hi
	
	btfsc STATUS,C
	bra cmd_1_freq_down_ok

	;It's wrapped, just clear the registers
	clrf fstep_lo
	clrf fstep_hi

cmd_1_freq_down_ok
	infsnz fstep_lo
	incf fstep_hi

	retfie FAST

cmd_2_freq_up
	movlw .1
	addwf fstep_lo
	movlw .0
	addwfc fstep_hi
	retfie FAST

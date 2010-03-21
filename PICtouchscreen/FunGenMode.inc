;FuncGen mode for the touch screen

#include "NumericalInput.inc"

FuncGen_Mode_output_str db "On/Off",0x00
FuncGen_Mode_FreqSync_str db "FreqSync",0x00

FuncGen_Mode_setup
	;////////////////////////////////////
	;Setup the UART to the FuncGen device
	;Enable the UART
	BSF RCSTA,SPEN
	BCF TXSTA,SYNC 	;Asynchronous mode
	
	;9bit mode
	BSF RCSTA,RX9
	BSF TXSTA,TX9

	;//Baud rate
	;High speed
	BSF TXSTA,BRGH
	;16bit clocking
	BSF BAUDCON,BRG16

	;115.2k serial line
	clrf SPBRGH
	movlw .14
	movwf SPBRG

	;//Transmit Mode On
	BSF TXSTA,TXEN
	;//Recieve Mode On
	BSF RCSTA,CREN
	return

;//////////////////////////////////////
;Sets up the touch panel for FuncGen mode
FuncGen_Mode_Init
	;Prepare the display
	call FuncGen_Mode_redraw

	;Fill the screen with touch handlers for FuncGen
	call clear_TouchMenu

	movlw .2
	movwf Insert_Touch_var_y
	movlw .0
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N FuncGen_FreqSync_Handler,.3

	movlw .6
	movwf Insert_Touch_var_y
	movlw .13
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N FuncGen_Output_Handler,.3

	movlw .4
	movwf Insert_Touch_var_y
	movlw .13
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N Main_Mode_Init,.3

	call add_numerical_input_touch

	call touch_wait_for_release

	return 

;///////////////////////////////////////
;Draws the menu buttons
FuncGen_Mode_redraw
	call blank_framebuffer

	movlw .0
	movwf BIN_hi
	movlw .90
	movwf BIN_lo

	;//Convert the binary Decimal value to BCD
	call b16_d5

	call draw_numerical_input

	call Load_framebuffer
	movlw .2
	call fb_goto_row
	movlw .0
	call fb_goto_col
	LoadTable(FuncGen_Mode_FreqSync_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .6
	call fb_goto_row
	movlw .101
	call fb_goto_col
	LoadTable(FuncGen_Mode_output_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .4
	call fb_goto_row
	movlw .107
	call fb_goto_col
	LoadTable(Mode_back_str)
	call blit_box_print_table_string
	return

;//////////////////////////////////////////////
;Function generator mode handlers

FuncGen_Output_Handler
	btfss PIR1,TXIF
	bra $-.2
	
	;It's a command so bit9 is on
	bsf TXSTA,TX9
	movlw b'11110000'
	movwf TXREG

	;Make sure this isn't toggled multiple times
	call touch_wait_for_release
	return

FuncGen_FreqSync_Handler
	;Calculate the value to send, 16bits in BIN_L and BIN_H 
	call generate_DAC_tick_rate

	btfss PIR1,TXIF
	bra $-.2
	
	;It's a command so bit9 is on
	bsf TXSTA,TX9
	movlw b'11000011'
	movwf TXREG

	;Load the next byte then wait for an empty transmission buffer
	movf L_tmp,W
	btfss PIR1,TXIF
	bra $-.2
	movwf TXREG
	
	movf H_tmp,W
	btfss PIR1,TXIF
	bra $-.2
	movwf TXREG

	movf U_tmp,W
	btfss PIR1,TXIF
	bra $-.2
	movwf TXREG

	;Make sure this isn't toggled multiple times
	call touch_wait_for_release
	return

generate_DAC_tick_rate
	movff BIN_lo,L_tmp
	movff BIN_hi,H_tmp
	clrf U_tmp

	;To multiply by 100.2438656
	;We do
	;input * (((2 + 1)* 8 + 1) * 4 + 1/2 -1/4) ;Which gives a very good approximation while still using the highest bits
	;But for higher values we also do
	; 
	;The last bit can be broken down into a successive approximation
	;(-1/2+1/4

	;Section for ((input * 2 + input)* 8 + input) * 4
	x2_24bit L_tmp
	
	movf BIN_lo,W
	addwf L_tmp
	movf BIN_hi,W
	addwfc H_tmp
	movlw .0
	addwfc U_tmp

	x2_24bit L_tmp
	x2_24bit L_tmp
	x2_24bit L_tmp

	movf BIN_lo,W
	addwf L_tmp
	movf BIN_hi,W
	addwfc H_tmp
	movlw .0
	addwfc U_tmp

	x2_24bit L_tmp
	x2_24bit L_tmp

	movff BIN_hi,PRODH
	movff BIN_lo,PRODL
	
	;Generate +input/2
	bcf STATUS,C
	rrcf PRODH,F
	rrcf PRODL,F

	movf PRODL,W
	addwf L_tmp,F
	movf PRODH,W
	addwfc H_tmp,F
	movlw .0
	addwfc U_tmp,F

	;Generate -input/4
	bcf STATUS,C
	rrcf PRODH,F
	rrcf PRODL,F

	movf PRODL,W
	subwf L_tmp,F
	movf PRODH,W
	subwfb H_tmp,F
	movlw .0
	subwfb U_tmp,F	

	;Generate -1/256-1/512
	movf BIN_hi,W
	rcall generate_DAC_tick_rate_sub
	rcall generate_DAC_tick_rate_sub2
	return

generate_DAC_tick_rate_sub2
	bcf STATUS,C
	rrcf BIN_hi,W
generate_DAC_tick_rate_sub
	subwf L_tmp,F
	movlw .0
	subwfb H_tmp,F
	movlw .0
	subwfb U_tmp,F	
	return
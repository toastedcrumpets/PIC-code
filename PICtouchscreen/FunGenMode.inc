;FuncGen mode for the touch screen

FuncGen_Mode_output_str db "output",0x00

FuncGen_Mode_FreqDOWN_str db "Freq",.135,0x00
FuncGen_Mode_FreqUP_str db "Freq",.133,0x00

FuncGen_Mode_FreqSync_str db "FreqSync",0x00

FuncGen_Mode_setup
	return

;//////////////////////////////////////
;Sets up the touch panel for FuncGen mode
FuncGen_Mode_Init
	;Prepare the display
	call FuncGen_Mode_redraw

	;Fill the screen with touch handlers for FuncGen
	call clear_TouchMenu

	movlw .6
	movwf Insert_Touch_var_y
	movlw .0
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N FuncGen_FreqUP_Handler,.3

	movlw .4
	movwf Insert_Touch_var_y
	movlw .0
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N FuncGen_FreqDOWN_Handler,.3

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
	movlw .86
	movwf SPBRG

	;//Transmit Mode On
	BSF TXSTA,TXEN
	;//Recieve Mode On
	BSF RCSTA,CREN

	call touch_wait_for_release

	return 

;///////////////////////////////////////
;Draws the menu buttons
FuncGen_Mode_redraw
	call blank_framebuffer

	call Load_framebuffer
	movlw .6
	call fb_goto_row
	movlw .0
	call fb_goto_col
	LoadTable(FuncGen_Mode_FreqUP_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .4
	call fb_goto_row
	movlw .0
	call fb_goto_col
	LoadTable(FuncGen_Mode_FreqDOWN_str)
	call blit_box_print_table_string

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

FuncGen_FreqDOWN_Handler
	btfss PIR1,TXIF
	bra $-.2
	
	;It's a command so bit9 is on
	bsf TXSTA,TX9
	movlw b'11100001'
	movwf TXREG

	;Make sure this isn't toggled multiple times
	call touch_wait_for_release
	return

FuncGen_FreqUP_Handler
	btfss PIR1,TXIF
	bra $-.2
	
	;It's a command so bit9 is on
	bsf TXSTA,TX9
	movlw b'11010010'
	movwf TXREG

	;Make sure this isn't toggled multiple times
	call touch_wait_for_release
	return

FuncGen_FreqSync_Handler
	btfss PIR1,TXIF
	bra $-.2
	
	;It's a command so bit9 is on
	bsf TXSTA,TX9
	movlw b'11000011'
	movwf TXREG

	;Load the next byte then wait for an empty transmission buffer
	movlw 0x9A
	btfss PIR1,TXIF
	bra $-.2
	movwf TXREG
	
	movlw 0x0F
	btfss PIR1,TXIF
	bra $-.2
	movwf TXREG

	movlw 0x00
	btfss PIR1,TXIF
	bra $-.2
	movwf TXREG

	;Make sure this isn't toggled multiple times
	call touch_wait_for_release
	return
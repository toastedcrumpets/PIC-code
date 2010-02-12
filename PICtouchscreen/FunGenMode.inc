;FuncGen mode for the touch screen

FuncGen_Mode_send_str db "send",0x00

FuncGen_Mode_setup

	return

;//////////////////////////////////////
;Sets up the touch panel for FuncGen mode
FuncGen_Mode_Init
	;Prepare the display
	call FuncGen_Mode_redraw

	;Fill the screen with touch handlers for FuncGen
	call clear_TouchMenu

	movlw .7
	movwf Insert_Touch_var_y
	movlw .13
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N FuncGen_Handler,.3

	movlw .6
	movwf Insert_Touch_var_y
	movlw .13
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N Main_Mode_Init,.3

	call touch_wait_for_release

	return 

;///////////////////////////////////////
;Draws the menu buttons
FuncGen_Mode_redraw
	call blank_framebuffer
	call Load_framebuffer
	movlw .7
	call fb_goto_row
	movlw .107
	call fb_goto_col
	LoadTable(FuncGen_Mode_send_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .6
	call fb_goto_row
	movlw .107
	call fb_goto_col
	LoadTable(Mode_back_str)
	call blit_box_print_table_string
	return

;//////////////////////////////////////////////
;Draw Mode touch handlers

;/////////////
;This handler draws a pixel
FuncGen_Handler

	;Make sure this isn't toggled multiple times
	call touch_wait_for_release
	return
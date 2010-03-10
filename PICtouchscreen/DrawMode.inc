;Drawing mode for the touch screen

Drawing_Mode_clr_str db "clear",0x00

;//////////////////////////////////////
;Sets up the touch panel for drawing mode
Drawing_Mode_Init
	;Prepare the display
	call Drawing_Mode_redraw

	;Disable the standard func DAC output
	btfss PIR1,TXIF
	bra $-.2
	
	;It's a command so bit9 is on
	;Make the DAC turn output off
	bsf TXSTA,TX9
	movlw b'11110000'
	movwf TXREG

	;Fill the screen with touch handlers for drawing
	movlw LOW(Drawing_Handler)
	movwf fill_touch_menu_vec_L
	movlw HIGH(Drawing_Handler)
	movwf fill_touch_menu_vec_H
	movlw UPPER(Drawing_Handler)
	movwf fill_touch_menu_vec_U
	call fill_TouchMenu
	
	movlw .7
	movwf Insert_Touch_var_y
	movlw .13
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N Drawing_Mode_redraw,.3

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
Drawing_Mode_redraw
	call blank_framebuffer
	call Load_framebuffer
	movlw .7
	call fb_goto_row
	movlw .105
	call fb_goto_col
	LoadTable(Drawing_Mode_clr_str)
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
;This handler draws a pixel and outputs it to the DAC
Drawing_Handler

	;It's a command so bit9 is on, cmd 4 is recv DAC
	bsf TXSTA,TX9
	movlw b'10110100'
	movwf TXREG

	;First nibbles are blank
	clrf WREG
	btfss PIR1,TXIF
	bra $-.2
	movwf TXREG

	;Set the X and Y coords
	btfss PIR1,TXIF
	bra $-.2
	movff x_pos,TXREG

	btfss PIR1,TXIF
	bra $-.2
	movff y_pos,TXREG

	;Now draw it
	movff x_pos,pixel_x
	bcf STATUS,C
	rrcf pixel_x

	movff y_pos,pixel_y
	bcf STATUS,C
	rrcf pixel_y
	bcf STATUS,C
	rrcf pixel_y

	call fb_set_pixel
	return
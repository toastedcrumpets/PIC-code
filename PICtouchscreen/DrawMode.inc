;Drawing mode for the touch screen

Drawing_Mode_clr_str db "clear",0x00
Drawing_Mode_back_str db "back",0x00
;//////////////////////////////////////
;Sets up the touch panel for drawing mode
Drawing_Mode_Init
	;Prepare the display
	call Drawing_Mode_redraw

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
	LoadTable(Drawing_Mode_back_str)
	call blit_box_print_table_string
	return

;//////////////////////////////////////////////
;Draw Mode touch handlers

;/////////////
;This handler draws a pixel
Drawing_Handler
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
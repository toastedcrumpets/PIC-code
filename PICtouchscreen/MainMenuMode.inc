;Main mode for the touch screen

Main_Mode_title_str db "PICTouch Main Menu",0x00
Main_Mode_draw_str db .137," Draw",0x00
Main_Mode_funcG_str db .136," Func.",0x00
Main_Mode_Recal_str db .138," ReCal.",0x00
Main_Mode_SD_Mode_str db "SD card",0x00
Mode_back_str db "back",0x00
;//////////////////////////////////////
;Sets up the touch panel for Main mode
Main_Mode_Init
	;Prepare the display
	call Main_Mode_redraw

	;Fill the screen with touch handlers for Main
	call clear_TouchMenu
	
	movlw .5
	movwf Insert_Touch_var_y
	movlw .12
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N Drawing_Mode_Init,.4

	movlw .5
	movwf Insert_Touch_var_y
	movlw .0
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N SD_Mode_Init,.4

	movlw .3
	movwf Insert_Touch_var_y
	movlw .12
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N FuncGen_Mode_Init,.4

	movlw .1
	movwf Insert_Touch_var_y
	movlw .12
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N Menu_Recalibrate,.4

	call touch_wait_for_release

	return 

;///////////////////////////////////////
Menu_Recalibrate
	call touch_calibrate
	call Main_Mode_redraw
	return

;///////////////////////////////////////
;Draws the menu buttons
Main_Mode_redraw
	call blank_framebuffer
	call Load_framebuffer
	movlw .7
	call fb_goto_row
	movlw .26
	call fb_goto_col
	LoadTable(Main_Mode_title_str)
	call blit_print_table_string

	
	call Load_framebuffer
	movlw .5
	call fb_goto_row
	movlw .96
	call fb_goto_col
	LoadTable(Main_Mode_draw_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .3
	call fb_goto_row
	movlw .96
	call fb_goto_col
	LoadTable(Main_Mode_funcG_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .1
	call fb_goto_row
	movlw .96
	call fb_goto_col
	LoadTable(Main_Mode_Recal_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .5
	call fb_goto_row
	movlw .0
	call fb_goto_col
	LoadTable(Main_Mode_SD_Mode_str)
	call blit_box_print_table_string
	return

;//////////////////////////////////////////////
;Main Mode touch handlers
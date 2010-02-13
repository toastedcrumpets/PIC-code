;SD card menu mode

SD_Mode_fat_str db "FAT32",0x00
SD_Mode_Init_Fail db "Failed SD init",0x00
;//////////////////////////////////////
SD_Mode_Init
	;Prepare the display
	call SD_Mode_redraw

	
	;Prepare the SD card
	call Load_framebuffer
	LoadTable SD_Mode_Init_Fail
	call SD_init

	btfss STATUS,Z
	call blit_print_table_string

	;Fill the screen with blank touch handlers
	call clear_TouchMenu
	
	movlw .7
	movwf Insert_Touch_var_y
	movlw .13
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N SD_MODE_FAT,.3

	movlw .3
	movwf Insert_Touch_var_y
	movlw .13
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N Main_Mode_Init,.3

	call touch_wait_for_release

	return 

;///////////////////////////////////////
;Draws the menu buttons
SD_Mode_redraw
	call blank_framebuffer
	call Load_framebuffer
	movlw .7
	call fb_goto_row
	movlw .105
	call fb_goto_col
	LoadTable(SD_Mode_fat_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .3
	call fb_goto_row
	movlw .107
	call fb_goto_col
	LoadTable(Mode_back_str)
	call blit_box_print_table_string
	return

;//////////////////////////////////////
SD_MODE_FAT
	call SD_Mode_redraw
	call Load_framebuffer
	call FAT_init
	call touch_wait_for_release	
	return
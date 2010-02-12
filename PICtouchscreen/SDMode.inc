;SD card menu mode

SD_Mode_read_str db "Read",0x00
SD_Mode_write_str db "Write",0x00
SD_Mode_Init_Fail db "Failed to init card",0x00
SD_Mode_Read_Fail db "Failed to read card",0x00
SD_Mode_Write_Fail db "Failed to write card",0x00
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
	Insert_Touch_value_N SD_MODE_SEND_READ,.3

	movlw .5
	movwf Insert_Touch_var_y
	movlw .13
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N SD_MODE_SEND_WRITE,.3

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
	LoadTable(SD_Mode_read_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .5
	call fb_goto_row
	movlw .105
	call fb_goto_col
	LoadTable(SD_Mode_write_str)
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
SD_MODE_SEND_READ
	clrf SD_addr+2
	clrf SD_addr+1
	clrf SD_addr

	call SD_read
	xorlw .0
	bz SD_MODE_SEND_READ_Sucess

	call Load_framebuffer
	call W_2HEX_to_LCD

	LoadTable SD_Mode_Read_Fail
	call blit_print_table_string
	
	return

SD_MODE_SEND_READ_Sucess
	;Load the buffer
	call Load_framebuffer

	movlw 0x08
	movwf FSR2H
	clrf FSR2L

	movf POSTINC2,W
	call W_2HEX_to_LCD

	movf POSTINC2,W
	call W_2HEX_to_LCD

	movf POSTINC2,W
	call W_2HEX_to_LCD

	movf POSTINC2,W
	call W_2HEX_to_LCD

	movf POSTINC2,W
	call W_2HEX_to_LCD

	movf POSTINC2,W
	call W_2HEX_to_LCD

	call touch_wait_for_release	
	return

;//////////////////////////////////////
 cblock
tmpval
 endc
SD_MODE_SEND_WRITE
	;Load the buffer
	movlw 0x08
	movwf FSR2H
	clrf FSR2L

	movlw .0
SD_MODE_SEND_WRITE_LOOP
	movwf POSTINC2
	decfsz WREG
	bra SD_MODE_SEND_WRITE_LOOP

	clrf SD_addr+2
	clrf SD_addr+1
	clrf SD_addr

	call SD_write
	xorlw .0
	bz SD_MODE_SEND_WRITE_Sucess

	call Load_framebuffer
	call W_2HEX_to_LCD

	LoadTable SD_Mode_Write_Fail
	call blit_print_table_string
	
	return

SD_MODE_SEND_WRITE_Sucess
	call touch_wait_for_release	
	return
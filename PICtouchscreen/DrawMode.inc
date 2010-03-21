;Drawing mode for the touch screen

Drawing_Mode_clr_str db "clear",0x00
Drawing_Mode_print_str db "print",0x00
Drawing_Mode_save_str db "save",0x00
Drawing_Mode_load_str db "load",0x00

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
	Insert_Touch_value_N Print_Handler,.3

	movlw .5
	movwf Insert_Touch_var_y
	movlw .13
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N Main_Mode_Init,.3

	movlw .4
	movwf Insert_Touch_var_y
	movlw .13
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N Drawing_Mode_save,.3

	movlw .3
	movwf Insert_Touch_var_y
	movlw .13
	movwf Insert_Touch_var_x
	call Touch_Goto_Entry
	Insert_Touch_value_N Drawing_Mode_load,.3


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
	movlw .105
	call fb_goto_col
	LoadTable(Drawing_Mode_print_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .5
	call fb_goto_row
	movlw .105
	call fb_goto_col
	LoadTable(Mode_back_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .4
	call fb_goto_row
	movlw .105
	call fb_goto_col
	LoadTable(Drawing_Mode_save_str)
	call blit_box_print_table_string

	call Load_framebuffer
	movlw .3
	call fb_goto_row
	movlw .105
	call fb_goto_col
	LoadTable(Drawing_Mode_load_str)
	call blit_box_print_table_string
	return


;//////////////////////////////////////////////
;DAC transmit setting
 cblock
X_DAC,Y_DAC, TmpDACVar
 endc

DAC_OUTPUT
	;It's a command so bit9 is on, cmd 4 is recv DAC
	btfss PIR1,TXIF
	bra $-.2
	bsf TXSTA,TX9
	movlw b'10110100'
	movwf TXREG

	;nop ;Not needed as we must first prepare the data
	swapf Y_DAC,W
	andlw 0x0F
	movwf TmpDACVar
	movf X_DAC,W
	andlw 0xF0
	iorwf TmpDACVar,W
	;First nibbles are the high nibbles of X and Y
	btfss PIR1,TXIF
	bra $-.2
	movwf TXREG
	
	swapf X_DAC,W
	andlw 0xF0
	;Set the X and Y coords
	btfss PIR1,TXIF
	bra $-.2
	movwf TXREG

	swapf Y_DAC,W
	andlw 0xF0
	btfss PIR1,TXIF
	bra $-.2
	movwf TXREG
	return

;//////////////////////////////////////////////
;Draw Mode touch handlers

;//////////////////////////////////////////////
;Prints the current image on the oscilloscope for a few seconds
Print_Handler
	call Load_framebuffer

Print_Handler_Loop
	movf INDF0,W
	bz Print_Handler_SKIP

	;Calc the x position
	;128 pixels wide
	movlw b'01111111'
	andwf FSR0L,W
	movwf X_DAC

	rlncf X_DAC,F

	movlw 0x02
	subwf FSR0H,W
	rlcf WREG

	bcf WREG,0
	btfsc FSR0L,7
	bsf WREG,0

	;Now we have the y position in rows, multiply by 8 for the real val
	mullw .16
	movff PRODL,Y_DAC

	;Now check for active pixels
	btfsc INDF0,0
	call DAC_OUTPUT
	movlw .2
	addwf Y_DAC,F
	btfsc INDF0,1
	call DAC_OUTPUT
	movlw .2
	addwf Y_DAC,F
	btfsc INDF0,2
	call DAC_OUTPUT
	movlw .2
	addwf Y_DAC,F
	btfsc INDF0,3
	call DAC_OUTPUT
	movlw .2
	addwf Y_DAC,F
	btfsc INDF0,4
	call DAC_OUTPUT
	movlw .2
	addwf Y_DAC,F
	btfsc INDF0,5
	call DAC_OUTPUT
	movlw .2
	addwf Y_DAC,F
	btfsc INDF0,6
	call DAC_OUTPUT
	movlw .2
	addwf Y_DAC,F
	btfsc INDF0,7
	call DAC_OUTPUT
	
Print_Handler_SKIP
	infsnz FSR0L,F 
	incf FSR0H,F
	movlw 0x06
	cpfseq FSR0H
	bra Print_Handler_Loop

Print_Handler_Done
	return

;//////////////////////////////////////////////
;This handler draws a pixel and outputs it to the DAC
Drawing_Handler
	movff x_pos, X_DAC
	movff y_pos,Y_DAC
	call DAC_OUTPUT
	
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

DrawingFileName  db "Sketch  pbm",0x00
PBM_magic_string db "P4\n# PICTouch Writer v0.1\n128 64\n",0x00

 cblock
save_byte
 endc

Drawing_Mode_save
	;/Load the root directory
	call FAT_load_root
	
	;Delete the file if it is already there
	LoadTable DrawingFileName
	call FAT_delete_entry

	;Load the new file name into entryname
	LoadTable DrawingFileName
	call FAT_load_filename_TBLPTR

	;Normal files have no attributes
	clrf FAT_newfile_attrib

	;Set the number of clusters for the file, (going smaller than a cluster is wasteful and not currently possible)
	movlw .3
	movwf FAT_newfile_size 

	call FAT_new_entry
	TSTFSZ WREG
	retlw .1
	
	SD_load_buffer_FSR 2,0x000
	LoadTable(PBM_magic_string)
	call COPY_TABLE_STRING_TO_FSR2
	
;Now begin the writing of the pixel data
	call Load_framebuffer
	movlw b'00000001'
	movwf PRODH

	clrf save_byte
	bsf save_byte,0

Drawing_Mode_save_Loop
	;Check if the current bit is set
	movf POSTINC0,W	
	andwf PRODH,W

	;If it is then set the bit in the save byte
	bcf STATUS,C
	TSTFSZ WREG
	bsf STATUS,C

	rlcf save_byte,F

	btfss STATUS,C
	bra Drawing_Mode_save_Loop

	;Save the current byte
	movff save_byte,POSTINC2

	;Clear it ready for the next byte
	clrf save_byte
	bsf save_byte,0

	;Now check if the SD buffer is full
	SD_load_buffer_H + 2
	xorwf FSR2H,W
	bnz Drawing_Mode_save_check_row

	;It is full so write and get the next sector
	call SD_write
	xorlw .0
	bnz Drawing_Mode_save_fail

	;Fetch the next sector of the file
	call FAT_next_sector
	xorlw .0
	bnz Drawing_Mode_save_fail
	
	;Restart at the beginning of the buffer
	SD_load_buffer_FSR 2,0x000

Drawing_Mode_save_check_row
	;Now we check if we're at the end of a row
	movlw b'01111111'
	andwf FSR0L,W
	bnz Drawing_Mode_save_Loop

	;We're at the end of a row, so rotate the bits
	rlncf PRODH,F
	
	;Check if we're at the end of a 8bit row
	btfsc PRODH,0
	bra Drawing_Mode_new_8bit_row

	;Not at a new 8bit row, so rewind and repeat
	movlw .128
	subwf FSR0L,F
	movlw .0
	subwfb FSR0H,F
	bra Drawing_Mode_save_Loop

Drawing_Mode_new_8bit_row
	;Now check if we're at the end of the frame buffer
	movlw 0x06
	xorwf FSR0H,W
	bnz Drawing_Mode_save_Loop

	;The end! write the current data
	call SD_write
	xorlw .0
	bnz Drawing_Mode_save_fail

	call touch_wait_for_release
	retlw .0

Drawing_Mode_save_fail
	call touch_wait_for_release
	retlw .1

Drawing_Mode_load

	call touch_wait_for_release
	return
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
	call SD_MODE_FAT

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

;///////////////////////////////////////////////////////////////////////
;Filenames made by a PC are always uppercase (the long filename contains
;the mixed case version. New files created in this driver can be mixed case.
;
;The filenames must be 8+3 characters, long filenames are not supported.
;The last 3 characters are for file extensions

file1   db "TEST    PDF"
file2   db "NEWFILE TXT"
folder1 db "TESTDIR    "

;//////////////////////////////////////
SD_MODE_FAT
	call SD_Mode_redraw
	call Load_framebuffer
	call FAT_init
	TSTFSZ WREG
	bra SD_MODE_FAT_fail

	;////////////////////////////////////////////////
	;Delete file1="test.pdf" in the root directory

	;Get the root directory
	call FAT_load_root

	;Load the first file name into entryname
	LoadTable file1
	call FAT_load_filename_TBLPTR

	call FAT_delete_entry
	xorlw .0
	bnz SD_MODE_FAT_fail

	;/////////////////////////////////////////////////
	;Make a new file (file2="NEWFILE.TXT") which is 
	;two clusters long 
	;(a cluster is *usually* 8 sectors 8x512=4096 bytes

	;Load the root directory
	call FAT_load_root

	;Load the new file name into entryname
	LoadTable file2
	call FAT_load_filename_TBLPTR

	;Normal files have no attributes
	clrf FAT_newfile_attrib

	;Set the number of clusters for the file
	movlw .2
	movwf FAT_newfile_size 

	;Make the new file
	call FAT_new_entry
	xorlw .0
	bnz SD_MODE_FAT_fail

	;FAT_new_entry leaves us at the first sector of the new file in SD_addr

	;//NOTE: the SD buffer is not "safe" across FAT calls, the FAT calls use 
	;it, so allocate files then write/generate data for them

Fill_file_loop
	;Fill the SD buffer with some example data (M characters)
	SD_load_buffer_FSR 2,0x000

Fill_SD_buffer_loop
	movlw "M"
	movwf POSTINC2

	SD_load_buffer_H + 2
	xorwf FSR2H,W
	bnz	Fill_SD_buffer_loop

	;Buffer is full, write it to the file
	call SD_write
	xorlw .0
	bnz SD_MODE_FAT_fail

	;Fetch the next sector of the file
	call FAT_next_sector
	xorlw .0
	bz Fill_file_loop

	;We're here because FAT_next_sector failed (probably due to a EOF)
	;Check it is an EOF
	xorlw .1
	bnz SD_MODE_FAT_fail

	;/////////////////////////////////////////////////
	;Load a directory (TESTDIR) and make a blank new file in it of zero size

	;Load the root directory
	call FAT_load_root

	;Load the subdirectory name
	LoadTable folder1
	call FAT_load_filename_TBLPTR

	;Open the subdirectory
	call FAT_open_entry
	xorlw .0
	bnz SD_MODE_FAT_fail
	
	;Load the new file name
	LoadTable file2
	call FAT_load_filename_TBLPTR

	;Make the blank file
	;Normal files have no attributes
	clrf FAT_newfile_attrib

	;Set the number of clusters for the file
	movlw .0
	movwf FAT_newfile_size 

	;Make the new file
	call FAT_new_entry
	xorlw .0
	bnz SD_MODE_FAT_fail
	

SD_MODE_FAT_fail
	call touch_wait_for_release	
	return
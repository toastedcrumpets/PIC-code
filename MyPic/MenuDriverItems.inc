;*****************************************************************************************
; This is a sample navigation definition

;Button handling


Menu_Load_Func
	TBLRD+*
	movff TABLAT,Menu_Jump_locL
	TBLRD+*
	movff TABLAT,Menu_Jump_locH
	TBLRD+*
	movff TABLAT,Menu_Jump_locU

	;Print a forward arrow
	call LCD_line_2
	movlw b'01111110'
	call LCD_data_send

Menu_Load_Func_LOOP
	call Menu_button_process
	Menu_navigation_macro
	
	;If select is pressed, just return and menuing dies
	btfss buttonreleased,2
	bra Menu_Load_Func_LOOP
	
	movff Menu_Jump_locL,Current_menuL
	movff Menu_Jump_locH,Current_menuH
	movff Menu_Jump_locU,Current_menuU
	movlw .1
	movwf Current_item

	goto Menu_Open

;This macro is defined so the navigation can be altered easily
Menu_navigation_macro macro
	btfsc buttonreleased,1
	goto Menu_Prev_Item

	btfsc buttonreleased,0
	goto Menu_Next_Item
	endm

;Just some menu option types

;*****************************************************************************************
;Menu item entry points (Item types)

;////////////////////\\\\\\\\\\\\\\\\\\\
;The menu item is just a string
Menu_Text_Func 
	call Menu_button_process
	Menu_navigation_macro
	bra Menu_Text_Func

;////////////////////\\\\\\\\\\\\\\\\\\\
;Just break out of the menuing loop
Menu_Exit_Func
	;Print a back arrow
	call LCD_line_2
	movlw b'00001000'
	call LCD_data_send

Menu_Exit_Func_LOOP
	call Menu_button_process
	Menu_navigation_macro
	
	;If select is pressed, just return and menuing dies
	btfss buttonreleased,2
	bra Menu_Exit_Func_LOOP
	return

;////////////////////\\\\\\\\\\\\\\\\\\\
;Moves to another menu
;Save the possible menu jump address
 cblock
Menu_Jump_locL,Menu_Jump_locH,Menu_Jump_locU
 endc
Menu_Load_Func
	TBLRD+*
	movff TABLAT,Menu_Jump_locL
	TBLRD+*
	movff TABLAT,Menu_Jump_locH
	TBLRD+*
	movff TABLAT,Menu_Jump_locU

	;Print a forward arrow
	call LCD_line_2
	movlw b'01111110'
	call LCD_data_send

Menu_Load_Func_LOOP
	call Menu_button_process
	Menu_navigation_macro
	
	;If select is pressed, just return and menuing dies
	btfss buttonreleased,2
	bra Menu_Load_Func_LOOP
	
	movff Menu_Jump_locL,Current_menuL
	movff Menu_Jump_locH,Current_menuH
	movff Menu_Jump_locU,Current_menuU
	movlw .1
	movwf Current_item

	goto Menu_Open

;////////////////////\\\\\\\\\\\\\\\\\\\
;Gotos to the function stored
;Save the possible menu jump address
 cblock
Menu_Goto_locL,Menu_Goto_locH,Menu_Goto_locU
 endc
Menu_Goto_Func
	TBLRD+*
	movff TABLAT,Menu_Goto_locL
	TBLRD+*
	movff TABLAT,Menu_Goto_locH
	TBLRD+*
	movff TABLAT,Menu_Goto_locU

	;Print a forward arrow
	call LCD_line_2
	movlw b'01111110'
	call LCD_data_send

Menu_Goto_Func_LOOP
	call Menu_button_process
	Menu_navigation_macro
	
	;If select is pressed, just return and menuing dies
	btfss buttonreleased,2
	bra Menu_Goto_Func_LOOP
	
	movff Menu_Goto_locU,PCLATU
	movff Menu_Goto_locH,PCLATH
	movf Menu_Goto_locL,W
	movwf PCL
	;We've now jumped to the called function

;////////////////////\\\\\\\\\\\\\\\\\\\
;Control the value of a byte by scrolling
	cblock
Byte_Adjust_Mode
	endc

Menu_Byte_Adjust
	;Determine what to display
	call Menu_load_arg
	;Don't start in adjust mode
	clrf Byte_Adjust_Mode
Menu_Byte_Redisplay
	;Set the line for data output
	call LCD_line_2
	movf INDF1,W
	call W_to_Dec_LCD
	
	btfss Byte_Adjust_Mode,0
	bra Menu_Byte_Adjust_LOOP_space
	movlw '*'
	call LCD_data_send
	
Menu_Byte_Adjust_LOOP_space
	movlw ' '
	call LCD_data_send	
Menu_Byte_Adjust_LOOP
	call Menu_button_process

	;Check we're not in data entry mode
	btfsc Byte_Adjust_Mode,0
	bra Byte_Adjust_Entry_Mode

	Menu_navigation_macro

	;Check if the select button was pressed
	btfss buttonreleased,2
	bra Menu_Byte_Adjust_LOOP

	;Select was pressed
	comf Byte_Adjust_Mode
	bra Menu_Byte_Redisplay

Byte_Adjust_Entry_Mode
	; Respond to the common button presses, just do up and down
	btfss buttonreleased,0
	bra $+.6
	decf  INDF1
	bra Menu_Byte_Redisplay

	btfss buttonreleased,1
	bra $+.6
	incf  INDF1
	bra Menu_Byte_Redisplay

	;Check if the select button was pressed
	btfss buttonreleased,2
	bra Menu_Byte_Adjust_LOOP

	;Select was pressed
	comf Byte_Adjust_Mode
	bra Menu_Byte_Redisplay

;////////////////////\\\\\\\\\\\\\\\\\\\
;Toggle the first bit of a register
Yes_string db "Yes",0x00
No_string  db "No ",0x00

Menu_Bit_Toggle 
	;Determine what to display
	call Menu_load_arg

Menu_bit_redisplay
	;Set the line for data output
	call LCD_line_2
	LCDPrintString Yes_string
	btfsc INDF1,0
	bra Menu_Bit_Toggle_LOOP
	call LCD_line_2
	LCDPrintString No_string

Menu_Bit_Toggle_LOOP
	call Menu_button_process
	Menu_navigation_macro

	;Check if the select button was pressed
	btfss buttonreleased,2
	bra Menu_Bit_Toggle_LOOP

	;Select was pressed
	comf INDF1
	bra Menu_bit_redisplay

;////////////////////\\\\\\\\\\\\\\\\\\\
;Watch a byte, and update the screen if the byte changes
Menu_Byte_Watch
	;Determine what to monitor
	call Menu_load_arg
	
Menu_Byte_Watch_Redisplay
	;Set the line for data output
	call LCD_line_2
	;movf INDF1,W
	;call W_to_Dec_LCD
	;call LCD_line_3
	movff INDF1,BIN_L
	call Byte_to_Binary_LCD

Menu_Byte_Watch_LOOP
	call Menu_button_process
	Menu_navigation_macro

	bra Menu_Byte_Watch_Redisplay
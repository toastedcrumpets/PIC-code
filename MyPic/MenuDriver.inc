;*****************************************************************************************
;EA DOG LCD Menu Driver
; To create a menu, make the tables wherever you like. The format is
; AddMenuItem "A string", initFunction, tickfunction
; db "data",0x00,0x2323,
;
; For each item, the string is displayed, the tick function is stored in ram, then initFunction is jumped to.
; The data in the following line is available to initFunction through the TABPTR, and it is 
; set to point to the first byte of the data. When the initFunction returns, whatever called 
; Menu_Open returns.
;
; You end a menu by assiging it a label using
; Menu_Label endCurrentMenu
;
; So a complete menu is
; AddMenuItem "A menu item", initFunction, tickfunction
; db "Some data for the initFunction",0x00
; AddMenuItem "Another item",different/sameInitFunc, sameTickFunc
; db "Some more data for this init function, could setup the tick func"
;Main_Menu endCurrentMenu
; 
; To open the menu, initialise the menu and call menu_open, this will prep the display and return
;	Load_Menu Main_Menu
;	call Menu_Open
;
; Then you can scroll the menu using next_item and prev_item to navigate it.
;
; If you call Menu_Ticker, this will redirect the call to the stored tickfunction.
;*****************************************************************************************

; Menu variables
 cblock
;Navigation variables
Current_item, Max_items, Menu_Loop_Counter
;Current menu pointer
Current_menuL,Current_menuH,Current_menuU
;Current action pointer
Current_actionL,Current_actionH,Current_actionU
;Button vars
buttonstate, buttonnewstate, buttonchanged, buttonpressed, buttonreleased
 endc

 variable menucounter
menucounter set 0

 variable menuitemcounter
menuitemcounter set 0

;*****************************************************************************************
;Menu construction macros

;This macro will load the menu pointer, use to initialise the menu 
Load_Menu macro _menuLocation
	movlw LOW(_menuLocation)
	movwf Current_menuL
	movlw HIGH(_menuLocation)
	movwf Current_menuH
	movlw UPPER(_menuLocation)
	movwf Current_menuU

	movlw .1
	movwf Current_item
	endm

;These just turn 3/2 byte locations into storable data
#define UnpackPLocation(_function) LOW(_function),HIGH(_function),UPPER(_function)
#define UnpackDLocation(_data) LOW(_data),HIGH(_data)

; This defines a general menu item with init_function and a loop_function
AddMenuItem macro _string,_init_function,_loop_function
Menu#v(menucounter)_Item#v(menuitemcounter) db _string,0x00,UnpackPLocation(_loop_function),UnpackPLocation(_init_function)
menuitemcounter=menuitemcounter+1
	endm

; Once all items and levels are defined, this builds the menu tables
endCurrentMenu macro
	local itemloopcounter = 0
	;First store the number of items, and a spare byte for padding
	db .#v(menuitemcounter),.0
	while itemloopcounter < menuitemcounter
		;Store a pointer to the menu item, and a spare byte for packing
		db LOW(Menu#v(menucounter)_Item#v(itemloopcounter)),HIGH(Menu#v(menucounter)_Item#v(itemloopcounter)),UPPER(Menu#v(menucounter)_Item#v(itemloopcounter)),0x00
itemloopcounter = itemloopcounter+1
		endw
	data 0x00
menuitemcounter=0
menucounter=menucounter+1
	endm

;*****************************************************************************************
;Helper Functions

;Load the first RAM data argument into INDF1
Menu_load_arg
	TBLRD*+
	movff TABLAT,FSR1L
	TBLRD*+
	movff TABLAT,FSR1H
	return

;*****************************************************************************************
;Opening the menu
 cblock
L_menu_tmp,H_menu_tmp
 endc
Menu_Open
	movff Current_menuL,TBLPTRL
	movff Current_menuH,TBLPTRH
	movff Current_menuU,TBLPTRU

	call LCD_clear
	;Prep the loop counter
	movff Current_item,Menu_Loop_Counter

	;Get the number of items 
	TBLRD*+
	movff TABLAT,Max_items
	;Hop the padding byte 
	TBLRD*+

	dcfsnz Menu_Loop_Counter
	bra $+.12
	;MenuTable pointers are stored in 4 byte (2 word) blocks
	TBLRD*+
	TBLRD*+
	TBLRD*+
	TBLRD*+
	bra $-.12

	;Now load the items table ptr from the table ptr
	TBLRD*+
	movff TABLAT,L_menu_tmp
	TBLRD*+
	movff TABLAT,H_menu_tmp
	TBLRD*+
	movff TABLAT,TBLPTRU
	movff H_menu_tmp,TBLPTRH
	movff L_menu_tmp,TBLPTRL

	;Print the title text of the item
	call LCD_line_1
	call LCD_PrintTString

	;Store the menu items loop function
	TBLRD*+
	movff TABLAT,Current_actionL
	TBLRD*+
	movff TABLAT,Current_actionH
	TBLRD*+
	movff TABLAT,Current_actionU

	;Begin loading the computed goto for the initialisation function
	TBLRD*+	
	movff TABLAT,L_menu_tmp
	TBLRD*+
	movff TABLAT,PCLATH
	TBLRD*+
	movff TABLAT,PCLATU

	;Ready to perform the computed jump, just align the table pointer on the next machine word
	btfsc TBLPTRL,0
	TBLRD*+

	;Perform the computed goto
	movf  L_menu_tmp,W
	movwf PCL

;*****************************************************************************************
;Menu controls
Menu_Prev_Item
	dcfsnz Current_item,W
	movlw .1
	movwf Current_item
	goto Menu_Open

Menu_Next_Item
	movf  Current_item,W
	subwf Max_items,W
	btfss STATUS,Z
	incf Current_item
	goto Menu_Open

;Jump to the menu items loop
Menu_Ticker	
	movff Current_actionH,PCLATH
	movff Current_actionU,PCLATU
	movf  Current_actionL,W
	movwf PCL

Menu_load_from_table
	TBLRD*+
	movff TABLAT,Current_menuL
	TBLRD*+
	movff TABLAT,Current_menuH
	TBLRD*+
	movff TABLAT,Current_menuU
	movlw .1
	movwf Current_item
	goto Menu_Open
;*****************************************************************************************
; Three button menu
;
; You must define the three buttons for the menu before including this file
; #define Menu_btnUp PORTB,5
; #define Menu_btnDwn PORTB,0
; #define Menu_btnSel PORTA,4
;

;*****************************************************************************************
;General navigation handler
;
; This function is used like so
;  AddMenuItem "A Menu Item",Menu_Navigation_Init, Menu_Navigation
;  db UnpackPLocation(a_function),moredata
;
; In this menu item, the menu can navigated up (btnUp) and down (btnDwn), and when select is pressed,
; a_function is called and the table pointer will point to the first byte of moredata.

 cblock
Menu_Table_locL,Menu_Table_locH,Menu_Table_locU
 endc

;The init function stores the current table location for when the select button is pressed 
Menu_Navigation_Init
	movff TBLPTRL,Menu_Table_locL
	movff TBLPTRH,Menu_Table_locH
	movff TBLPTRU,Menu_Table_locU	
	return
	
;This function 
Menu_Navigation
	setf buttonnewstate
	;Check buttons
	btfss Menu_btnUp
	bcf buttonnewstate,0

	btfss Menu_btnDwn
	bcf buttonnewstate,1

	btfss Menu_btnSel
	bcf buttonnewstate,2

	;Determine if a button has changed	
	movf buttonstate,W
	xorwf buttonnewstate,W
	movwf buttonchanged

	; if the button has flipped, and the new state is high it's a release
	andwf buttonnewstate,W
	movwf buttonreleased

	; if the button has flipped, and its not released its a press
	xorwf buttonchanged,W
	movwf buttonpressed

	; Update the state vector
	movff buttonnewstate,buttonstate

	; Menu Navigation
	btfsc buttonreleased,1
	goto Menu_Prev_Item

	btfsc buttonreleased,0
	goto Menu_Next_Item

	btfss buttonreleased,2
	return
	;Select button pressed
	;First load the table location
	movff Menu_Table_locL,TBLPTRL
	movff Menu_Table_locH,TBLPTRH
	movff Menu_Table_locU,TBLPTRU
	
	;Then use the first 3 bytes to jump to the stored function
	TBLRD*+
	movf TABLAT,W
	TBLRD*+
	movff TABLAT,PCLATH
	TBLRD*+
	movff TABLAT,PCLATU
	movwf PCL

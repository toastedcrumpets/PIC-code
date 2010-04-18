;*****************************************************************************************
;EA DOG LCD SPI driver
;*****************************************************************************************
;
; To use the driver, you must define the PORT/LAT for the three pins
;	LCD_RS, LCD_SI, LCD_SCL
; and configure these ports for output.
; 
; You must also pick the number of lines and voltage of 
; the display by defining one of
;   LCD_2L6V, LCD_2L5V, LCD_2L3V3, LCD_3L5V
;
; Finally, you must call LCD_INIT, to initialise the display for output
 
 cblock
LCD_SENDBYTE, BIT_counter
R0, R1, R2
BIN_L, BIN_H
counter, temp
 endc

;****************************************************************
; Helper Macros
;Sets up the table ptr for a fixed address
LoadTable macro _table
	movlw LOW (_table) 
	movwf TBLPTRL	
	movlw HIGH (_table)	
	movwf TBLPTRH 
	movlw UPPER (_table)
	movwf TBLPTRU 
	endm

; Print on the LCD the (db "String to print",0x00) pointed to by _table
LCDPrintString macro _table
	LoadTable(_table)
	call LCD_PrintTString
	endm

;****************************************************************
;Debugging macros
;Prints the current value of the table ptr
LCD_PrintTABPTR macro
	movf TBLPTRL,W
	call W_2HEX_to_LCD
	movf TBLPTRH,W
	call W_2HEX_to_LCD
	movf TBLPTRU,W
	call W_2HEX_to_LCD
	endm

;Prints whatever is at the table ptr
LCD_PrintTableLine macro
	TBLRD*
	movf TABLAT,W
	call W_2HEX_to_LCD
	endm

;****************************************************************
;Initialisation tables for the LCD
 ifdef LCD_2L6V
 #define NUM_Lines	.2
LCD_INIT_DATA			;für 8 Bit Bus und serielle Kommunikation
	dw	b'00111001'		;Function set: 8 Bit BUS
	dw	b'00011100'		;Bias set BS 1/4, 2-Zeilig
	dw	b'01010001'		;Power control: Booster aus, Kontrast C5,C4 setzen
	dw	b'01101010'		;Follower control: Spannungsfolger und Verstärkung
	dw	b'01110100'		;Contrast set: Kontrast C3, C2, C1 setzen
	dw	b'00111000'		;Instruction Table 0
	dw	b'00001100'		;Display ein, Coursor aus
	dw	b'00000001'		;Display Löschen, return home
	dw	b'00000110'		;Cursor auto incrment
	dw	0x00			;ENDE Initialisierung
 endif

 ifdef LCD_2L5V
 #define NUM_Lines	.2
LCD_INIT_DATA          ;für 8 Bit Bus und serielle Kommunikation
	dw	b'00111001'		;Function set: 8 Bit BUS
	dw	b'00011100'		;Bias set BS 1/4, 2-Zeilig
	dw	b'01010010'		;Power control: Booster aus, Kontrast C5,C4 setzen
	dw	b'01101001'		;Follower control: Spannungsfolger und Verstärkung
	dw	b'01110111'		;Contrast set: Kontrast C3, C2, C1 setzen
	dw	b'00111000'		;Instruction Table 0
	dw	b'00001100'		;Display ein, Coursor aus
	dw	b'00000001'		;Display Löschen, return home
	dw	b'00000110'		;Cursor auto incrment
	dw	0x00			;ENDE Initialisierung
 endif

 ifdef LCD_2L3V3
 #define NUM_Lines	.2
LCD_INIT_DATA			;für 8 Bit Bus und serielle Kommunikation
	dw	b'00111001'		;Function set: 8 Bit BUS
	dw	b'00011100'		;Bias set BS 1/4, 2-Zeilig
	dw	b'01010110'		;Power control: Booster ein, Kontrast C5,C4 setzen
	dw	b'01101100'		;Follower control: Spannungsfolger und Verstärkung
	dw	b'01110111'		;Contrast set: Kontrast C3, C2, C1 setzen
	dw	b'00111000'		;Instruction Table 0
	dw	b'00001100'		;Display ein, Coursor aus
	dw	b'00000001'		;Display Löschen, return home
	dw	b'00000110'		;Cursor auto incrment
	dw	0x00			;ENDE Initialisierung
 endif

 ifdef LCD_3L5V
 #define NUM_Lines	.3
LCD_INIT_DATA			;für 8 Bit Bus und serielle Kommunikation
	dw	b'00111001'		;Function set: 8 Bit Daten, switch to Instr. Table 01
	dw	b'00011101'		;Bias set BS 1/4, 3-Zeilig
	dw	b'01010010'		;Power control: Booster aus, Kontrast C5,C4 setzen
	dw	b'01101001'		;Follower control: Spannungsfolger und Verstärkung
	dw	b'01110111'		;Contrast set: Kontrast C3, C2, C1 setzen
	dw	b'00111000'		;Instruction Table 0
	dw	b'00001100'		;Display ein, Coursor ein, cursor flash ein
	dw	b'00000001'		;Display Löschen, return home
	dw	b'00000110'		;Cursor auto incrment
	dw	0x00			;ENDE Initialisierung
 endif

;Routines*****************************************************************
;Helper macro for commands
LCD_cmd macro _cmd
	movlw   _cmd
	call	LCD_control_send
	return
	endm

;Clear the screen
LCD_clear LCD_cmd(b'00000001')
;Move to line 1
LCD_line_1 LCD_cmd(b'10000000');h'80'+h'00'

;Two line screen definition for line 2
 if (NUM_Lines == .2)
LCD_line_2 LCD_cmd(b'11000000');h'80'+h'40'
 endif
 
;Three line screen definition for line 2 and 3
 if (NUM_Lines == .3)
LCD_line_2 LCD_cmd(b'10010000');h'80'+h'10'
LCD_line_3 LCD_cmd(b'10100000');h'80'+h'20'
 endif 

;*******************************************************************
;Initialisation of LCD

LCD_INIT
	LoadTable(LCD_INIT_DATA)	;Adresse der
	
LCD_INIT_loop					;Initialisierungschleife
	call	delay_2ms			;
	TBLRD*+					;Hole neue Daten zum Initialisieren
	movf	TABLAT,W			;
	bz	LCD_init_end			;Falls Datum == 0: Ende des Initialisierungsfiles
	TBLRD*+					;Dummy read: zweites Byte im Speicher wird nicht gebraucht
	
	call	LCD_control_send		;sende Daten an das LCD
	bra	LCD_INIT_loop			;Lies nächstes Byte ein


LCD_init_end					;Ende der Initialisierung
	clrf	TBLPTRL				;Pointer auf das Initialisierungsfile löschen
	clrf	TBLPTRH				;
	clrf	TBLPTRU				;
	return					;

;*******************************************************************
;Prints the string pointed to by the table pointer
LCD_PrintTString
	TBLRD*+	
	movf	TABLAT,W
	btfsc	STATUS,Z
	return

	rcall	LCD_data_send
	bra	LCD_PrintTString

LCD_PrintIDR1String 
	movf	POSTINC0,W
	btfsc	STATUS,Z
	return
	rcall	LCD_data_send
	bra	LCD_PrintIDR1String

;*******************************************************************
;1 Bit serial communication
; Byte to be sent is in W 
LCD_data_send
	bsf	LCD_RS		;Sende Daten
	bra	$+.4		;
				;
LCD_control_send		;
	bcf	LCD_RS		;Sende Kommando
	movwf	LCD_SENDBYTE	;W sichern
 				;
	movlw	.8		;sende 8 Bits
	movwf	BIT_counter	;

LCD_loop			;
	btfsc	LCD_SENDBYTE,7	;Kopiere Bit 7 auf den BUS
	bsf	LCD_SI		;
	btfss	LCD_SENDBYTE,7	;
	bcf	LCD_SI		;
				;
	bsf	LCD_SCL		;Clock toggeln: LCD übernimmt Bit
	nop			;
	bcf	LCD_SCL		;
	
	rlncf	LCD_SENDBYTE,f	;zu sendendes Byte nach links schieben
	decfsz	BIT_counter,f	;Alle Bits gesendet?
	bra	LCD_loop

LCD_end
	btfsc	LCD_RS		;Warte: falls Daten: 30µs
	call	delay_30us
	
	btfss	LCD_RS		;falls Kommando: 2ms
	call	delay_2ms
 	
	bcf	LCD_RS		;Ausgänge zurücksetzen
	bcf	LCD_SI		;
	bcf	LCD_SCL		;

	return

;************************************************************************
;Translate W to a 2 word hex code, and write that hex code to the screen
W_2HEX_to_LCD
	movwf	w_temp
	swapf	w_temp,W		;First Number
	call	W_1HEX_to_LCD
	movf	w_temp,W		;second_number
	call	W_1HEX_to_LCD
	return

;************************************************************************
;Translate the lower nibble of W as a hex code and write it to the screen
W_1HEX_to_LCD				;Interpretiert das untere Nibble als eine HEX stelle->LCD 
	andlw	0x0f
	sublw	0x09
	btfss	STATUS,C		;carry gelöscht, wen Buchstabe A-F
	addlw	0xF9			;=-7
	sublw	0x39			;=0x30+d'9'
	call	LCD_data_send
	return

;************************************************************************
;************************************************************************
; Convert 2 byte value in FSR0 to a decimal value on the screen
FSR16_to_LCD
	movf	POSTINC0,W
	movwf	BIN_L
	movf	POSTINC0,W
	movwf	BIN_H
	call	BIN_to_BCD
	
	movf	R0,W
	call	W_1HEX_to_LCD
	movf	R1,W
	call	W_2HEX_to_LCD
	movf	R2,W
	call	W_2HEX_to_LCD
	return

;************************************************************************
;************************************************************************
; Convert W to a decimal value on the screen
W_to_Dec_LCD
	clrf    BIN_H
	movwf   BIN_L

	call	BIN_to_BCD
	movf	R1,W
	call	W_1HEX_to_LCD
	movf	R2,W
	call	W_2HEX_to_LCD
	return

; Prints the bits of the register BIN_L on the screen
 cblock
B2BLCD_mask
 endc
Byte_to_Binary_LCD


Byte_to_Binary_LCD_LOOP
	movf BIN_L,W
	andwf B2BLCD_mask,W
	btfss STATUS,Z
	movlw .1
	;Make 1 and 0, a character 1 and 0
	iorlw b'00110000'
	call LCD_data_send

	rrcf B2BLCD_mask,f
	btfss STATUS,C
	bra Byte_to_Binary_LCD_LOOP
	return

; This function converts the 2-byte digit stored in BIN_H and BIN_L to 5 digit BCD stored in R0(low nibble only), R1, R2  
BIN_to_BCD
	bcf     STATUS,C                ; clear the carry bit
	movlw   d'16'
	movwf   counter
	clrf    R0
	clrf    R1
	clrf    R2
loop16  rlcf    BIN_L,f
	rlcf    BIN_H,f
	rlcf    R2,f
	rlcf    R1,f
	rlcf    R0,f

	decfsz  counter,f
	goto    adjDEC
	goto	BIN_2_BCD_end

adjDEC  lfsr	FSR2, R2
	call    adjBCD

	lfsr	FSR2, R1
	call    adjBCD

	lfsr	FSR2, R0
	call    adjBCD

	goto    loop16

adjBCD  movlw   3
	addwf   INDF2,W
	movwf   temp
	btfsc   temp,3          ; test if result > 7
	movwf   INDF2
	movlw   30
	addwf   INDF2,W
	movwf   temp
	btfsc   temp,7		; test if result > 7
	movwf   INDF2		; save as MSD
	retlw	d'0'


BIN_2_BCD_end
	return
;************************************************************************
;************************************************************************
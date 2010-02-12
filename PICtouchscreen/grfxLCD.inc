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
counter, LCD_temp
 endc

;****************************************************************
; Helper Macros


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
 #define LCD_delay

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
	dw	b'11111111'		;ENDE Initialisierung

;Clear the screen
LCD_clear LCD_cmd(b'00000001')
 endif

 ifdef LCD_2L5V
 #define NUM_Lines	.2
 #define LCD_delay

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
	dw	b'11111111'		;ENDE Initialisierung

;Clear the screen
LCD_clear LCD_cmd(b'00000001')
 endif

 ifdef LCD_2L3V3
 #define NUM_Lines	.2
 #define LCD_delay

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
	dw	b'11111111'		;ENDE Initialisierung

;Clear the screen
LCD_clear LCD_cmd(b'00000001')
 endif

 ifdef LCD_3L5V
 #define NUM_Lines	.3
 #define LCD_delay

LCD_INIT_DATA			;für 8 Bit Bus und serielle Kommunikation
	dw	b'00111001'		;Function set: 8 Bit Daten, switch to Instr. Table 01
	dw	b'00011101'		;Bias set BS 1/4, 3-Zeilig
	dw	b'01010010'		;Power control: Booster aus, Kontrast C5,C4 setzen
	dw	b'01101001'		;Follower control: Spannungsfolger und Verstärkung
	dw	b'01110001'		;Contrast set: Kontrast C3, C2, C1 setzen
	dw	b'00111000'		;Instruction Table 0
	dw	b'00001100'		;Display ein, Coursor ein, cursor flash ein
	dw	b'00000001'		;Display Löschen, return home
	dw	b'00000110'		;Cursor auto incrment
	dw	b'11111111'		;ENDE Initialisierung

;Clear the screen
LCD_clear LCD_cmd(b'00000001')
 endif

 ifdef LCD_grfx
 #define NUM_Lines	.0
 ;Don't need delays for this screen
 ;#define LCD_delay

LCD_INIT_DATA			;
	dw	b'01000000'		;Display start line 0
	dw	b'10100000'		;ADC forward
	dw	b'11000000'		;Normal COM0-COM63
	dw	b'10100110'		;Display Normal
	dw	b'10100010'		;Set bias 1/9 (Duty 1/65)
	dw	b'00101111'		;Booster, Regulator and Follower on
	dw	b'11111000'		;Internal booster to 4x
	dw	b'00000000'		;....
	dw	b'00100111'		;Contrast set
	dw	b'10000001'		;....
	dw	b'00010110'		;....
	dw	b'10101100'		;Indicator off
	dw	b'00000000'		;....
	dw	b'10101111'		;Display on
	dw  b'10100100'     ;All points off
	dw  b'11111111'     ;End init

 cblock
col_num
 endc

LCD_goto_column
	addlw .4
    movwf col_num
	swapf col_num,W

	andlw b'00001111'
	iorlw b'00010000'
	call LCD_control_send

	movf col_num,W
	andlw b'00001111'
	call LCD_control_send
	return

LCD_goto_row
	andlw b'00000111'
	iorlw b'10110000'
	call LCD_control_send
	return

 cblock
row_ctr
 endc
LCD_clear
	;Setup the counter	
	movlw .8
	movwf row_ctr
	
LCD_clear_loop
	decf row_ctr
	;load the row
	movf row_ctr,w

	call LCD_goto_row
	call LCD_clear_line

	tstfsz row_ctr
	bra LCD_clear_loop

	return

 cblock
column_ctr
 endc

LCD_clear_line
	;Set column to the beginning of the line
	movlw .0
	call LCD_goto_column

	;Setup the counter	
	movlw .128
	movwf column_ctr
	;Start wiping the row
LCD_clear_line_loop
	movlw b'00000000'
	call LCD_data_send
	decfsz column_ctr
	bra LCD_clear_line_loop
	return
 endif

;Routines*****************************************************************
;Helper macro for commands
LCD_cmd macro _cmd
	movlw   _cmd
	call	LCD_control_send
	return
	endm

;Move to line 1
 if (NUM_Lines > 0)
LCD_line_1 LCD_cmd(b'10000000');h'80'+h'00'
 endif

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

 cblock
LCD_tmp_init_cmd
 endc

LCD_INIT
	LoadTable(LCD_INIT_DATA)	;Adresse der
	movlw .0

LCD_INIT_loop					;Initialisierungschleife
 ifdef LCD_delay
	call	delay_2ms			;
 endif

	TBLRD*+					;Hole neue Daten zum Initialisieren
	movff	TABLAT, LCD_tmp_init_cmd

	movf LCD_tmp_init_cmd,W
	xorlw b'11111111'
	bz	LCD_init_end
			;Falls Datum == 0: Ende des Initialisierungsfiles

	movf LCD_tmp_init_cmd,W
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
LCD_control_send
	bcf	LCD_RS		;Sende Daten
	bra	$+.4		;

LCD_data_send
	bsf	LCD_RS		;Sende Kommando
	movwf	LCD_SENDBYTE	;W sichern
 				;
	movlw	.8		;sende 8 Bits
	movwf	BIT_counter	;

LCD_loop
 ifdef LCD_delay			;
	btfsc	LCD_SENDBYTE,7	;Kopiere Bit 7 auf den BUS
	bsf	LCD_SI		;
	btfss	LCD_SENDBYTE,7	;
	bcf	LCD_SI		;
 else
	bsf	LCD_SI		;
	btfss	LCD_SENDBYTE,7	;
	bcf	LCD_SI		;
 endif

				;
	bsf	LCD_SCL		;Clock toggeln: LCD übernimmt Bit
 ifdef LCD_delay
	nop			;
 endif
	bcf	LCD_SCL		;
	
	rlncf	LCD_SENDBYTE,f	;zu sendendes Byte nach links schieben
	decfsz	BIT_counter,f	;Alle Bits gesendet?
	bra	LCD_loop

LCD_end
 ifdef LCD_delay
	btfsc	LCD_RS		;Warte: falls Daten: 30µs
	call	delay_30us
	
	btfss	LCD_RS		;falls Kommando: 2ms
	call	delay_2ms
 endif
	bcf	LCD_RS		;Ausgänge zurücksetzen
	bcf	LCD_SI		;
;	bcf	LCD_SCL		;

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
	movlw b'10000000'
	movwf B2BLCD_mask

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
	movwf   LCD_temp
	btfsc   LCD_temp,3          ; test if result > 7
	movwf   INDF2
	movlw   30
	addwf   INDF2,W
	movwf   LCD_temp
	btfsc   LCD_temp,7		; test if result > 7
	movwf   INDF2		; save as MSD
	retlw	d'0'


BIN_2_BCD_end
	return
;************************************************************************
;************************************************************************
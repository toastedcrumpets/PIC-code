;UART debug header
cblock
UART_tmp,UART_W_tmp,UART_STATUS_tmp
UART_TBLPTRL_tmp,UART_TBLPTRH_tmp,UART_TBLPTRU_tmp
endc

UART_save_state
	movff WREG,UART_W_tmp
	movff STATUS,UART_STATUS_tmp
	movff TBLPTRL,UART_TBLPTRL_tmp
	movff TBLPTRH,UART_TBLPTRH_tmp
	movff TBLPTRU,UART_TBLPTRU_tmp
	return

UART_restore_state
	movff UART_W_tmp,WREG
	movff UART_STATUS_tmp,STATUS
	movff UART_TBLPTRL_tmp,TBLPTRL
	movff UART_TBLPTRH_tmp,TBLPTRH
	movff UART_TBLPTRU_tmp,TBLPTRU
	return

UART_transmit_char
	btfss PIR1,TXIF
	bra $-.2
	movwf TXREG
	return

UART_transmit_esc_char
	movlw 0x1B ;Esc Char
	rcall UART_transmit_char
	movlw 0x5B ;[ Char
	rcall UART_transmit_char
	return

UART_clear_screen 
	rcall UART_transmit_esc_char
	movlw "2" ;Clear screen code
	rcall UART_transmit_char
	movlw "J" ;.................
	rcall UART_transmit_char
	return

UART_cursor_home
	rcall UART_transmit_esc_char
	movlw "H" ;Clear screen code
	rcall UART_transmit_char
	return

UART_print_string_helper
	TBLRD*+	
	movf	TABLAT,W
	btfsc	STATUS,Z
	return

	rcall	UART_transmit_char
	bra	UART_print_string_helper


UART_print_string macro _table
	call UART_save_state
	movlw LOW (_table) 
	movwf TBLPTRL	
	movlw HIGH (_table)	
	movwf TBLPTRH 
	movlw UPPER (_table)
	movwf TBLPTRU
	call UART_print_string_helper
	call UART_restore_state
	endm

UART_print_reg_HEX macro reg
	call UART_save_state
	movff reg,UART_tmp
	movlw '0'
	rcall UART_transmit_char
	movlw 'x'
	rcall UART_transmit_char
	
	swapf	UART_tmp,W		;First Number
	call	UART_print_nibble_HEX
	movf	UART_tmp,W		;second_number
	call	UART_print_nibble_HEX

	call UART_restore_state	
	endm

UART_print_nibble_HEX			
	andlw	0x0f
	sublw	0x09
	btfss	STATUS,C
	addlw	0xF9	
	sublw	0x39		
	rcall   UART_transmit_char
	return

UART_print_reg_BIN macro reg
	call UART_save_state

	movff reg,UART_tmp
	call UART_print_reg_BIN_helper

	call UART_restore_state
	endm

UART_print_reg_BIN_helper
	movlw 'b'
	rcall UART_transmit_char
	movlw '\''
	rcall UART_transmit_char

	bsf STATUS,C
	rlcf UART_tmp,F
UART_print_reg_BIN_loop
	movlw b'00110000'
	btfsc STATUS,C
	movlw b'00110001'
	rcall UART_transmit_char
	bcf STATUS,C
	rlcf UART_tmp,F
	bnz UART_print_reg_BIN_loop	

	movlw '\''
	rcall UART_transmit_char
	return
	
UART_init
	;Setup the baud rate 115.2K4
	bcf TXSTA,SYNC
	bcf TXSTA,BRGH
	bsf BAUDCON,BRG16
	bcf TXSTA,TX9
	
	bsf TRISB,5
	bsf TRISB,7

	;115.2k
	clrf SPBRGH
	movlw .24
	movwf SPBRG

	;9.6k
	;movlw HIGH(.311)
	;movwf SPBRGH
	;movlw LOW(.311)
	;movwf SPBRG

	;Enable the UART
	bsf RCSTA,SPEN

	;Turn on transmission
	bsf TXSTA,TXEN

	rcall UART_clear_screen
	rcall UART_cursor_home

	return

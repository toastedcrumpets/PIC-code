;**************************************************
;NMEA GP, serial parser

;**************************************************
;The current sentence
 cblock
NMEA_sentence:80
Sentence_Bytes_Rec
NMEA_status
NMEA_status_2
 endc

;The system is ready to recieve NMEA data
#define NMEA_RecvEnable NMEA_status,0
;The system has recieved a NMEA sentence and is waiting for the checksum
#define NMEA_checksum_mode  NMEA_status,1
;The system has recieved the first checksum hex value
#define NMEA_recvdUpperChecksum  NMEA_status,2
;The system has recieved a valid NMEA sentence
#define NMEA_valid NMEA_status,3
;The system has obtained a waypoint
#define NMEA_waypt_obtained NMEA_status,4
;The system has the bearing to the waypoint
#define NMEA_waypt_bearing_valid NMEA_status,5
;The system has the heading to steer for the waypoint
#define NMEA_waypt_heading_valid NMEA_status,6

#define NMEA_new_waypt_data_flag NMEA_status,7

#define NMEA_time_valid NMEA_status_2,0
#define NMEA_fix_valid NMEA_status_2,1
#define NMEA_new_fix_data_flag NMEA_status_2,2

;**************************************************
;Initialises the UART and the NMEA parser
NMEA_init
	;Establish the UART
	bsf ADCON1,5
	bsf ADCON1,6
	bsf TRISB,4
	bsf TRISB,1
	
	;Remove all settings
	clrf NMEA_status
	clrf NMEA_status_2
	call NMEA_reset_recv
	clrf NMEA_Waypoint_Name
	clrf NMEA_Waypoint_Name+.8
	;Switch on interrupts for USART
	bsf PIE1,RCIE
	bsf IPR1,RCIP ;High priority

	bcf TXSTA,TXEN
	bcf TXSTA,SYNC
	bsf RCSTA,SPEN

	;Fixed Baud
	bsf TXSTA,BRGH
	bsf BAUDCON,BRG16

 #define BRGVAL .416

	movlw HIGH(BRGVAL)
	movwf SPBRGH
	movlw LOW(BRGVAL)
	movwf SPBRG

	bsf  RCSTA,CREN
	return

;**************************************************
;Enables the parser for reception again
NMEA_reset_recv
	clrf Sentence_Bytes_Rec
	;Clear the lower 4 bytes
	movlw b'11110000'
	andwf NMEA_status,f
	clrf NMEA_sentence_checksum
	;Perform last to enable the interrupt sequence
	bsf NMEA_RecvEnable
	return

NMEA_load_sentence
	movlw LOW(NMEA_sentence)
	movwf FSR1L
	movlw HIGH(NMEA_sentence)
	movwf FSR1H
	return

;**************************************************
;The function called on interrup for UART data recieved
 cblock
FSR1H_state,FSR1L_state,NMEA_byte_rec
NMEA_sentence_checksum
NMEA_recv_checksum_H
NMEA_recv_checksum_L
 endc
NMEA_data_rec
	;Read the data
	movff RCREG,NMEA_byte_rec

	;Return if not ready
	btfss NMEA_RecvEnable
	return

	;Check if its the start of a sentence
	movf NMEA_byte_rec,W
	sublw '$'
	bz NMEA_reset_recv

	;Or the newline char
	movf NMEA_byte_rec,W
	sublw 0x0A
	bz NMEA_reset_recv

	;or if we've run out of characters (E.g., missed the star)
	movf Sentence_Bytes_Rec,W
	sublw .80
	bz NMEA_reset_recv

	;Test if we're getting data or checksum
	btfsc NMEA_checksum_mode
	bra NMEA_data_rec_checksum

	;check if we're going to start getting the checksum
	movf NMEA_byte_rec,W
	sublw '*'
	bnz NMEA_data_rec_data

	;Switch to recieving the checksum
	bsf NMEA_checksum_mode
	return

NMEA_data_rec_data
	;Save the indirect register state
	movff FSR1H,FSR1H_state
	movff FSR1L,FSR1L_state

	;Load the sentence point
	call NMEA_load_sentence
	
	movf Sentence_Bytes_Rec,W
	movff NMEA_byte_rec,PLUSW1
	
	movf NMEA_byte_rec,W
	xorwf NMEA_sentence_checksum,f

	incf Sentence_Bytes_Rec
	;Restore the indirect register state
	movff FSR1H_state,FSR1H
	movff FSR1L_state,FSR1L		
	return

NMEA_data_rec_checksum
	;Check if we're getting the upper byte
	btfsc NMEA_recvdUpperChecksum
	bra NMEA_data_rec_checksum_low_recv

	;We're getting the upper byte
	bsf NMEA_recvdUpperChecksum
	movff NMEA_byte_rec,NMEA_recv_checksum_H
	return

NMEA_data_rec_checksum_low_recv
	;We've already recieved the upper byte, get the lower
	movff NMEA_byte_rec,NMEA_recv_checksum_L
	
	;Now verify the checksum
	swapf NMEA_sentence_checksum,W  
	andlw 0x0f
	sublw 0x09
	btfss STATUS,C
	addlw 0xF9
	sublw 0x39
	;Now check if the first checkbyte is right
	subwf NMEA_recv_checksum_H
	bnz	NMEA_reset_recv

	;Load the lower nibble
	movf NMEA_sentence_checksum,W  
	andlw 0x0f
	sublw 0x09
	btfss STATUS,C
	addlw 0xF9
	sublw 0x39
	;Now check if the second checkbyte is right
	subwf NMEA_recv_checksum_L
	bnz	NMEA_reset_recv
	
	;We made it! a genuine NMEA sentence recvd
	bcf NMEA_RecvEnable
	bsf NMEA_valid
	call NMEA_parse_APB
	call NMEA_parse_RMC
	call NMEA_reset_recv
	return

;******************************************************
;Header parser for a valid NMEA sentence
;Requires the expected header in the table pointer
NMEA_parse_header_check
	;Load the sentence in INDF1
	call NMEA_load_sentence
	;Check characters
NMEA_parse_sentence_hdr_loop
	TBLRD*+
	movf TABLAT,W
	;If the tablepointer is null we've reached the end of the header check
	bz NMEA_parse_sentence_exit
	subwf POSTINC1,W
	bnz NMEA_parse_sentence_exit
	bra NMEA_parse_sentence_hdr_loop
	;If this fails, it's because W is non zero!
NMEA_parse_sentence_exit
	return

;******************************************************
;Skips to past the next comma in the NMEA string
 cblock
NMEA_comma_skip_var
 endc
NMEA_comma_skip
	movf POSTINC1,W
	sublw ','
	btfsc STATUS,Z
	return
	bra NMEA_comma_skip

;******************************************************
;Parses the single decimal value in the current NMEA sentence field
 cblock
NMEA_parse_var
 endc

NMEA_parse_decfield
	;Check the field is not empty
	movff POSTINC1,NMEA_parse_var
	movf NMEA_parse_var,W
	sublw ','
	btfsc STATUS,Z
	;field is empty, so return null
	retlw .0
	
	;field is not empty, begin parsing
	;Clear the last three fields, the first two are guarranteed to be set
	clrf D2
	clrf D3
	clrf D4
	;Skip over the field
	call NMEA_comma_skip

	;Step back over the comma, and onto the next data elem
	movf POSTDEC1,W
	movf POSTDEC1,W


	;Load the first value
	movf POSTDEC1,W
	addlw -.48
	movwf D0

	;Load the punct and check its a punct
	movf POSTDEC1,W
	sublw '.'
	btfss STATUS,Z
	retlw .0

	;Load the second val, abort if its a comma
	movff POSTDEC1,D1
	movf D1,W
	sublw ','
	btfsc STATUS,Z
	retlw .0

	movf  D1,W
	addlw -.48
	movwf D1

	;Load the third val, goto parse if it is a comma
	movff POSTDEC1,NMEA_parse_var
	movf NMEA_parse_var,W
	sublw ','
	bz NMEA_parse_decfield_BCD_to_16bit
	movf NMEA_parse_var,W
	addlw -.48
	movwf D2

	;Try loading the fourth val, goto parse if its a comma
	movff POSTDEC1,NMEA_parse_var
	movf NMEA_parse_var,W
	sublw ','
	bz NMEA_parse_decfield_BCD_to_16bit
	movf NMEA_parse_var,W
	addlw -.48
	movwf D3

NMEA_parse_decfield_BCD_to_16bit
	call dec2bin16
	call NMEA_comma_skip
	call NMEA_comma_skip
	retlw .1

;******************************************************
;APB waypoint parser check
;
 cblock
NMEA_Waypoint_Name:9
NMEA_Bearing_H,NMEA_Bearing_L
NMEA_Heading_H,NMEA_Heading_L
NMEA_APB_tmp_var
 endc

;The header to look for
NMEA_APB_HDR db "GPAPB",0x00

NMEA_parse_APB
	;Load the example header in the table pointer	
	LoadTable NMEA_APB_HDR
	call NMEA_parse_header_check

	;Check the header matched
	btfss STATUS,Z
	return

	;Header matched, navigate to the waypoint name
	movlw .10
    movwf NMEA_APB_tmp_var

	;Set that new data is available
	bsf NMEA_new_waypt_data_flag
	;Loop over comma seperated fields
NMEA_parse_APB_LOOP_1
	call NMEA_comma_skip
	decfsz NMEA_APB_tmp_var
	bra NMEA_parse_APB_LOOP_1

	;Load the waypoint name in the second indirect register
	movlw LOW(NMEA_Waypoint_Name)
	movwf FSR2L
	movlw HIGH(NMEA_Waypoint_Name)
	movwf FSR2H

	;Now copy the waypoint name to the INDR2 until you hit a comma
NMEA_parse_APB_LOOP_2
	movf POSTINC1,W
	movwf NMEA_APB_tmp_var
	sublw ','
	bz NMEA_parse_APB_LOOP_2_end
	movff NMEA_APB_tmp_var,POSTINC2
	bra NMEA_parse_APB_LOOP_2

	;Add a null to terminate the waypoint name
NMEA_parse_APB_LOOP_2_end
	movlw 0x00
	movwf POSTINC2
	;Now we have the waypoint name
	bsf NMEA_waypt_obtained

	;Now check the bearing exists, we're already passed the seperating comma
	bcf NMEA_waypt_bearing_valid
	call NMEA_parse_decfield
	sublw .0
	btfss STATUS,Z
	bsf NMEA_waypt_bearing_valid

	movff NUMLO,NMEA_Bearing_L
	movff NUMHI,NMEA_Bearing_H

	call NMEA_comma_skip
	;Now check if there is a heading field
NMEA_parse_APB_heading_check
	bcf NMEA_waypt_heading_valid

	;Returns 0 in w if the parse failed
	call NMEA_parse_decfield
	sublw .0
	btfss STATUS,Z
	bsf NMEA_waypt_heading_valid

	movff NUMLO,NMEA_Heading_L
	movff NUMHI,NMEA_Heading_H
	
NMEA_parse_APB_return
	return


;******************************************************
;RMC waypoint parser check
;
 cblock
NMEA_clock:12
NMEA_Latitude:13
NMEA_Longitude:13
NMEA_RMC_tmp_var
 endc

;The header to look for
NMEA_RMC_HDR db "GPRMC",0x00

NMEA_parse_RMC
	;Load the example header in the table pointer	
	LoadTable NMEA_RMC_HDR
	call NMEA_parse_header_check

	;Check the header matched
	btfss STATUS,Z
	return

	call NMEA_comma_skip

	;Load the NMEA time into the INDR2
	movlw LOW(NMEA_clock)
	movwf FSR2L
	movlw HIGH(NMEA_clock)
	movwf FSR2H

	;Now copy the time to the INDR2 until you hit a comma
NMEA_parse_RMC_LOOP_1
	movf POSTINC1,W
	movwf NMEA_RMC_tmp_var
	sublw ','
	bz NMEA_parse_RMC_LOOP_1_end
	movff NMEA_RMC_tmp_var,POSTINC2
	bra NMEA_parse_RMC_LOOP_1

	;Add a null to terminate the time string
NMEA_parse_RMC_LOOP_1_end
	;Now add the N/S character
	movlw 0x00
	movwf POSTINC2
	;Now we have the time

	;Now check the fix is valid
	movf POSTINC1,W 
	sublw 'A'
	btfss STATUS,Z
	return

	;Skip the following comma
	movf POSTINC1,W
	
	;The fix is valid, start gathering more data
	bsf NMEA_time_valid	

	;Begin with the Latitude
	movlw LOW(NMEA_Latitude)
	movwf FSR2L
	movlw HIGH(NMEA_Latitude)
	movwf FSR2H
NMEA_parse_RMC_LOOP_2
	movf POSTINC1,W
	movwf NMEA_RMC_tmp_var
	sublw ','
	bz NMEA_parse_RMC_LOOP_2_end
	movff NMEA_RMC_tmp_var,POSTINC2
	bra NMEA_parse_RMC_LOOP_2

	;Add a null to terminate the lattitude string
NMEA_parse_RMC_LOOP_2_end
	;Now add the E/W character
	movff POSTINC1,POSTINC2
	movf POSTINC1,W 
	movlw 0x00
	movwf POSTINC2

	;Now longitude
	movlw LOW(NMEA_Longitude)
	movwf FSR2L
	movlw HIGH(NMEA_Longitude)
	movwf FSR2H
NMEA_parse_RMC_LOOP_3
	movf POSTINC1,W
	movwf NMEA_RMC_tmp_var
	sublw ','
	bz NMEA_parse_RMC_LOOP_3_end
	movff NMEA_RMC_tmp_var,POSTINC2
	bra NMEA_parse_RMC_LOOP_3

	;Add a null to terminate the longitude string
NMEA_parse_RMC_LOOP_3_end
	movff POSTINC1,POSTINC2
	movf POSTINC1,W 
	movlw 0x00
	movwf POSTINC2

	bsf NMEA_fix_valid

	bsf NMEA_new_fix_data_flag
	return

	return
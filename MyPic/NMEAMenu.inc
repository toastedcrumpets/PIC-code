;**************************************************
;NMEA 3 button menu system

;**************************************************

;*****************************************************************************************
;Prints the navigation output

Waypt_Headerstring db "Navigate Mode",0x00
Waypoint_String db "Waypt:",0x00
Br_String db "Br:",0x00
Hd_String db " Hd:",0x00
NoData_String db "-",0x00

NMEA_Menu_Waypoint_init
	bsf NMEA_new_waypt_data_flag
	goto Menu_Navigation_Init

NMEA_Menu_Waypoint
	;Check that there is data to display
	btfss NMEA_new_waypt_data_flag
	goto Menu_Navigation
	
	;Clear the new data flag
	bcf NMEA_new_waypt_data_flag

	call LCD_clear
	LCDPrintString Waypt_Headerstring
	call LCD_line_2
	LCDPrintString Waypoint_String

	btfss NMEA_waypt_obtained
	bra NMEA_Menu_Waypoint_No_Waypoint

	movlw LOW(NMEA_Waypoint_Name)
	movwf FSR0L
	movlw HIGH(NMEA_Waypoint_Name)
	movwf FSR0H
	call LCD_PrintIDR1String
	bra NMEA_Menu_Waypoint_bearing

NMEA_Menu_Waypoint_No_Waypoint
	LCDPrintString NoData_String	

NMEA_Menu_Waypoint_bearing
	call LCD_line_3
	LCDPrintString Br_String

	btfss NMEA_waypt_bearing_valid
	bra NMEA_Menu_Waypoint_no_bearing

	movff NMEA_Bearing_H, BIN_H
	movff NMEA_Bearing_L, BIN_L
	call BIN_to_BCD

	movf	R1,W
	call	W_2HEX_to_LCD
	movf	R2,W
	call	W_2HEX_to_LCD

	bra NMEA_Menu_Waypoint_heading

NMEA_Menu_Waypoint_no_bearing
	LCDPrintString NoData_String

NMEA_Menu_Waypoint_heading
	LCDPrintString Hd_String

	btfss NMEA_waypt_heading_valid
	bra NMEA_Menu_Waypoint_no_heading

	movff NMEA_Heading_H, BIN_H
	movff NMEA_Heading_L, BIN_L
	call BIN_to_BCD

	movf	R0,W
	call	W_1HEX_to_LCD
	movf	R1,W
	call	W_2HEX_to_LCD
	movf	R2,W
	call	W_2HEX_to_LCD

	goto Menu_Navigation

NMEA_Menu_Waypoint_no_heading
	LCDPrintString NoData_String

	goto Menu_Navigation


;*****************************************************************************************
;Prints the fix data

Fix_Headerstring db "Fix:",0x00
Lat_string db "Lat:",0x00
Lon_string db "Lon:",0x00

NMEA_Menu_Fix_init
	bsf NMEA_new_fix_data_flag
	goto Menu_Navigation_Init

NMEA_Menu_Fix
	;Check that there is data to display
	btfss NMEA_new_fix_data_flag
	goto Menu_Navigation
	
	;Clear the new data flag
	bcf NMEA_new_fix_data_flag

	call LCD_clear
	LCDPrintString Fix_Headerstring

	btfss NMEA_time_valid
	bra NMEA_Menu_Fix_invalid

	movlw LOW(NMEA_clock)
	movwf FSR0L
	movlw HIGH(NMEA_clock)
	movwf FSR0H
	call LCD_PrintIDR1String
	
	call LCD_line_2
	LCDPrintString Lat_string
	movlw LOW(NMEA_Latitude)
	movwf FSR0L
	movlw HIGH(NMEA_Latitude)
	movwf FSR0H
	call LCD_PrintIDR1String

	call LCD_line_3
	LCDPrintString Lon_string
	movlw LOW(NMEA_Longitude)
	movwf FSR0L
	movlw HIGH(NMEA_Longitude)
	movwf FSR0H
	call LCD_PrintIDR1String

	goto Menu_Navigation

NMEA_Menu_Fix_invalid
	LCDPrintString NoData_String	

NMEA_Menu_Fix_position

	goto Menu_Navigation
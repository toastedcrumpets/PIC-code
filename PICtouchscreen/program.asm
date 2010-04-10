 PROCESSOR 18F4620
 CONFIG  OSC=HSPLL,FCMEN=OFF,IESO=OFF,PWRT=OFF,BOREN=OFF,BORV=2,WDT=OFF,WDTPS=32768,MCLRE=ON,LPT1OSC=OFF,PBADEN=OFF,XINST=OFF,LVP=OFF,STVREN=ON
 ;OSC=IRCIO67

 org 0x0000
	goto INIT

 cblock 0x00
w_temp
 endc

 include <p18f4680.inc>
 include "macros.inc"
 include "24bitMacros.inc"
 include "divide.inc"



 ;LCD display
 #define LCD_RS  LATB,2
 #define LCD_SCL LATB,3
 #define LCD_SI  LATB,4
 #define LCD_grfx
 include "grfxLCD.inc"

 ;Touch screen

 include "touchPanel.inc"
 include "framebuffer.inc"
 include "textBlit.inc"
 include "touchmenu.inc"

 ;SD card
 #define SD_CS LATD,2
 #define SD_DI LATD,3
 #define SD_CLK LATD,4
 #define SD_DO PORTD,5
 ;This defines the start of the 512 byte scratchpad for the SD card
 #define SD_load_buffer_H movlw 0x08
 include "SDCard.inc"

 cblock
sense_ctr
 endc

FAT_init_fail db "Failed to init the filesystem", 0x00
SD_init_fail_1 db "Failed to init the SD card", 0x00
SD_init_fail_2	db "Please cycle the power", 0x00

INIT
	;//////////////////////////////////////
	;Establish the lines for the SD card
	clrf TRISD
	;Data input line
	bsf TRISD,5

	;//////////////////////////////////////
	;Setup the display
	;Set portB to output for the LCD
	clrf TRISB
	;Pull down B5 to reset the display
	clrf LATB
	call delay_1ms
	;Enable the display
	bsf LATB,5
	call delay_1ms
	call LCD_INIT
	call LCD_clear

	;////////////////////////////
	;Setup the function generator communication
	call FuncGen_Mode_setup

	;//////////////////////////////////////
	;Setup the touch panel
	call touch_init


	;//////////////////////////////////////
	;All of the SD card stuff
	call SD_init
	xorlw 0x00
	bz SD_init_success

	;SD init failed! Inform the user
	call blank_framebuffer
	call Load_framebuffer
	movlw .4
	call fb_goto_row
	movlw .10
	call fb_goto_col
	LoadTable(SD_init_fail_1)
	call blit_print_table_string

	call Load_framebuffer
	movlw .3
	call fb_goto_row
	movlw .20
	call fb_goto_col
	LoadTable(SD_init_fail_2)
	call blit_box_print_table_string
	
	call transmit_framebuffer
	bra $

SD_init_success	
	call SD_Mode_setup
	xorlw .0
	bz FAT_init_success
	;FAT init failed! Inform the user
	call blank_framebuffer
	call Load_framebuffer
	movlw .4
	call fb_goto_row
	movlw .10
	call fb_goto_col
	LoadTable(FAT_init_fail)
	call blit_print_table_string

	call Load_framebuffer
	movlw .3
	call fb_goto_row
	movlw .20
	call fb_goto_col
	LoadTable(SD_init_fail_2)
	call blit_box_print_table_string
	
	call transmit_framebuffer
	bra $

FAT_init_success
	;Entry vector for the menu
	call Main_Mode_Init

MAIN
	call Menu_ticker
	bra MAIN

 include "DrawMode.inc"
 include "FunGenMode.inc"
 include "MainMenuMode.inc"
 include "SDMode.inc"
 include "FAT32.inc"
 include "chartable.inc"
 end
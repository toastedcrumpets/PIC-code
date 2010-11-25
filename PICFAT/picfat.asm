;An example of use for the PICFAT include file
PROCESSOR 18F4680
 CONFIG  OSC=HSPLL,FCMEN=OFF,IESO=OFF,PWRT=OFF,BOREN=OFF,BORV=2,WDT=OFF,WDTPS=32768,MCLRE=ON,LPT1OSC=OFF,PBADEN=OFF,XINST=OFF,LVP=OFF,STVREN=ON

 org 0x0000
	goto INIT

 ;Get the standard definitions for this PIC
 include <p18f4680.inc>
 include "macros.inc"

 ;SD card data lines
 #define SD_CS LATD,2
 #define SD_DI LATD,3
 #define SD_CLK LATD,4
 #define SD_DO PORTD,5

 ;This defines the start of the 512 byte scratchpad for the SD card
 #define SD_load_buffer_H movlw 0x08

 include "SDCard.inc"

 ;FAT32 library
 include "FAT32.inc"

;Some file and folder names to manipulate
file1      db "TEST    PDF"
file2      db "NEWFILE TXT"
folder1    db "TESTDIR    "

INIT
	;Setup the direction of the SD card lines
	clrf TRISD
	;DI must be set as an input
	bsf TRISD,5

	;Initialise the SD_card 
	call SD_init
	xorlw 0x00
	bz init_filesystem //Jump to fat init on success

	;Try again
	call SD_init
	xorlw 0x00
	bz init_filesystem //Jump to fat init on success

	;Try again
	call SD_init
	xorlw 0x00
	bnz SD_MODE_FAT_fail //Fail if the third time fails

init_filesystem
	;You must check WREG for non-zero values indicating failure of the FAT command.
	call FAT_init
	TSTFSZ WREG
	bra SD_MODE_FAT_fail

	
	;Load the root directory, the FAT library remembers the current directory
	call FAT_load_root

	;////////////////////////////////////////////////
	;Delete file1="test.pdf" in the root directory

	;Load the file name into entryname
	LoadTable file1
	call FAT_load_filename_TBLPTR

	call FAT_delete_entry
	
	;Here we're allowing the delete to fail, as you might not have put a file called test.pdf on the SD card
	;xorlw .0
	;bnz SD_MODE_DELETE_fail

	;/////////////////////////////////////////////////////////////////////////////////////////
	;Make a new file (file2="NEWFILE.TXT") which is 
	;two clusters long 
	;(a cluster is *usually* 8 sectors 8x512=4096 bytes
	;This file is then filled with M characters to demonstrate writing

	;Load the root directory (don't need to do this as the library remembers it's in the root)
	;call FAT_load_root

	;Load the new file name into entryname
	LoadTable file2
	call FAT_load_filename_TBLPTR

	;Normal files have no attributes (see the FAT spec for the different possible attributes)
	clrf FAT_newfile_attrib

	;Set the number of clusters for the file, (going smaller than a cluster doesn't save space and is not supported)
	movlw .2
	movwf FAT_newfile_size 

	;Make the new file, if the file size >0 then the file is also opened by FAT_new_entry
	call FAT_new_entry
	xorlw .0
	bnz SD_MODE_FAT_fail

	;FAT_new_entry leaves us at the first sector of the new file in SD_addr
	;We're ready to fill the file with data
	;//NOTE: the SD buffer is not "safe" across FAT calls, the FAT calls use 
	;it, so you must first allocate files then write/generate data for them


	;Fill the SD buffer with some example data (In this case, M characters)
	SD_load_buffer_FSR 2,0x000

Fill_SD_buffer_loop
	movlw "M"
	movwf POSTINC2

	SD_load_buffer_H + 2
	xorwf FSR2H,W
	bnz	Fill_SD_buffer_loop

	;Buffer is full, write it to every sector of the file
Fill_file_loop
	call SD_write
	xorlw .0
	bnz SD_MODE_FAT_fail

	;Fetch the next sector of the file
	call FAT_next_sector
	xorlw .0
	bz Fill_file_loop

	;We're here because FAT_next_sector failed (probably due to a EOF)
	;Check it is an EOF and continue to the next example
	xorlw .1
	bnz SD_MODE_FAT_fail

	;///////////////////////////////////////////////////////////////////
	;Make a directory (folder1="TESTDIR") in the current directory and open it
	LoadTable folder1
	call FAT_load_filename_TBLPTR

	;Set the entry as a folder
	clrf FAT_newfile_attrib
	;Add the directory bit
	bsf FAT_newfile_attrib,4

	;No need to set the number of clusters, the driver handles that
	
	;Make the new directory
	call FAT_new_entry
	xorlw .0
	bnz SD_MODE_FAT_fail

	;We're now inside the new directory

	;/////////////////////////////////////////////////
	;Goes up one directory, then back into the directory (to show directory navigation) and make
	;a blank new file in it of zero size (to test it!)

	;Go up a directory
	LoadTable FAT_prev_dir
	call FAT_load_filename_TBLPTR
	call FAT_open_entry
	xorlw .0
	bnz SD_MODE_FAT_fail

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
	

	;/////////////////////////////////////////////////
	;Goes to the root dir and reads the file we made before

	;Load the root directory
	call FAT_load_root

	;Load the new file name into entryname
	LoadTable file2
	call FAT_load_filename_TBLPTR

	call FAT_open_entry
	xorlw .0
	bnz SD_MODE_FAT_fail

	call convert_cluster_in_addr

Read_file_loop
	call SD_read
	xorlw .0
	bnz SD_MODE_FAT_fail		
	
	SD_load_buffer_FSR 2,0x000
	;Do something with each sector (512 bytes) of data pointed to by FSR2

	;Fetch the next sector of the file
	call FAT_next_sector
	xorlw .0
	bz Read_file_loop

	;We're here because FAT_next_sector failed (probably due to a EOF)
	;Check it is an EOF
	xorlw .1
	bnz SD_MODE_FAT_fail

	;////////////////////////////////////////////////////
	;End of demo

SD_MODE_FAT_fail
	;Now go into an infinite loop
	bra $

end
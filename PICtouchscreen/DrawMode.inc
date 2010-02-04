;Drawing mode for the touch screen


;//////////////////////////////////////
;Sets up the touch panel for drawing mode
Drawing_Mode_Init
	call blank_framebuffer
	call clear_TouchMenu

	movlw LOW(Drawing_Handler)
	movwf fill_touch_menu_vec_L
	movlw HIGH(Drawing_Handler)
	movwf fill_touch_menu_vec_H
	movlw UPPER(Drawing_Handler)
	movwf fill_touch_menu_vec_U
	call fill_TouchMenu
	return

;//////////////////////////////////////////////
;Draw Mode touch handlers

;/////////////
;This handler draws a pixel
Drawing_Handler
	movff x_pos,pixel_x
	bcf STATUS,C
	rrcf pixel_x

	movff y_pos,pixel_y
	bcf STATUS,C
	rrcf pixel_y
	bcf STATUS,C
	rrcf pixel_y

	call fb_set_pixel
	return
;24 bit macros
;////////////////////////////////////////////////////////
;Helper macros and functions
x2_24bit macro in
	bcf STATUS,C
	rlcf in+0,F
	rlcf in+1,F
	rlcf in+2,F
	endm

div2_24bit macro in
	bcf STATUS,C
	rrcf in+2,F
	rrcf in+1,F
	rrcf in+0,F
	endm

add_16bit_to_24bit macro b,a
	movf b+0,W
	addwf a+0,F
	movf b+1,W
	addwfc a+1,F
	clrf WREG
	addwfc a+2,F
	endm

add_24bit macro a,b
	movf b+0,W
	addwf a+0,F
	movf b+1,W
	addwfc a+1,F
	movf b+2,W
	addwfc a+2,F
	endm

incf_24bit macro a
	incfsz a+0,F
	bra $+.6
	infsnz a+1,F
	incf a+2,F
	endm

decf_24bit macro a
	decfsz a+0,F
	bra $+.6
	dcfsnz a+1,F
	decf a+2,F
	endm

sub_24bit macro a,b
	movf b+0,W
	subwf a+0,F
	movf b+1,W
	subwfb a+1,F
	movf b+2,W
	subwfb a+2,F
	endm

sub_w_24bit macro a
	subwf a+0,F
	movlw .0
	subwfb a+1,F
	movlw .0
	subwfb a+2,F
	endm

add_w_24bit macro a
	addwf a+0,F
	movlw .0
	addwfc a+1,F
	movlw .0
	addwfc a+2,F
	endm

cp_24bit macro a,b
	movff a+0,b+0
	movff a+1,b+1
	movff a+2,b+2
	endm

swap_24bit macro a,b
	movf a+0,W
	movff b+0,a+0
	movwf b+0

	movf a+1,W
	movff b+1,a+1
	movwf b+1

	movf a+2,W
	movff b+2,a+1
	movwf b+2
	endm

cp_FSR_24bit macro a,b
	movff POSTINC#v(a),b+0
	movff POSTINC#v(a),b+1
	movff POSTINC#v(a),b+2
	endm
	
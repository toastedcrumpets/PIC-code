;Sine wave generator
 cblock
x1,index,product
 endc

#define INDEX_MASK  0x0f
#define INTERP_MASK 0xf0

;--------------------------------------------------------
;sine
;
;  The purpose of this routine is to take the sine of the
;16-bit frequency variable f_hi:f_lo to produce a signed
;8-bit result that is within +/- 1 count of the true sine
;value.
;  Only the lower 14 bits of the frequency variable are
;actually used. The frequency variable maps into degrees
;by the following transfer function:
;  degrees = (f & 0x3fff) * 360 / 0x4000
;  The sine output is approximately
;sine = int(127*sin( (f & 0x3fff) * 360 / 0x4000) )
; where
; sin() is the true sine function
; int() is the nearest integer function
;
;The technique used to obtain the sine value is a combination
;of table look-up and first order linear interpolation. Sixteen
;equidistant frequency samples of the first quadrant of sin(x) 
;are stored in sine_table.
;
; The frequency variable is broken down as follows:
; xxQQTTTT IIIIPPPP
; where 
;  xx - don't care
;  QQ - Quadrant: 00 = quadrant 1, 01 = quadrant 2, etc.
;  TTTT - Index into the sine_table.
;  IIII - Interpolation between successive entries in the table
;  PPPP - Phase accumulation (not needed in this function, it's
;         only used to increase the dynamic range in frequency
;         steps).
;Once the frequency has been broken down in to these parts, the
;sine function for the first quadrant can be calculated as follows:
;  x1 = sine_table[index]
;  x2 = sine_table[index+1]
;  sine = x1 + ((x2-x1) * interp) / 16
;The first term, x1, is seen to be a first order approximation to
;sine function. The second term improves on that approximation by
;interpolating between x1 and x2. The interpolation variable interp
;is 0 <= interp <= 15 and it is divided by 16. Consequently, the
;interpolation factor ranges between 0 and 15/16.
;
;The sine function in the other three quadrants can be obtained
;from calculations based on the first quadrant by using the following
;trig identities:
; first, let 0 <= f <= 90, i.e. f covers the first quadrant.
;  quadrant 2:  u = 90 + f,   90 < u < 180
;     sin(u-90) = sin(f)
;     x1 = sine_table(16-index), x2 = sine_table(15-index)
;  quadrant 3:  u = 180 + f,  180 < u < 270
;     sin(u) = sin(f+180) = -sin(f)
;     x1 = -sine_table(index), x2 = -sine_table(index+1)
;  quadrant 4:  u = 270 + f,  270 < u < 360
;     sin(u-90) = sin(f+180) = -sin(f)
;     x1 = -sine_table(16-index), x2 = -sine_table(15-index)
;
;Thus, for quadrants 2 and 4, the sine table is indexed in reverse
;order and for quadrants 3 and 4 the values from the sine table 
;are negated. A slight change is made on this indexing and negation
;scheme so that the operation (x2-x1) * interp / 16 only deals with
;positive numbers. This significantly simplifies the multiplication.
;The modification changes the formula for each quadrant as follows:
; quadrant 1:   (no change)
;   x1 = sine_table[index],  x2 = sine_table[index+1]
;   sine = x1 + ((x2-x1) * interp) / 16
; quadrant 2:
;   x1 = sine_table[15-index],  x2 = sine_table[16-index]
;   sine = x2 - ((x2-x1) * interp) / 16
; quadrant 3:
;   x1 = sine_table[index],  x2 = sine_table[index+1]
;   sine = -(x1 + ((x2-x1) * interp) / 16)
; quadrant 4:
;   x1 = sine_table[15-index],  x2 = sine_table[16-index]
;   sine = -(x2 - ((x2-x1) * interp) / 16)
;
;Input
; f_hi:f_lo  -  16-bit frequency variable
;Output
; W = int(127*sin( (f & 0x3fff) * 360 / 0x4000) )
;
;Execution time: 30 Cycles (for all cases)
;
sine_interp
    ;Get the 4-bit index
    movf    f_hi,W
    andlw   INDEX_MASK
    btfsc   f_hi,4
     sublw  .15                   ;Odd quadrants, index = 15 - index

	addlw	LOW(sine_interp_table)
	movwf	TBLPTRL

	clrf TBLPTRH
	movlw 	HIGH(sine_interp_table)
	addwfc	TBLPTRH,F

	clrf TBLPTRU
	movlw 	UPPER(sine_interp_table)
	addwfc	TBLPTRU,F

    tblrd*+                   ;Get x1=sin(index)
    movf    TABLAT,w
	movwf	x1

    tblrd*                       ;Get x2=sin(index+1)
    subwf   TABLAT,w              ;W=x2-x1, This is always positive. 
	movwf	product

    ;multiply interp and x2 - x1 and divide by 16. This is actually a 4 by 8
    ;bit multiplication.
    ;

	movlw	0xf0
	andwf	f_lo,w
	mulwf	product

	movf	PRODH,w		;Round up if fractional part is > 1/2
	btfsc	PRODL,7
	 incf	PRODH,w


    btfss   f_hi,4
     addwf  x1,W
    btfsc   f_hi,4
     subwf  TABLAT,w		;x2 is in the tablat register
    btfsc   f_hi,5
     sublw  0
	
    return

sine_interp_table
        db  .128,.140       ;+128 127*sin(0 * 90/16)
                        ;127*sin(1 * 90/16)
        db  .153,.165      ;127*sin(2 * 90/16)
                        ;127*sin(3 * 90/16)
        db  .177,.188      ;127*sin(4 * 90/16)
                        ;127*sin(5 * 90/16)
        db  .199,.209      ;127*sin(6 * 90/16)
                        ;127*sin(7 * 90/16)
        db  .218,.226      ;127*sin(8 * 90/16)
                        ;127*sin(9 * 90/16)
        db  .234,.240    ;127*sin(10 * 90/16)
                        ;127*sin(11 * 90/16)
        db  .245,.250    ;127*sin(12 * 90/16)
                        ;127*sin(13 * 90/16)
        db  .253,.254    ;127*sin(14 * 90/16)
                        ;127*sin(15 * 90/16)
        db  .255,.0      ;127*sin(16 * 90/16)

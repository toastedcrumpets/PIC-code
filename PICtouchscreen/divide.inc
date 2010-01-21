 cblock
x:2
y
temp:3
 endc

; x = 256*x/y
;
; Input:
;  x - 8 bit unsigned integer
;  y - 8 bit unsigned integer (unmodified)
; Output:
;  x, x+1 - 16 bit unsigned integer (x - LSB, x+1 - MSB)
; Temporary:
;  temp, x+1 - current result/counter
;  temp+1, temp+2 - current remainder

div8scaled8
        clrf temp               ;prepare temp regs
        clrf temp+1
        clrf temp+2
        clrf x+1
        bsf temp, 0             ;set counter bit

        bcf STATUS,C

div8scaled8_loop
        rlcf x, f                ;shift in zero and shift out next divisor bit
        rlcf temp+1, f
        rlcf temp+2, f

        movf y, w               ;move y to w
        btfss temp, 0
         goto div8scaled8_add
        subwf temp+1, f
        movlw 1
        btfss STATUS,C
         subwf temp+2, f
        goto div8scaled8_next

div8scaled8_add
        addwf temp+1, f
        movlw 1
        btfsc STATUS,C
         addwf temp+2, f

div8scaled8_next
        rlcf temp, f             ;shift in next result
        rlcf x+1, f
        btfss STATUS,C                    ;repeat until carry is set
         goto div8scaled8_loop

        movf temp, w            ;move result to x
        movwf x
        return                  ;done!

 cblock
RES0,RES1,RES2,RES3
DSOR_H,DSOR_L
CNTR_SHIFTS
 endc

DIV_3216U_ATFN               ;unsigned 32/16 bits values division
;To call the routine:
;dividend in RES3:0 (max val H'FFFE0001' =4.294.836.225) (32 bits).
;divisor in DSOR_H:L (max value H'FFFF' =65.535) (16 bits).
;User to ensure being within range or if a division by zero is going to happen.
;The routine gives:
;quotient in RES1:0 (16 bits)
;reminder in RES3:2 (16 bits)
;Result of RES3:RES0 / DSOR_1:DSOR_0 => RES1:RES0 (quotient)
;Remainder in RES3:RES2
    MOVLW 16                ;initialize the
    MOVWF CNTR_SHIFTS       ;shiftings counter

DIV_3216U_ATFN_LOOP
    BCF STATUS,C            ;we ensure that b0
    RLCF RES0,F           ;of RES0 is =0
    RLCF RES1,F           ;after the shifting
    RLCF RES2,F           ;to the left of the
    RLCF RES3,F           ;whole dividend
;We test C. If =1, shifting caused an o'flow! Substract DSOR_H:L from RES3:2
    BC DIV_3216U_ATFN_SUBST_DSOR
    MOVF DSOR_H,W           ;compare DSOR_H
    SUBWF RES3,W          ;against RES3

;We test Z. If =0, DSOR_H != RES3. Chech if  DSOR_H < RES3   
    BNZ DIV_3216U_CHKIF_DSOR_LOW_RES
    MOVF DSOR_L,W           ;DSOR_H = RES3; now compare
    SUBWF RES2,W          ;value of DSOR_L against RES2

DIV_3216U_CHKIF_DSOR_LOW_RES   
;We test C. If =1, DSOR_  < RES - Substract DSOR_H:L from RES3:2
    BNC DIV_3216U_ATFN_DECR_CNTR_SH

DIV_3216U_ATFN_SUBST_DSOR   ;substract DSOR from MSBs of dividend
    MOVF DSOR_L,W           ;RES2 = RES2 - DSOR_L
    SUBWF RES2,F          ;Borrow not used (LSB)
    MOVF DSOR_H,W           ;RES3 = RES3 - DSOR_H
    SUBWFB RES3,F         ;Borrow used
    BSF RES0,0            ;flag "divisor was substracted from dividend"

DIV_3216U_ATFN_DECR_CNTR_SH
    DECFSZ CNTR_SHIFTS,F
    BRA DIV_3216U_ATFN_LOOP
    RETURN                  ;job done!


 cblock
DSOR_0,DSOR_1,DSOR_2,DSOR_3,QUOT_H,QUOT_L,PNTR_DSOR
 endc
DIV_3216U_KEN               ;unsigned 32/16-bit values division (KENYAN)

;Agustin T. Ferrari Nicolay - Buenos Aires - April 2007

;Algorithm implemented:

;Given RES and DSOR, initialize QUOT =0 and INDEX_DSOR =0.
;Duplicate DSOR repeatedly. Increase PNTR_DSOR every time DSOR is doubled
;Repeat doubling to get a DSOR equal to, or the closest below RES.
;Substract the highest DSOR from RES setting b0 of QUOT.
;Shift QUOT to left and decrement PNTR_DSOR.
;Succesively try to substract from the resulting remainder above, every DSOR
;value, down in the list.
;For every possible substraction, keep setting b0 of QUOT and shifting it
;to left. If not possible, just shift QUOT to the left. In any case, always
;decrement PNTR_DSOR.
;Once finished (PNTR_DSOR again =0), the remainder is the result of the last
;substraction. QUOT in the corresponding register.
;------------------------------------------------------------------------------

;To call the routine:
;dividend in RES3:0 (max val H'FFFE 0001' = H'FFFF' * H'FFFF' =4.294.836.225)
;divisor in DSOR_1:0 (max value H'FFFF' =65.535) - DSOR_3:2 used internally in
;the routine

;User to ensure being within range or if a division by zero is attempted.

;The routine gives:
;Result of RES3:RES0 / DSOR_1:DSOR_0 => QUOT_H:QUOT_L.
;Remainder in RES3:0

    CLRF DSOR_3             ;clear them
    CLRF DSOR_2             ;as a precaution
    CLRF QUOT_H
    CLRF QUOT_L
    CLRF PNTR_DSOR

DIV_3216U_KEN_INC_DSOR_LOOP
    BCF STATUS,C            ;ensure b0 of
    RLCF DSOR_0,F           ;DSOR_0 is clear
    RLCF DSOR_1,F           ;after the shifting
    RLCF DSOR_2,F           ;to the right to get
    RLCF DSOR_3,F           ;DSOR =DSOR*2
    
    INCF PNTR_DSOR,F        ;we go for a new value (double of previous DSOR)

;the following is to check if DSOR exceeded RES (if so, stop doubling DSOR)

    MOVF DSOR_3,W           ;we test DSOR_3 against RES3
    SUBWF RES3,W          ;to see if they are equal
    BNZ CHKIF_DSOR3_GT_RES3;they are not - check if DSOR > RES
    
    MOVF DSOR_2,W           ;we test DSOR_2 against RES2
    SUBWF RES2,W          ;to see if they are equal
    BNZ CHKIF_DSOR2_GT_RES2;they are not - check if DSOR > RES

    MOVF DSOR_1,W           ;we test DSOR_1 against RES1
    SUBWF RES1,W          ;to see if they are equal
    BNZ CHKIF_DSOR1_GT_RES1;they are not - check if DSOR > RES
    
    MOVF DSOR_0,W           ;we test DSOR_0 against RES0
    SUBWF RES0,W          ;to see if DSOR >RES - If so, start substraction
    BC DIV_3216U_KEN_INC_DSOR_LOOP;otherwise, go for next doubling of DSOR
    
DIV_3216U_KEN_SUBST_DSOR_LOOP
    TSTFSZ PNTR_DSOR
    BRA DIV_3216U_KEN_DECR_PNTR
    RETURN

DIV_3216U_KEN_DECR_PNTR
    DECF PNTR_DSOR          ;we look down in the list of double DSOR values
    
    BCF STATUS,C            ;ensure b0 of QUOT_L is clear after shifting
    RLCF QUOT_L,F           ;shift QUOT to the left to have it ready
    RLCF QUOT_H,F           ;for next substraction

    BCF STATUS,C            ;ensure b7 of DSOR_3 is clear after shifting
    RRCF DSOR_3,F           ;shift DSOR
    RRCF DSOR_2,F           ;to the right
    RRCF DSOR_1,F           ;to get
    RRCF DSOR_0,F           ;DSOR =DSOR/2 

;the following is to check if DSOR <RES (if so, substract DSOR from RES)
    MOVF DSOR_3,W           ;we test DSOR_3 against RES3
    SUBWF RES3,W          ;to see if they are equal
    BNZ CHKIF_DSOR3_LT_RES3
    
    MOVF DSOR_2,W           ;we test DSOR_2 against RES2
    SUBWF RES2,W          ;to see if they are equal
    BNZ CHKIF_DSOR2_LT_RES2
    
    MOVF DSOR_1,W           ;we test DSOR_1 against RES1
    SUBWF RES1,W          ;to see if they are equal
    BNZ CHKIF_DSOR3_LT_RES3
    
    MOVF DSOR_0,W           ;we test DSOR_0 against RES0
    SUBWF RES0,W          ;to see if DSOR may be susbtracted from RES
    BNC DIV_3216U_KEN_SUBST_DSOR_LOOP;DSOR >RES - get next double DSOR value

DIV_3216U_KEN_SUBST_DSOR    ;substract DSOR_3:0 from RES3:0
    MOVF DSOR_0,W           ;LSB, borrow
    SUBWF RES0,F          ;is NOT used

    MOVF DSOR_1,W           ;borrow
    SUBWFB RES1,F         ;IS used

    MOVF DSOR_2,W           ;borrow
    SUBWFB RES2,F         ;IS used

    MOVF DSOR_3,W           ;borrow
    SUBWFB RES3,F         ;IS used

    BSF QUOT_L,0            ;flag "a valid substraction from dividend occurred"
    BRA DIV_3216U_KEN_SUBST_DSOR_LOOP

CHKIF_DSOR3_GT_RES3
    BNC DIV_3216U_KEN_SUBST_DSOR_LOOP
    BRA DIV_3216U_KEN_INC_DSOR_LOOP

CHKIF_DSOR2_GT_RES2
    BNC DIV_3216U_KEN_SUBST_DSOR_LOOP
    BRA DIV_3216U_KEN_INC_DSOR_LOOP

CHKIF_DSOR1_GT_RES1
    BNC DIV_3216U_KEN_SUBST_DSOR_LOOP
    BRA DIV_3216U_KEN_INC_DSOR_LOOP

CHKIF_DSOR3_LT_RES3
    BC DIV_3216U_KEN_SUBST_DSOR
    BRA DIV_3216U_KEN_SUBST_DSOR_LOOP

CHKIF_DSOR2_LT_RES2
    BC DIV_3216U_KEN_SUBST_DSOR
    BRA DIV_3216U_KEN_SUBST_DSOR_LOOP

CHKIF_DSOR1_LT_RES1
    BC DIV_3216U_KEN_SUBST_DSOR
    BRA DIV_3216U_KEN_SUBST_DSOR_LOOP
;------------------------------------------------------------------------------

 cblock
NROA_H,NROA_L
NROB_H,NROB_L
 endc
MUL_1616U                   ;unsigned 16-bit values multiplication
;Straight from the manual for the 18F452. Nothing original from my side.
;To call the routine:
;multiplicands in NROA_H:NROA_L and NROB_H:NROB_L.
;Maximum value => H'FFFF' * H'FFFF' =H'FFFE0001'

;Result of NROA * NROB => RES3:RES0 (32 bits)

    MOVF NROA_L,W
    MULWF NROB_L            ;NROA_L*NROB_L => PRODH:PRODL
    MOVFF PRODH,RES1
    MOVFF PRODL,RES0
    
    MOVF NROA_H,W
    MULWF NROB_H            ;NROA_H*NROB_H => PRODH:PRODL
    MOVFF PRODH,RES3
    MOVFF PRODL,RES2
    
    MOVF NROA_L,W
    MULWF NROB_H            ;NROA_L*NROB_H => PRODH:PRODL
    MOVF PRODL,W
    ADDWF RES1,F           ;add cross
    MOVF PRODH,W            ;products
    ADDWFC RES2,F
    CLRF WREG
    ADDWFC RES3,F
    
    MOVF NROA_H,W
    MULWF NROB_L            ;NROA_H*NROB_L => PRODH:PRODL
    
    MOVF PRODL,W
    ADDWF RES1,F           ;add cross
    MOVF PRODH,W            ;products
    ADDWFC RES2,F
    CLRF WREG
    ADDWFC RES3,F

    RETURN                  ;result in RES3:RES0

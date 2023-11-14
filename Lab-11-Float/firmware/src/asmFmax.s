/*** asmFmax.s   ***/
#include <xc.h>
.syntax unified

@ Declare the following to be in data memory
.data  

@ Define the globals so that the C code can access them

.global f1,f2,fMax,signBitMax,biasedExpMax,expMax,mantMax
.type f1,%gnu_unique_object
.type f2,%gnu_unique_object
.type fMax,%gnu_unique_object
.type signBitMax,%gnu_unique_object
.type biasedExpMax,%gnu_unique_object
.type expMax,%gnu_unique_object
.type mantMax,%gnu_unique_object

.global sb1,sb2,biasedExp1,biasedExp2,exp1,exp2,mant1,mant2
.type sb1,%gnu_unique_object
.type sb2,%gnu_unique_object
.type biasedExp1,%gnu_unique_object
.type biasedExp2,%gnu_unique_object
.type exp1,%gnu_unique_object
.type exp2,%gnu_unique_object
.type mant1,%gnu_unique_object
.type mant2,%gnu_unique_object
 
.align
@ use these locations to store f1 values
f1: .word 0
sb1: .word 0
biasedExp1: .word 0  /* the unmodified 8b exp value extracted from the float */
exp1: .word 0
mant1: .word 0
 
@ use these locations to store f2 values
f2: .word 0
sb2: .word 0
exp2: .word 0
biasedExp2: .word 0  /* the unmodified 8b exp value extracted from the float */
mant2: .word 0
 
@ use these locations to store fMax values
fMax: .word 0
signBitMax: .word 0
biasedExpMax: .word 0
expMax: .word 0
mantMax: .word 0

.global nanValue 
.type nanValue,%gnu_unique_object
nanValue: .word 0x7FFFFFFF            

@ Tell the assembler that what follows is in instruction memory    
.text
.align

/********************************************************************
 function name: initVariables
    input:  none
    output: initializes all f1*, f2*, and *Max varibales to 0
********************************************************************/
.global initVariables
 .type initVariables,%function
initVariables:
    /* YOUR initVariables CODE BELOW THIS LINE! Don't forget to push and pop! */
    
    PUSH {R4-R11,LR} @ push registers R4-R11 onto the stack
    LDR R4, =0 @ zero register for initVaribles
    
    /* initialize f1* variables to 0 */
    LDR R5, =f1
    STR R4, [R5]
    LDR R5, =sb1
    STR R4, [R5] 
    LDR R5, =biasedExp1
    STR R4, [R5]
    LDR R5, =exp1
    STR R4, [R5]
    LDR R5, =mant1 
    STR R4, [R5]
    
    /* initialize f2* variables to 0 */
    LDR R5, =f2 
    STR R4, [R5]
    LDR R5, =sb2
    STR R4, [R5] 
    LDR R5, =biasedExp2
    STR R4, [R5]
    LDR R5, =exp2
    STR R4, [R5]
    LDR R5, =mant2
    STR R4, [R5]
    
    /* initialize *Max variables to 0 */
    LDR R5, =fMax
    STR R4, [R5]
    LDR R5, =signBitMax
    STR R4, [R5] 
    LDR R5, =biasedExpMax
    STR R4, [R5]
    LDR R5, =expMax
    STR R4, [R5]
    LDR R5, =mantMax
    STR R4, [R5]
    
    POP {R4-R11,LR} @ pop registers R4-R11 off the stack
    BX LR	    @ return to caller
    
    /* YOUR initVariables CODE ABOVE THIS LINE! Don't forget to push and pop! */

    
/********************************************************************
 function name: getSignBit
    input:  r0: address of mem containing 32b float to be unpacked
            r1: address of mem to store sign bit (bit 31).
                Store a 1 if the sign bit is negative,
                Store a 0 if the sign bit is positive
                use sb1, sb2, or signBitMax for storage, as needed
    output: [r1]: mem location given by r1 contains the sign bit
********************************************************************/
.global getSignBit
.type getSignBit,%function
getSignBit:
    /* YOUR getSignBit CODE BELOW THIS LINE! Don't forget to push and pop! */
    
    PUSH {R4-R11,LR}	@ push registers R4-R11 onto the stack
    
    LDR R4, [R0]	@ load value of input to function
    LDR R5, =0x80000000 @ load bit mask into R5
    TST R4, R5		@ bitwise AND with input value and sign mask
    LDREQ R4, =0	@  if zero flag is set, the result is positive
    LDRNE R4, =1	@  if zero flag isn't set, the result is negative
    STR R4, [R1]	@ store the result into the sign bit address

    POP {R4-R11,LR}	@ pop registers R4-R11 off the stack
    BX LR		@ return to caller
    
    /* YOUR getSignBit CODE ABOVE THIS LINE! Don't forget to push and pop! */
    

    
/********************************************************************
 function name: getExponent
    input:  r0: address of mem containing 32b float to be unpacked
            r1: address of mem to store BIASED
                bits 23-30 (exponent) 
                BIASED means the unpacked value (range 0-255)
                use exp1, exp2, or expMax for storage, as needed
            r2: address of mem to store unpacked and UNBIASED 
                bits 23-30 (exponent) 
                UNBIASED means the unpacked value - 127
                use exp1, exp2, or expMax for storage, as needed
    output: [r1]: mem location given by r1 contains the unpacked
                  original (biased) exponent bits, in the lower 8b of the mem 
                  location
            [r2]: mem location given by r2 contains the unpacked
                  and UNBIASED exponent bits, in the lower 8b of the mem 
                  location
********************************************************************/
.global getExponent
.type getExponent,%function
getExponent:
    /* YOUR getExponent CODE BELOW THIS LINE! Don't forget to push and pop! */
    
    PUSH {R4-R11,LR} @ push registers R4-R11 onto the stack
    
    /* calculate the biased exponent */
    LDR R4, [R0]	@ load the value of the float into R4
    LDR R5, =0x7F800000 @ initialize r5 to hold bit mask for bits 23-30
    AND R4, R4, R5	@ isolate exponent bits using bitwise AND with the mask
    LSR R4, R4, 23	@ shift the exponent bits to the LSBs
    STR R4, [R1]	@ store the biased exponent to the passed R1 input address
    
    /* calculate the unbiased exponent 
    if the biased exponent is 0, set the biased to -126 */
    CMP R4, 0		    @ check if biased is zero
    LDREQ R4, =0xFFFFFF82   @ 32 bit 2's complement of -126 if so
    SUBNE R4, R4, 127	    @ otherwise unbiased exponent = biased exponent-127
    STR R4, [R2]	    @ store the unbiased exponent to passed R2 input address
    
    POP {R4-R11,LR} @ pop registers R4-R11 off the stack
    BX LR	    @ return to caller
    
    /* YOUR getExponent CODE ABOVE THIS LINE! Don't forget to push and pop! */
   

    
/********************************************************************
 function name: getMantissa
    input:  r0: address of mem containing 32b float to be unpacked
            r1: address of mem to store unpacked bits 0-22 (mantissa) 
                of 32b float. 
                Use mant1, mant2, or mantMax for storage, as needed
    output: [r1]: mem location given by r1 contains the unpacked
                  mantissa bits
********************************************************************/
.global getMantissa
.type getMantissa,%function
getMantissa:
    /* YOUR getMantissa CODE BELOW THIS LINE! Don't forget to push and pop! */
    
    PUSH {R4-R11,LR} /* push registers R4-R11 onto the stack */
    
    LDR R4, [R0]	@ load the float input into r4
    LDR R5, =0x7FFFFF	@ bit mask for bits 0-22 to be set to 1
    AND R4, R4, R5	@ isolate bits 0-22 with bitwise AND, bit mask, and float input
    STR R4, [R1]	@ store the mantissa value to the passed r1 input address
    
    POP {R4-R11,LR} @ pop registers R4-R11 off the stack
    BX LR	    @ return to caller
    
    /* YOUR getMantissa CODE ABOVE THIS LINE! Don't forget to push and pop! */
   


    
/********************************************************************
function name: asmFmax
function description:
     max = asmFmax ( f1 , f2 )
     
where:
     f1, f2 are 32b floating point values passed in by the C caller
     max is the ADDRESS of fMax, where the greater of (f1,f2) must be stored
     
     if f1 equals f2, return either one
     notes:
        "greater than" means the most positive numeber.
        For example, -1 is greater than -200
     
     The function must also unpack the greater number and update the 
     following global variables prior to returning to the caller:
     
     signBitMax: 0 if the larger number is positive, otherwise 1
     expMax:     The UNBIASED exponent of the larger number
                 i.e. the BIASED exponent - 127
     mantMax:    the lower 23b unpacked from the larger number
     
     SEE LECTURE SLIDES FOR EXACT REQUIREMENTS on when and how to adjust values!


********************************************************************/    
.global asmFmax
.type asmFmax,%function
asmFmax:   

    /* Note to Profs: Solution used to test c code is located in Canvas:
     *    Files -> Lab Files and Coding Examples -> Lab 11 Float Solution
     */

    /* YOUR asmFmax CODE BELOW THIS LINE! VVVVVVVVVVVVVVVVVVVVV  */
    
    PUSH {R4-R11,LR} /* push registers R4-R11 onto the stack */
    
    BL initVariables @ initialize variables to 0
    
    /* unpack float values */
    LDR R4, =f1	    @ load f1 address into R4
    LDR R5, =f2	    @ load f2 address into R5
    STR R0, [R4]    @ store the f1 value into the f1 address
    STR R1, [R5]    @ store the f2 value into the f2 address
    
    MOV R0, R4	    @ move the f1 address back into R0
    LDR R1, =sb1    @ load the address of sign bit 1 into R1
    BL getSignBit   @ call getSignBit with current R0 and R1 values as input
    
    MOV R0, R5	    @ move the f2 address back into R0
    LDR R1, =sb2    @ load the address of sign bit 2 into R1
    BL getSignBit   @ call getSignBit with current R0 and R1 values as input
    
    MOV R0, R4		@ move the f1 address back into R0
    LDR R1, =biasedExp1 @ load the address of biased exponent 1 into R1
    LDR R2, =exp1	@ load the address of exponent 1 into R2
    BL getExponent	@ call getExponent with current R0, R1, and R2 values as input
    
    MOV R0, R5		@ move the f2 address back into R0
    LDR R1, =biasedExp2 @ load the address of sign bit 2 into R1
    LDR R2, =exp2	@ load the address of exponent 2 into R2
    BL getExponent	@ call getExponent with current R0, R1, and R2 values as input
    
    MOV R0, R4	    @ move the f1 address back into R0
    LDR R1, =mant1  @ load the address of mantissa 1 into R1
    BL getMantissa  @ call getMantissa with current R0 and R1 values as input
    
    MOV R0, R5	    @ move the f2 address back into R0
    LDR R1, =mant2  @ load the address of mantissa 2 into R1
    BL getMantissa  @ call getMantissa with current R0 and R1 values as input
    
    /* load the required variables for mantissa adjustment */
    LDR R0, =mant1	@ load the address of mantissa 1 into R0
    LDR R1, [R0]	@ load the value of mantissa 1 into R1
    LDR R2, =biasedExp1 @ load the address of biased exponent 1 into R2
    LDR R2, [R2]	@ load the value of biased exponent 1 into R2
    LDR R3, =mant2	@ load the address of mantissa 2 into R3
    LDR R4, [R3]	@ load the value of mantissa 2 into R4
    LDR R5, =biasedExp2 @ load the address of biased exponent 2 into R5
    LDR R5, [R5]	@ load the value of biased exponent 2 into R5
    
    /* if biased exp is not zero, bit 23 has to be set to 1 */
    LDR R6, =0x800000	@ initialize R6 with bit mask for bit 23
    
    CMP R2, 0		@ check if f1 unbiased exponent is not zero
    ORRNE R1, R1, R6	@ set bit 23 to 1 with bitwise OR and bit mask
    STRNE R1, [R0]	@ store the adjusted mantissa back to the address
    
    CMP R5, 0		@ check if f2 unbiased exponent is not zero
    ORRNE R4, R4, R6	@ set bit 23 to 1 with bitwise OR and bit mask
    STRNE R4, [R3]	@ store the adjusted mantissa back to the address
    
    /* Check if f1 is NaN and handle infinity */
    specialCheck1:
    LDR R6, =biasedExp1 @ load biased exponent address into R6
    LDR R6, [R6]	@ load biased exponent value into R6
    
    /* NaN is specified as biased exponent of 255 and 
    mantissa value above 0 according to IEEE-754. */
    CMP R6, 255		@ check if biased exponent is 255
    BNE specialCheck2	@ branch to f2 check if not
    LDREQ R6, =mant1	@ load mantissa address into R6
    LDREQ R7, [R6]	@ load mantissa value into R7
    CMPEQ R7, 0x800000	@ check if the adjusted mantissa is 0
    
    /* if the mantissa is zero we have infinity */
    MOVEQ R0, 0		@ use R0 as an intermediate register to hold zero
    STREQ R0, [R6]	@  store zero back into the mantissa address
    BEQ specialCheck2	@ branch to the next check
    
    /* if mantissa is bigger than zero we have NaN */
    LDRGT R0, =fMax	    @ load fMax address into R0
    LDRGT R1, =0x7FFFFFFF   @ load NaN value into R1
    STRGT R1, [R0]	    @ store NaN value into fMax address
    BGT setf1Maxes	    @ branch to set rest of max variables to f1
    
    /* Check if f2 is NaN and handle infinity */
    specialCheck2:
    LDR R6, =biasedExp2 @ load biased exponent 2 address into R6
    LDR R6, [R6]	@ load the biased exponent 2 value into R6
    CMP R6, 255		@ check if biased exponent is 255
    BNE main		@ branch to main program if not
    LDREQ R6, =mant2	@ load the address of mantissa 2 into R6
    LDREQ R7, [R6]	@ load the value of mantissa 2 into R7
    CMPEQ R7, 0x800000	@ check if the adjusted mantissa is 0
    
    /* if the mantissa is zero we have infinity */
    MOVEQ R0, 0		@ use R0 as an intermediate register to hold zero
    STREQ R0, [R6]	@ store zero back into the mantissa address
    BEQ main		@ branch to the main program
    
    /* if the mantissa is above zero we have NaN */
    LDRGT R0, =fMax	    @ load fMax address into R0
    LDRGT R1, =0x7FFFFFFF   @ load NaN value into R1
    STRGT R1, [R0]	    @ store NaN value into fMax address
    BGT setf2Maxes	    @ branch to set rest of max variables to f2
    
    main:
    /* compare sign values */
    LDR R0, =sb1    @ load the address of sign bit 1 into R1
    LDR R0, [R0]    @ load the value of sign bit 1 into R0
    LDR R1, =sb2    @ load the address of sign bit 2 into R1
    LDR R1, [R1]    @ load the value of sign bit 2 into R1
    CMP R0, R1	    @ compare sign bit values
    BLO setf1Max    @ lowest unsigned compared sign value gets chosen as max
    BHI setf2Max
    
    /* compare exponent values */
    LDR R1, =exp1   @ load the address of exponent 1 into R1
    LDR R2, =exp2   @ load the address of exponent 2 into R2
    LDR R1, [R1]    @ load the value of exponent 1 into R1
    LDR R2, [R2]    @ load the value of exponent 2 into R2
    CMP R0, 1	    @ check if the signs are negative
    NEGEQ R1, R1    @ flip the exponents if the float is negative
    NEGEQ R2, R2
    CMP R1, R2	    @ compare the adjusted exponent values
    BGT setf1Max    @ highest signed compare exponent gets chosen as max
    BLT setf2Max

    /* compare mantissa values */
    LDR R0, =mant1  @ load the address of mantissa 1 into R0
    LDR R0, [R0]    @ load the value of mantissa 1 into R0
    LDR R1, =mant2  @ load the address of mantissa 2 into R1
    LDR R1, [R1]    @ load the value of mantissa 2 into R1
    CMP R0, R1	    @ compare the mantissa values
    BHI setf1Max    @ highest unsigned mantissa gets chosen as max
    BLO setf2Max
    
    setf1Max:
    LDR R0, =fMax   @ load the address of fMax into R0
    LDR R1, =f1	    @ load the address of f1 into R1
    LDR R1, [R1]    @ load the value of f1 into R1
    STR R1, [R0]    @ store the value of f1 into fMax
    B setf1Maxes    @ branch to set remaining max variables
    
    setf1Maxes:
    /* set remaining max variables to f1's values */
    LDR R1, =signBitMax
    LDR R2, =sb1
    LDR R2, [R2]
    STR R2, [R1]
    LDR R1, =biasedExpMax
    LDR R2, =biasedExp1
    LDR R2, [R2]
    STR R2, [R1]
    LDR R1, =expMax
    LDR R2, =exp1
    LDR R2, [R2]
    STR R2, [R1]
    LDR R1, =mantMax
    LDR R2, =mant1
    LDR R2, [R2]
    STR R2, [R1]
    B done	    @ branch to exit function
    
    setf2Max:
    LDR R0, =fMax   @ load the address of fMax into R0
    LDR R1, =f2	    @ load the address of f1 into R1
    LDR R1, [R1]    @ load the value of f1 into R1
    STR R1, [R0]    @ store the value of f1 into fMax
    B setf2Maxes    @ branch to set remaining max variables
    
    setf2Maxes:
    /* set remaining max variables to f2's values */
    LDR R1, =signBitMax
    LDR R2, =sb2
    LDR R2, [R2]
    STR R2, [R1]
    LDR R1, =biasedExpMax
    LDR R2, =biasedExp2
    LDR R2, [R2]
    STR R2, [R1]
    LDR R1, =expMax
    LDR R2, =exp2
    LDR R2, [R2]
    STR R2, [R1]
    LDR R1, =mantMax
    LDR R2, =mant2
    LDR R2, [R2]
    STR R2, [R1]
    B done	    @ branch to exit function
    
    done:
    LDR R0, =fMax   @ safety case to load fMax address into R0
    POP {R4-R11,LR} @ pop registers R4-R11 off the stack
    BX LR	    @ return to caller
    
    /* YOUR asmFmax CODE ABOVE THIS LINE! ^^^^^^^^^^^^^^^^^^^^^  */

   

/**********************************************************************/   
.end  /* The assembler will not process anything after this directive!!! */
           



    

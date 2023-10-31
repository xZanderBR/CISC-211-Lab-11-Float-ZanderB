/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.c

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include <stdio.h>
#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <string.h>
#include <malloc.h>
#include <inttypes.h>   // required to print out pointers using PRIXPTR macro
#include "definitions.h"                // SYS function prototypes
#include "testFuncs.h" // lab test structs
#include "printFuncs.h"  // lab print funcs


#define MAX_PRINT_LEN 1000

static uint8_t txBuffer[MAX_PRINT_LEN] = {0};


static char * pass = "PASS";
static char * fail = "FAIL";
static char * oops = "OOPS";

// externs defined in the assembly file:
extern float f1,f2,fMax;
extern uint32_t sb1,sb2,signBitMax;
// ERROR: exp2 is the name of a built-in C function!
// extern int32_t exp1,exp2,expMax; // adjusted UNBIASED exponent
extern int32_t expMax; // adjusted UNBIASED exponent
extern uint32_t mant1,mant2,mantMax; // adjusted mantissa (hidden bit added when appropriate))
extern int32_t biasedExp1,biasedExp2,biasedExpMax;
extern uint32_t nanValue;



/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

/* ************************************************************************** */
/** Descriptive Data Item Name

  @Summary
    Brief one-line summary of the data item.
    
  @Description
    Full description, explaining the purpose and usage of data item.
    <p>
    Additional description in consecutive paragraphs separated by HTML 
    paragraph breaks, as necessary.
    <p>
    Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.
    
  @Remarks
    Any additional remarks
 */

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

/* ************************************************************************** */

/** 
  @Function
    int ExampleLocalFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Description
    Full description, explaining the purpose and usage of the function.
    <p>
    Additional description in consecutive paragraphs separated by HTML 
    paragraph breaks, as necessary.
    <p>
    Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.

  @Precondition
    List and describe any required preconditions. If there are no preconditions,
    enter "None."

  @Parameters
    @param param1 Describe the first parameter to the function.
    
    @param param2 Describe the second parameter to the function.

  @Returns
    List (if feasible) and describe the return values of the function.
    <ul>
      <li>1   Indicates an error occurred
      <li>0   Indicates an error did not occur
    </ul>

  @Remarks
    Describe any special behavior not described above.
    <p>
    Any additional remarks.

  @Example
    @code
    if(ExampleFunctionName(1, 2) == 0)
    {
        return 3;
    }
 */

// have to play games with data types to get floats to be passed in r0 and r1
// otherwise, assy needs to use VMOV instructions to move from s registers
// to r registers
static uint32_t reinterpret_float(float f)
{
    float *pf = &f;
    void * pv = (void *) pf;
    uint32_t * pi = (uint32_t *) pv;
    return *pi;
}


// returns true for NaN
static bool isNan(float inpVal) 
{
    uint32_t iFloat;
    // reinterpret the float as 32b int so we can use bitwise operators
    iFloat = reinterpret_float(inpVal);
    // check to see if exp bits are all 1, and mantissa is non-zero
    if((iFloat&PLUS_INF) == PLUS_INF &&
       (iFloat&NAN_MASK) != 0) {
        return true;
    }
    return false;
}

// returns +1 for +inf, -1 for -inf
static int isInf(float inpVal) 
{
    uint32_t iFloat;
    // reinterpret the float as 32b int so we can use bitwise operators
    iFloat = reinterpret_float(inpVal);
    if(iFloat == PLUS_INF) {
        return 1;
    }
    else if (iFloat == NEG_INF) {
        return -1;
    }
    return 0;
}


static void check(int32_t in1, 
        int32_t in2, 
        int32_t *goodCount, 
        int32_t *badCount,
        char **pfString )
{
    if (in1 == in2)
    {
        *goodCount += 1;
        *pfString = pass;
    }
    else
    {
        *badCount += 1;
        *pfString = fail;        
    }
    return;
}

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

// *****************************************************************************

/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */

void calcExpectedValues(
        float input, // test number
        expectedValues *e)  // ptr to struct where values will be stored
{
    // TODO!!!!!!!!!!
    if (true == isNan(input)) // if NaN...
    {
        e->floatVal = NAN;
        e->intVal = reinterpret_float(e->floatVal);
        e->biasedExp = 0xFF;
        e->unbiasedExp = 0xFF-127;
        e->signBit = e->intVal >> 31;
        e->mantissa = 0x7FFFFF; // It can be any non-0 value, we'll use all 1's.
    }
    else if (true == isInf(input))   // if +/- inf
    {
        e->floatVal = input;
        e->intVal = reinterpret_float(e->floatVal);
        e->biasedExp = 0xFF;
        e->unbiasedExp = 0xFF-127;
        e->signBit = e->intVal >> 31; // could be +/- inf
        e->mantissa = 0x0;      // for +/- inf, this must be zero 
    }
    else // either subnormal or normal float
    {
        e->floatVal = input;
        e->intVal = reinterpret_float(e->floatVal);
        e->signBit = e->intVal >> 31; // could be +/- inf
        e->biasedExp = e->intVal >>23;
        e->unbiasedExp = e->biasedExp - 127;
        e->mantissa = e->intVal & 0x7FFFFF; // mask off LSB 23 bits
        if(e->unbiasedExp == -127) // then this is a subnormal number
        {
            // adjust to -126, and skip adding hidden bit to mantissa
            e->unbiasedExp = -126;
        }
        else // not subnormal; just a run-of-the-mill float...
        {
            // add hidden bit to mantissa
            e->mantissa = e->mantissa | 0x800000; // add  missing bit    
        }
    }
        
    return;
}



void testResult(int testNum, 
                      float testVal1, // val passed to asm in r0
                      float testVal2, // val passed to asm in r1
                      float*pResult, // pointer to max chosen by asm code
                      float *pGood, //ptr to correct location
                      int32_t* passCnt,
                      int32_t* failCnt,
                      volatile bool * txComplete)
{
    float asmResult = *pResult;
    int asmInf = isInf(asmResult);
    bool asmNan = isNan(asmResult);
    int inp1Inf = isInf(testVal1);
    bool inp1Nan = isNan(testVal1);
    int inp2Inf = isInf(testVal2);
    bool inp2Nan = isNan(testVal2);
    expectedValues e;

    char * ptrCheck = oops;

    char *s1 = pass;
    *passCnt = 0;
    *failCnt = 0;
    
    check((int32_t)(&fMax),(int32_t)pResult,passCnt,failCnt,&ptrCheck);
    
    //int32_t  goodExponent = ((iGoodMax >>23) & 0xFF) - 127;
    //if goodExponent == -127
    //uint32_t goodMantissa = iGoodMax & 0x7FFFFF;

    
    if (inp1Nan || inp2Nan) // if either or both test inputs were NaN ...
    {
        calcExpectedValues(NAN, &e);
        if (true != asmNan) // if asm value was not NAN
        {
            s1 = fail;
            *failCnt += 1;
        }
        else
        {
            *passCnt += 1;
        }
    }
    else if ((inp1Inf ==1) || (inp2Inf == 1)) // if either input was +inf
    {
        calcExpectedValues(INFINITY, &e);
        if (true != asmInf) // fail if max was not set to +inf
        {
            s1 = fail;
            *failCnt += 1;
        }
        else
        {
            *passCnt += 1;
        }
    }
    else if (inp1Inf == -1) // if f1 was -inf, then assume f2 was max
    {
        calcExpectedValues(testVal2, &e);
        if (testVal2 != asmResult) 
        {
            s1 = fail;
            *failCnt += 1;
        }
        else
        {
            *passCnt += 1;
        }
    }
    else if (inp2Inf == -1)  // else if f2 was -inf, then assume f1 was max
    {
        calcExpectedValues(testVal1, &e);
        if (testVal1 != asmResult) 
        {
            s1 = fail;
            *failCnt += 1;
        }
        else
        {
            *passCnt += 1;
        }
    }    
    else if(testVal1<testVal2) // else, check to see which was larger
    {
        calcExpectedValues(testVal2, &e);
        if (testVal2 != asmResult) 
        {
            s1 = fail;
            *failCnt += 1;
        }
        else
        {
            *passCnt += 1;
        }
    }
    else // either testVal1 is greater, or they are equal
    {
        calcExpectedValues(testVal1, &e);
        if (testVal1 != asmResult) 
        {
            s1 = fail;
            *failCnt += 1;
        }
        else
        {
            *passCnt += 1;
        }
    }
       
    snprintf((char*)txBuffer, MAX_PRINT_LEN,
            "========= Test Number: %d\r\n"
            "inp1: %.4e; inp2: %.4e \r\n"
            "expected: %.4e; your result: %.4e; %s \r\n"
            "pointer check: %s\r\n"
            "tests passed: %ld; tests failed: %ld \r\n"
            "\r\n",
            testNum,
            testVal1,testVal2,
            e.floatVal,asmResult,s1,
            ptrCheck,
            *passCnt,*failCnt); 
    printAndWait((char*)txBuffer,txComplete);
    
    return;
}




/* *****************************************************************************
 End of File
 */

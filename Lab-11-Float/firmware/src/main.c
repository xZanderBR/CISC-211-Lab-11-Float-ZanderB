/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project. It is intended to
    be used as the starting point for CISC-211 Curiosity Nano Board
    programming projects. After initializing the hardware, it will
    go into a 0.5s loop that calls an assembly function specified in a separate
    .s file. It will print the iteration number and the result of the assembly 
    function call to the serial port.
    As an added bonus, it will toggle the LED on each iteration
    to provide feedback that the code is actually running.
  
    NOTE: PC serial port should be set to 115200 rate.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

//DOM-IGNORE-BEGIN 
/*******************************************************************************
* Copyright (C) 2020 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
//DOM-IGNORE-END 

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdio.h>
#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <string.h>
#include <float.h>
#include "definitions.h"                // SYS function prototypes
#include "printFuncs.h"  // lab print funcs

/* RTC Time period match values for input clock of 1 KHz */
#define PERIOD_50MS                             51
#define PERIOD_500MS                            512
#define PERIOD_1S                               1024
#define PERIOD_2S                               2048
#define PERIOD_4S                               4096

#define MAX_PRINT_LEN 1000

#define PLUS_INF ((0x7F800000))
#define NEG_INF  ((0xFF800000))
#define NAN_MASK  (~NEG_INF)

#ifndef NAN
#define NAN ((0.0/0.0))
#endif

#ifndef INFINITY
#define INFINITY ((1.0f/0.0f))
#define NEG_INFINITY ((-1.0f/0.0f))
#endif


static volatile bool isRTCExpired = false;
static volatile bool changeTempSamplingRate = false;
static volatile bool isUSARTTxComplete = true;
static uint8_t uartTxBuffer[MAX_PRINT_LEN] = {0};

static char * pass = "pass";
static char * fail = "FAIL";

// PROF COMMENT:
// The ARM calling convention permits the use of up to 4 registers, r0-r3
// to pass data into a function. Only one value can be returned from a C
// function call. It will be stored in r0, which will be automatically
// used by the C compiler as the function's return value.
//
// Function signature
// For this lab, return the larger of the two floating point values passed in.
extern float * asmFmax(uint32_t, uint32_t);

// externs defined in the assembly file:
extern float f1,f2,fMax;
extern uint32_t sb1,sb2,signBitMax;
// ERROR: exp2 is the name of a built-in C function!
// extern int32_t exp1,exp2,expMax; // adjusted UNBIASED exponent
extern int32_t expMax; // adjusted UNBIASED exponent
extern uint32_t mant1,mant2,mantMax; // adjusted mantissa (hidden bit added when appropriate))
extern int32_t biasedExp1,biasedExp2,biasedExpMax;
extern uint32_t nanValue;


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

static float tc[][2] = {
    {     0.1,                  0.99},  // 
    {     1.14437421182e-28,   785.066650391},  //
    { -4000.1,                   0.0,},  // 
    {    -1.9e-5,               -1.9e-5},  // 
    {     1.347e10,              2.867e-10},  // 
    {     1.0,                   0.0}
};
#define USING_HW 1

#if USING_HW
static void rtcEventHandler (RTC_TIMER32_INT_MASK intCause, uintptr_t context)
{
    if (intCause & RTC_MODE0_INTENSET_CMP0_Msk)
    {            
        isRTCExpired    = true;
    }
}
static void usartDmaChannelHandler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle)
{
    if (event == DMAC_TRANSFER_EVENT_COMPLETE)
    {
        isUSARTTxComplete = true;
    }
}
#endif

bool isNan(float inpVal) 
{
    uint32_t iFloat;
    iFloat = reinterpret_float(inpVal);
    if((iFloat&PLUS_INF) == PLUS_INF &&
       (iFloat&NAN_MASK) != 0) {
        return true;
    }
    return false;
}

int isInf(float inpVal) 
{
    uint32_t iFloat;
    iFloat = reinterpret_float(inpVal);
    if(iFloat == PLUS_INF) {
        return 1;
    }
    else if (iFloat == NEG_INF) {
        return -1;
    }
    return 0;
}

#if 0
static void testResult(int testNum, 
                      float testVal1, 
                      float testVal2, 
                      float*pResult,// pointer to max chosen by asm code
                      int32_t* passCnt,
                      int32_t* failCnt,
                      volatile bool * txComplete)
{ return; }
#endif

#if 1
// return failure count. A return value of 0 means everything passed.
static void testResult(int testNum, 
                      float testVal1, 
                      float testVal2, 
                      float*pResult,// pointer to max chosen by asm code
                      int32_t* passCnt,
                      int32_t* failCnt,
                      volatile bool * txComplete)
{
    float goodMax = 0.0;
    float asmResult = *pResult;
    int asmInf = isInf(asmResult);
    bool asmNan = isNan(asmResult);
    int inp1Inf = isInf(testVal1);
    bool inp1Nan = isNan(testVal1);
    int inp2Inf = isInf(testVal2);
    bool inp2Nan = isNan(testVal2);

    char *s1 = pass;
    *passCnt = 0;
    *failCnt = 0;
    
    if (inp1Nan || inp2Nan) // if either or both test inputs were NaN ...
    {
        goodMax = NAN;
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
        goodMax = INFINITY;
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
        goodMax = testVal2;
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
        goodMax = testVal1;
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
        goodMax = testVal2;
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
    else 
    {
        goodMax = testVal1;
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
       
    snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
            "========= Test Number: %d\r\n"
            "inp1: %.4e; inp2: %.4e \r\n"
            "expected: %.4e; your result: %.4e; %s \r\n"
            "tests passed: %ld; tests failed: %ld \r\n"
            "\r\n",
            testNum,
            testVal1,testVal2,
            goodMax,asmResult,s1,
            *passCnt,*failCnt); 
    printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);
    
    return;
}
#endif

#if 0
// return failure count. A return value of 0 means everything passed.
static int testResult(int testNum, 
        float f1, 
        float f2, 
        float*pResult)
{
    int failCount = 0;
    float goodMax = f1;
    if(f1<f2)
    {
        goodMax = f2;
    }
    uint32_t iGoodMax = reinterpret_float(goodMax);
    uint32_t testMax = reinterpret_float(*pResult);
    
    char *s1 = pass;
    char *s2 = pass;
    char *s3 = pass;
    char *s4 = pass;
    
    uint32_t goodSignBit = (iGoodMax >>31) & 0x1;
    int32_t  goodExponent = ((iGoodMax >>23) & 0xFF) - 127;
    if goodExponent == -127
    uint32_t goodMantissa = iGoodMax & 0x7FFFFF;

    if (iGoodMax != testMax)
    {
        s1 = fail;
        ++failCount;        
    }
    if (goodSignBit != signBitMax)
        s2 = fail;
    if (goodExponent != expMax)
        s3 = fail;
    if (goodMantissa != mantMax)
        s4 = fail;
   
    snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
            "========= Test Number: %d\r\n"
            "f1: %.4e; f2: %.4e\r\n"
            "expected max: %.4e; your max: %.4e; %s\r\n"
            "word32 expected: 0x%08lx; your result: 0x%08lx\r\n"
            "sign bit expected: %10lu; your result: %10lu; %s\r\n"
            "exponent expected: %10ld; your result: %10ld; %s\r\n"
            "mantissa expected: 0x%08lx; your result: 0x%08lx; %s\r\n"
            "\r\n",
            testNum,
            f1,f2,goodMax,*pResult,s1,
            iGoodMax,testMax,
            goodSignBit, signBitMax, s2,
            goodExponent, expMax, s3,
            goodMantissa, mantMax, s4
            ); 
     return failCount;
    
}
#endif

// extern char * someString;



// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************
int main ( void )
{
    
 
#if USING_HW
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    DMAC_ChannelCallbackRegister(DMAC_CHANNEL_0, usartDmaChannelHandler, 0);
    RTC_Timer32CallbackRegister(rtcEventHandler, 0);
    RTC_Timer32Compare0Set(PERIOD_50MS);
    RTC_Timer32CounterSet(0);
    RTC_Timer32Start();

#else // using the simulator
    isRTCExpired = true;
    isUSARTTxComplete = true;
#endif //SIMULATOR

    int32_t passCount = 0;
    int32_t failCount = 0;
    int32_t totalPassCount = 0;
    int32_t totalFailCount = 0;
    int32_t totalTestCount = 0;
    int iteration = 0;   
    int maxIterations = sizeof(tc)/sizeof(tc[0]);
    
    while ( true )
    {
        if (isRTCExpired == true)
        {
            isRTCExpired = false;
            
            LED0_Toggle();
            
            uint32_t ff1 = reinterpret_float(tc[iteration][0]);
            uint32_t ff2 = reinterpret_float(tc[iteration][1]);
            
            // Place to store the result of the call to the assy function
            float *max;
            
            // Make the call to the assembly function
            max = asmFmax(ff1,ff2);
            
            testResult(iteration,tc[iteration][0],tc[iteration][1],
                    max,
                    &passCount,
                    &failCount,
                    &isUSARTTxComplete);
            totalPassCount += passCount;        
            totalFailCount += failCount;        
            totalTestCount += failCount + passCount;        
             ++iteration;
            
            if (iteration >= maxIterations)
            {
                break; // tally the results and end program
            }
            
        }
        /* Maintain state machines of all polled MPLAB Harmony modules. */
    }

#if USING_HW
    snprintf((char*)uartTxBuffer, MAX_PRINT_LEN,
            "========= ALL TESTS COMPLETE!\r\n"
            "tests passed: %ld \r\n"
            "tests failed: %ld \r\n"
            "total tests:  %ld \r\n"
            "score: %ld/20 points \r\n\r\n",
            totalPassCount,
            totalFailCount,
            totalTestCount,
            20*totalPassCount/totalTestCount); 
            isUSARTTxComplete = false;
#if 0            
    DMAC_ChannelTransfer(DMAC_CHANNEL_0, uartTxBuffer, \
                    (const void *)&(SERCOM5_REGS->USART_INT.SERCOM_DATA), \
                    strlen((const char*)uartTxBuffer));
#endif
    
    printAndWait((char*)uartTxBuffer,&isUSARTTxComplete);

#else
            isRTCExpired = true;
            isUSARTTxComplete = true;
            if (iteration >= maxIterations)
            {
                break; // end program
            }

            continue;
#endif

    
    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}
/*******************************************************************************
 End of File
*/


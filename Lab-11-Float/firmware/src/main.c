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
#include "testFuncs.h"  // lab print funcs

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

// static char * pass = "PASS";
// static char * fail = "FAIL";
// static char * oops = "OOPS";

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
    {     1.0,                  2.0}, 
    {    -3.1,                  -1.2}, 
    {     NAN,                  1.0}, 
    {    -1.0,                  NAN}, 
    {     0.1,                  0.99},  // 
    {     1.14437421182e-28,   785.066650391},  //
    { -4000.1,                   0.0,},  // 
    {    -1.9e-5,               -1.9e-5},  // 
    {     1.347e10,              2.867e-10},  // 
    {     1.4e-42,              -3.2e-43}, // subnormals
    {     -2.4e-42,              2.313e29}, // subnormals
    {    INFINITY,           NEG_INFINITY},
    {    NEG_INFINITY,           -6.24},
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
                    &fMax,
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


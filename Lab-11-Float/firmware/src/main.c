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

/* RTC Time period match values for input clock of 1 KHz */
#define PERIOD_500MS                            512
#define PERIOD_1S                               1024
#define PERIOD_2S                               2048
#define PERIOD_4S                               4096

#define MAX_PRINT_LEN 400

static volatile bool isRTCExpired = false;
static volatile bool changeTempSamplingRate = false;
static volatile bool isUSARTTxComplete = true;
static uint8_t uartTxBuffer[MAX_PRINT_LEN] = {0};
static char * pass = "pass";
static char * fail = "fail";

// VB COMMENT:
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
extern uint32_t signBitMax;
extern int32_t expMax; // UNBIASED exponent
extern uint32_t mantMax;
extern int32_t biasedExp1;
extern uint32_t nanValue;

static uint32_t reinterpret_float(float f)
{
    float *pf = &f;
    void * pv = (void *) pf;
    uint32_t * pi = (uint32_t *) pv;
    return *pi;
}

static float af1[] = {  0.1, 1.14437421182e-28, -4000.1, -1.9e-5, 1.347e10,
                        0.0/0.0};
static float af2[] = {  0.99, 785.066650391,     0.0, -1.9e-5, 2.867e-10,
                        1.0/0.0};

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

// return failure count. A return value of 0 means everything passed.
static int testResult(int testNum, float f1, float f2, float*pResult)
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
            "f1: %.4e; f2: %.4e; expected: %.4e; your result: %.4e; %s\r\n"
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
    RTC_Timer32Start();
#else // using the simulator
    isRTCExpired = true;
    isUSARTTxComplete = true;
#endif //SIMULATOR

    int iteration = 0;   
    int maxIterations = sizeof(af1)/sizeof(float);
    
    while ( true )
    {
        if ((isRTCExpired == true) && (true == isUSARTTxComplete))
        {
            isRTCExpired = false;
            isUSARTTxComplete = false;
            
            LED0_Toggle();
            
            uint32_t ff1 = reinterpret_float(af1[iteration]);
            uint32_t ff2 = reinterpret_float(af2[iteration]);
            
            // Place to store the result of the call to the assy function
            float *max;
            
            // Make the call to the assembly function
            max = asmFmax(ff1,ff2);
            
            testResult(iteration,af1[iteration],af2[iteration],max);
                    
             ++iteration;
#if USING_HW
            DMAC_ChannelTransfer(DMAC_CHANNEL_0, uartTxBuffer, \
                    (const void *)&(SERCOM5_REGS->USART_INT.SERCOM_DATA), \
                    strlen((const char*)uartTxBuffer));

            if (iteration >= maxIterations)
            {
                while  (false == isUSARTTxComplete );
                break; // end program
            }
#else
            isRTCExpired = true;
            isUSARTTxComplete = true;
            if (iteration >= maxIterations)
            {
                break; // end program
            }

            continue;
#endif
            
            
            
            
            
        }
        /* Maintain state machines of all polled MPLAB Harmony modules. */
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}
/*******************************************************************************
 End of File
*/


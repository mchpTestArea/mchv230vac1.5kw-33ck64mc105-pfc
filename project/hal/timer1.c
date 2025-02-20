// <editor-fold defaultstate="collapsed" desc="Description/Instruction ">
/**
 * timer1.c
 *
 * This file includes subroutine to configure TIMER1 Module
 * 
 * Definitions in this file are for dsPIC33CK64MC105.
 * 
 * Component: Master Core - HAL - TIMER1
 * 
 */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Disclaimer ">

/*******************************************************************************
* SOFTWARE LICENSE AGREEMENT
* 
* © [2024] Microchip Technology Inc. and its subsidiaries
* 
* Subject to your compliance with these terms, you may use this Microchip 
* software and any derivatives exclusively with Microchip products. 
* You are responsible for complying with third party license terms applicable to
* your use of third party software (including open source software) that may 
* accompany this Microchip software.
* 
* Redistribution of this Microchip software in source or binary form is allowed 
* and must include the above terms of use and the following disclaimer with the
* distribution and accompanying materials.
* 
* SOFTWARE IS "AS IS." NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY,
* APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,
* MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL 
* MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR 
* CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO
* THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE 
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY
* LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL
* NOT EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR THIS
* SOFTWARE
*
* You agree that you are solely responsible for testing the code and
* determining its suitability.  Microchip has no obligation to modify, test,
* certify, or support the code.
*
*******************************************************************************/
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="HEADER FILES ">

#ifdef __XC16__  // See comments at the top of this header file
    #include <xc.h>
#endif // __XC16__

#include <stdint.h>
#include <stdbool.h>
#include "timer1.h"

// </editor-fold> 

// <editor-fold defaultstate="collapsed" desc="INTERFACE FUNCTIONS ">
/**
 * <B> Function: TIMER1_Initialize()  </B>
 * @brief Function to configure TIMER1 Module
 * @param None.
 * @return None.
 * @example
 * <code>
 * status = TIMER1_Initialize();
 * </code>
 */ 
void TIMER1_Initialize(void)
{
    /** Initialize Timer1 Control Register
        bit 15:8    TON-U-SIDL-TMWDIS TMWIP-PRWIP-TECS<1:0> 
        bit 7:0     TGATE-U-TCKPS<1:0> U-TSYNC-TCS-U                          */
    T1CON = 0;
    
    /** Timer1 On bit  1 = Starts 16bit Timer1, 0 = Stops 16bit Timer1. */
    T1CONbits.TON = 0;
    /** Timer1 Stop in Idle Mode bit
        1 = Discontinues module operation when device enters Idle mode
        0 = Continues module operation in Idle mode */
    T1CONbits.TSIDL = 0;
    
    /** TGATE: Timer1 Gated Time Accumulation Enable bit
        When TCS = 1: This bit is ignored.
        When TCS = 0:
        1 = Gated time accumulation is enabled
        0 = Gated time accumulation is disabled  */
    T1CONbits.TGATE = 0;
    
    /** TCKPS<1:0>: Timer1 Input Clock Prescale Select bits
        0b11 = 1:256 , 0b10 = 1:64 ,0b01 = 1:8 0b00 = 1:1                     */
    T1CONbits.TCKPS = 0; 
    
    /** Timer1 External Clock Input Synchronization Select bit(1)
        When TCS = 1:
        1 = Synchronizes the External Clock input
        0 = Does not synchronize the External Clock input
        When TCS = 0: This bit is ignored.                                    */
    T1CONbits.TCS = 0; 

    /** Timer1 Clock Source Select 
        1 = External Clock source selected by TECS<1:0>
        0 = Internal peripheral clock (FP) */
    T1CONbits.TSYNC = 0;

    PR1  = 0; 
    TMR1 = 0;

    TIMER1_InterruptDisable();
    TIMER1_InterruptFlagClear();
}
// </editor-fold>
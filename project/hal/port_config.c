// <editor-fold defaultstate="collapsed" desc="Description/Instruction ">
/**
 * @file port_config.c
 *
 * @brief This module initializes the GPIO pins as analog/digital,input or 
 * output etc. 
 * 
 * Definitions in the file are for dsPIC33CK64MC105 MC DIM plugged onto 
 * Motor Control Development board from Microchip
 *
 * Component: PORTS
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
#include <xc.h>
#include "port_config.h"

// </editor-fold> 

// <editor-fold defaultstate="collapsed" desc="INTERFACE FUNCTIONS ">
/**
 * <B> Function: SetupGPIOPorts()  </B>
 * @brief Function initializes GPIO pins for input or output ports,
 *        analog/digital pins,remap the peripheral functions to desires RPx pins.
 * @param None.
 * @return None.
 * @example
 * <code>
 * status = SetupGPIOPorts();
 * </code>
 */

void SetupGPIOPorts(void)
{
    // Reset all PORTx register (all inputs)
    #ifdef TRISA
        TRISA = 0xFFFF;
        LATA  = 0x0000;
    #endif
    #ifdef ANSELA
        ANSELA = 0x0000;
    #endif

    #ifdef TRISB
        TRISB = 0xFFFF;
        LATB  = 0x0000;
    #endif
    #ifdef ANSELB
        ANSELB = 0x0000;
    #endif

    #ifdef TRISC
        TRISC = 0xFFFF;
        LATC  = 0x0000;
    #endif
    #ifdef ANSELC
        ANSELC = 0x0000;
    #endif

    #ifdef TRISD
        TRISD = 0xFFFF;
        LATD  = 0x0000;
    #endif
    #ifdef ANSELD
        ANSELD = 0x0000;
    #endif

    #ifdef TRISE
        TRISE = 0xFFFF;
        LATE  = 0x0000;
    #endif
    #ifdef ANSELE
        ANSELE = 0x0000;
    #endif

    MapGPIOHWFunction();

    return;
}

/**
 * <B> Function: Map_GPIO_HW_Function()  </B>
 * @brief Routine to setup GPIO pin used as input/output analog/digital etc
 * @param None.
 * @return None.
 * @example
 * <code>
 * status = MapGPIOHWFunction();
 * </code>
 */

void MapGPIOHWFunction(void)
{
    
    /* ANALOG SIGNALS */
    //Vdc
    ANSELBbits.ANSELB8  = 1;
    TRISBbits.TRISB8    = 1;   //PIN36: PGD1/AN10/RP40/SCL1/RB8
    
    //Vac
    ANSELCbits.ANSELC0  = 1;
    TRISCbits.TRISC0    = 1;   //PIN7 : AN12/RP48/RC0
    
    //IL
    ANSELCbits.ANSELC3  = 1;
    TRISCbits.TRISC3    = 1;   //PIN20: AN15/IBIAS2/RP51/RC3

    //PFC Enable pin
    TRISCbits.TRISC11   = 0;  //PIN41: RP59/RC11
    
    // Debug LEDs
    // LED2
    TRISCbits.TRISC7    = 0;   // PIN:24 - ISRC2/RP55/RC7
    // LED1
    TRISDbits.TRISD10   = 0;   // PIN:23 - ISRC3/RP74/RD10

    // Push button Switches            
    // Push Button
    TRISCbits.TRISC10   = 1;     // PIN40: RP58/RC10   
    
	/** Re-map UART Channels to the device pins connected to the following 
        PIM pins on the Motor Control Development Boards.
        UART_RX : RP61/RC13 (Input)
        UART_TX : RP60/RC12 (Output)   */
    _U1RXR = 61;
    _RP60R = 0b000001;
}
// </editor-fold> 

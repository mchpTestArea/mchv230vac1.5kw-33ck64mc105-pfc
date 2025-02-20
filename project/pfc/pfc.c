// <editor-fold defaultstate="collapsed" desc="Description/Instruction ">
/**
 * @file pfc.c
 *
 * @brief This module has functions required for Power Factor Correction Control
 */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Disclaimer ">

/*******************************************************************************
* SOFTWARE LICENSE AGREEMENT
* 
* � [2024] Microchip Technology Inc. and its subsidiaries
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
#include "libq.h"
#include "pfc.h"
#include "board_service.h"

// </editor-fold> 

// <editor-fold defaultstate="collapsed" desc="Function Declarations ">

static int16_t PFC_SignalRectification(PFC_MEASURE_VOLTAGE_T *);
static int16_t PFC_CurrentSampleCorrection(PFC_T *);
static void PFC_Average(PFC_AVG_T *,int16_t);
static void PFC_SquaredRMSCalculate(PFC_RMS_SQUARE_T *,int16_t);

inline static void PFC_CurrentRefGenerate(PFC_T *);
inline static void PFC_CurrentControlLoop(PFC_T *);

static void PFC_ParamsInit(PFC_T *);
static void PFC_ResetParams(PFC_T *);
static void PFC_FaultCheck(PFC_T *);
void PFC_StateMachine(PFC_T *);

// </editor-fold> 

// <editor-fold defaultstate="collapsed" desc="Global Variables  ">

PFC_T pfcParam;

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="INTERFACE FUNCTIONS ">
/**
* <B> Function: PFC_ADCInterrupt()  </B>
*
* @brief ADC interrupt vector ,and it performs following actions:
*        (1) Reads DC BUS voltage,input AC voltage and inductor current feedback 
*            from ADC data buffers.
*        (2) Executes Power Factor Correction State machine
*        (3) Loads duty cycle value generated by PFC current control loop to 
*            PWM Duty Register
*/
void __attribute__((__interrupt__,no_auto_psv)) PFC_ADCInterrupt()
{    
    /** Load ADC Buffer data to respective variables */
    pfcParam.pfcVoltage.vdc  = ADCBUF_VDC;
    pfcParam.pfcVoltage.vac  = ADCBUF_PFC_VAC;
    pfcParam.pfcCurrent.iL   = ADCBUF_PFC_IL;     
            
    PFC_StateMachine(&pfcParam);

#ifdef DEBUG_BOOST
    PFC_ENABLE_SIGNAL = 1;
    pfcParam.duty  = DEBUG_PFC_DUTY;
#endif
    
    PFC_PWM_PDC = pfcParam.duty;    
    LED1 = 0;
    ClearPFCADCIF();
}
/**
 * <B> Function: PFC_StateMachine(PFC_T *pfcData)  </B>
 * @brief Function to perform  - VAC RMS calculation, calculate moving average 
 *        of Vdc, PFC voltage control loop,  generate current reference, PFC  
 *        current control loop.
 * @param none.
 * @return none.
 * @example
 * <code>
 * status = PFC_StateMachine();
 * </code>
 */
void PFC_StateMachine(PFC_T *pfcData)
{    
    uint16_t pfcState = pfcData->state;
    PFC_MEASURE_CURRENT_T *pCurrent = &pfcData->pfcCurrent;
    PFC_MEASURE_VOLTAGE_T *pVoltage = &pfcData->pfcVoltage;
    
    /** Calculate average of PFC output voltage (DC voltage) feedback 
    to remove line frequency ripple */
    PFC_Average(&pfcData->vdcAVG,pVoltage->vdc);
    
    /** Calculate average of input AC voltage feedback for offset correction */
    PFC_Average(&pfcData->vacAVG,pVoltage->vac);
    pVoltage->offsetVac = pfcData->vacAVG.output;

    /** Function to rectify the input AC voltage */
    pfcData->rectifiedVac = PFC_SignalRectification(pVoltage);

    /** Calculate RMS Square of rectified input voltage  */
    PFC_SquaredRMSCalculate(&pfcData->vacRMS,pfcData->rectifiedVac);
    
    switch(pfcState)
    {
        case PFC_INIT:
            
            PFC_ResetParams(pfcData);
            HAL_PFCPWMDisableOutputs();
            PFC_MeasureCurrentInit(pCurrent);
              
            pfcState = PFC_OFFSET_MEAS;
            
        break;
        case PFC_OFFSET_MEAS:
            PFC_MeasureCurrentOffset(pCurrent);
            
            if(pCurrent->status == 1)
            {
                if(pfcData->vacAVG.status == 1)
                {
                    /** Upon first entry into this loop, set PFC output voltage 
                        reference as measures DC voltage and enable  soft start */
                    pfcData->piVoltage.reference = pfcData->vdcAVG.output;
                    pVoltage->offsetVac = pfcData->vacAVG.output;
                    pfcState = PFC_WAIT_1CYCLE;
                }
            }
        break;
        case PFC_WAIT_1CYCLE:

            if(pfcData->vacRMS.status == 1)
            {  
                HAL_PFCPWMEnableOutputs();
                pfcState = PFC_CTRL_RUN;
            }
            
        break;
        case PFC_CTRL_RUN:
            
#ifdef ENABLE_PFC_CURRENT_OFFSET_CORRECTION
            pfcData->iL = pCurrent->iL - pCurrent->offset;
#endif
            
            PFC_FaultCheck(pfcData);

            if(pfcData->faultStatus == PFC_FAULT_NONE)
            {
                /** Perform soft start when enabled */
                if (pfcData->piVoltage.reference < PFC_OUPUT_VOLTAGE_REFERENCE)
                {
                    if(pfcData->rampRate == 0)
                    {
                        pfcData->piVoltage.reference = pfcData->piVoltage.reference + RAMP_COUNT;
                        pfcData->rampRate = RAMP_RATE;
                    }
                    else
                    {
                        pfcData->rampRate--;
                    }
                }
                else
                {
                    pfcData->piVoltage.reference = PFC_OUPUT_VOLTAGE_REFERENCE; 
                }

                PFC_CurrentRefGenerate(pfcData);                

                if(pVoltage->vdc > 0)
                {
                    /** Calculate the ideal value of boost converter duty ratio 
                        based on current value of Vdc and Vac. 
                        Boost Duty Ratio = (1 - (Vac/Vdc))
                                         = ((Vdc-Vac)/Vdc) */
                    pfcData->boostDutyRatio = __builtin_divf(pVoltage->vdc - 
                                pfcData->rectifiedVac, pVoltage->vdc) ;
                }

                PFC_CurrentControlLoop(pfcData);
                
                if(pfcData->piVoltage.output < PFC_MIN_CURRENTREF_PEAK_Q15)
                {
                    pfcData->duty = 0;
                    pfcData->piCurrent.integralOut = 0;
                }
            }
            else
            {
                pfcState = PFC_FAULT; 
            }
            break;
        case PFC_FAULT:
            pfcData->duty = 0;
            HAL_PFCPWMDisableOutputs();
            
            if(pfcData->vacRMS.sqrOutput >= PFC_INPUT_UNDER_VOLTAGE_LIMIT_HI)
            {
                pfcData->faultStatus &= (~PFC_FAULT_IP_UV);    
            }
            if(pfcData->vacRMS.sqrOutput < PFC_INPUT_OVER_VOLTAGE_LIMIT_LO )
            {
                pfcData->faultStatus &= (~PFC_FAULT_IP_OV);
            }
            if(pfcData->faultStatus == PFC_FAULT_NONE)
            {
                pfcData->piVoltage.integralOut = 0;
                pfcData->piCurrent.integralOut = 0;
                pfcData->piVoltage.reference = pfcData->vdcAVG.output;
                pfcState = PFC_CTRL_RUN;
                HAL_PFCPWMEnableOutputs();
            }
        break;
        default:
        break;   
    }
    pfcData->state = pfcState;
}
/**
* <B> Function: PFC_ServiceInit()     </B>
* 
* @brief Function calls the PFC_ParamsInit() which initializes the control 
*       parameters.      
* @param none.
* @return none.
* @example
* <CODE> PFC_ServiceInit();        </CODE>
*
*/

void PFC_ServiceInit(void)
{
    /* Make sure ADC does not generate interrupt while initializing parameters*/
	DisablePFCADCInterrupt();
    
    PFC_ParamsInit(&pfcParam);
    
    /* Enable ADC interrupt and begin main loop timing */
    ClearPFCADCIF();
    ClearPFCADCIF_ReadADCBUF();
    EnablePFCADCInterrupt(); 
}
/**
 * <B> Function: PFC_ParamsInit(PFC_T *pfcData)  </B>
 * 
 * @brief Function to initialize PFC related variables 
 * @param Pointer to the data structure containing PFC related variables.
 * PI coefficients, scaling constants etc
 * @return none.
 * @example
 * <code>
 * PFC_ParamsInit(PFC_T *pfcData)
 * </code>
 */
void PFC_ParamsInit(PFC_T *pfcData)
{  
    /** Initialize variables related to RMS calculation - VAC */      
    pfcData->vacRMS.sampleLimit = PFC_RMS_SQUARE_COUNTMAX;
    /** Initialize variables related to Average calculation - VDC */ 
    pfcData->vdcAVG.scaler = PFC_AVG_SCALER;
    pfcData->vdcAVG.sampleLimit = 1<<pfcData->vdcAVG.scaler;
    
    pfcData->vacAVG.sampleLimit = PFC_INPUT_FREQUENCY_COUNTER;

/** Initialize PI controlling PFC Current Loop */    
    pfcData->piCurrent.kp = KP_I;
    pfcData->piCurrent.ki = KI_I;
    pfcData->piCurrent.kpScale = KP_I_SCALE;
    pfcData->piCurrent.kiScale = KI_I_SCALE;
    pfcData->piCurrent.maxOutput = INT16_MAX;
    pfcData->piCurrent.minOutput = 0;
    
/** Initialize PI controlling PFC Voltage Loop */    
    pfcData->piVoltage.kp = KP_V;
    pfcData->piVoltage.ki = KI_V;
    pfcData->piVoltage.kpScale = KP_V_SCALE;
    pfcData->piVoltage.kiScale = KI_V_SCALE;
    pfcData->piVoltage.maxOutput = INT16_MAX;
    pfcData->piVoltage.minOutput = 0;
    
    pfcData->state = PFC_INIT;
    pfcData->faultStatus = PFC_FAULT_NONE;
    pfcData->sampleCorrectionEnable = 0;
}
/**
 * <B> Function: PFC_ResetParams(PFC_T *pData)  </B>
 * 
 * @brief Function to reset the variables related to moving average filter of 
 * Vdc,variables related to moving average filter of Vac,variables related to 
 * PI integrator and the duty cycle.
 * @param Pointer to the data structure containing PFC related variables
 * @return none
 * @example
 * <code>
 * PFC_ResetParams(PFC_T *pData);
 * </code>
 */
void PFC_ResetParams(PFC_T *pData)
{   
    /** Initialize variables related to moving average filter - Vdc */    
    pData->vdcAVG.sum = 0;
    pData->vdcAVG.samples = 0;
    pData->vdcAVG.status = 0;

    /** Initialize variables related to moving average filter - Vac */
    pData->vacAVG.sum = 0;
    pData->vacAVG.samples = 0;
    pData->vacRMS.sum = 0;
    pData->vacRMS.samples = 0;
    pData->vacRMS.peak = 0;
    pData->vacRMS.status = 0;

    /** Initialize variables related to PI integrator */
    pData->piVoltage.integralOut = 0;
    pData->piCurrent.integralOut = 0;

    /** Initialize the duty cycle */
    pData->duty = 0;
}

/**
 * <B> Function: PFC_CurrentSampleCorrection(PFC_T *pData)  </B>
 * 
 * @brief Function to calculate average value of current in discontinuous 
 * conduction mode.
 * @param Pointer to the data structure containing PFC related variables
 * @return average current output
 * @example
 * <code>
 * PFC_CurrentSampleCorrection(PFC_T *pData);
 * </code>
 */
static int16_t PFC_CurrentSampleCorrection(PFC_T *pData)
{
    int16_t output = Q15(0.9999);
    
    /** Check if ideal duty is positive value */
    if(pData->boostDutyRatio > 0)
    {
        /** Calculate ratio of actual duty and ideal duty */
        output = __builtin_divf(pData->piCurrent.output, pData->boostDutyRatio);
    }
    /** Check if ratio previous result is greater than 0 */
    if(output > 0)
    {
        output = (int16_t)((__builtin_mulss(pData->iL,output)) >> 15);
    }
    else
    {
        output = pData->iL;
    }
    return(output);
}
/**
 * <B> Function: PFC_CurrentControlLoop(PFC_T *pData)  </B>
 * 
 * @brief Function to execute current control loop of PFC
 * @param Pointer to the data structure containing PFC related variables
 * @return none
 * @example
 * <code>
 * PFC_CurrentControlLoop(PFC_T *pData);
 * </code>
 */
inline static void PFC_CurrentControlLoop(PFC_T *pData)
{
    uint16_t duty;
    
    /** Ensure PFC current  is not negative.*/ 
    if (pData->iL < 0)
    {
        pData->iL  = 1;
    }
    /** Calculate average current if converter operates in discontinuous 
        conduction mode. In continuous conduction mode, measured current is 
        used as is ,as average current is obtained */
    if (pData->sampleCorrectionEnable == 1)
    {
        pData->averageCurrent = PFC_CurrentSampleCorrection(pData);
    }
    else
    {
        pData->averageCurrent = pData->iL;
    }
    
    PFC_PIController(&pData->piCurrent,pData->currentReference-pData->averageCurrent);
    
    /** Calculate duty cycle of PWM that controls PFC in terms of PWM Period */
    duty  = (__builtin_mulss(pData->piCurrent.output,PFC_LOOPTIME_TCY)>>15);
    if (duty  > PFC_MAX_DUTY)
    {
        pData->duty = PFC_MAX_DUTY;
        pData->piCurrent.integralOut = KI_I_INTGRAL_OUT_MAX;       
    }
    else if (duty  < PFC_MIN_DUTY)
    {
        pData->duty = PFC_MIN_DUTY;
    }
    else
    {
        pData->duty = duty;
    }
}
/**
 * <B> Function: PFC_CurrentRefGenerate(PFC_T *pData)  </B>
 * 
 * @brief Function to calculate current reference for the current control loop
 * @param Pointer to the data structure containing PFC related variables
 * @return none
 * @example
 * <code>
 * PFC_CurrentRefGenerate(PFC_T *pData);
 * </code>
 */
inline static void PFC_CurrentRefGenerate(PFC_T *pData)
{
    int16_t tempResult =  0;
    
    /** PI Execution - PFC output voltage control.
        Voltage PI is called at the rate specified by VOLTAGE_LOOP_EXE_RATE */
    if (pData->voltLoopExeRate > VOLTAGE_LOOP_EXE_RATE)
    {
        pData->piVoltage.error = pData->piVoltage.reference-pData->vdcAVG.output;

        if((pData->piVoltage.error > 700) || (pData->piVoltage.error < -700 ))
        {
            pData->piVoltage.ki = KI_V >> 1;
        }
        else
        {
            pData->piVoltage.ki = KI_V;
        }
        PFC_PIController(&pData->piVoltage,pData->piVoltage.error);
        pData->voltLoopExeRate = 0;
    }
    else
    {
       pData->voltLoopExeRate++; 
    }
    
#ifdef PFC_POWER_CONTROL    
    /** Current reference calculation is shown below
        Current reference = (Voltage PI o/p)*(Rectified Vac)*(1/VacRMS^2)*KMUL 
     */ 

    /** Step 1: Current reference calculation :  
            (Voltage PI o/p)*(Rectified AC input voltage)

        Multiply voltage PI output with rectified AC input voltage and 
        right shift it by 18(= 15+3) to make sure result is always less than 
        VacRMS^2 .    

        Note that additional right shift by 3 is compensated in the 
        second step in the current reference calculation */
    
        tempResult = (int16_t) ((__builtin_mulss(pData->piVoltage.output, 
                                            pData->rectifiedVac)) >> 18);

    /** Step 2: Current reference calculation  
        Divide the first step value by  VacRMS^2 */
    if(pData->vacRMS.sqrOutput > 0)
    {
        tempResult = (int16_t)(__builtin_divf(tempResult,pData->vacRMS.sqrOutput));
    }
    /** Step 3:  Current Reference Calculation 
        Multiply second step result with KMUL and right shift by 12 to 
        compensate for the right shift by 3 in the Step 1(above) */
        pData->currentReference = (int16_t)((__builtin_mulss(tempResult,KMUL)) >> 12);
#endif        
    /**  Perform Boundary check of generated current reference */
    if (pData->currentReference > Q15(0.999))
    {
        pData->currentReference = Q15(0.999);
    }
    else if (pData->currentReference < 0)
    {
        pData->currentReference = 0;
    }
}
/**
 * <B> Function: PFC_SignalRectification(PFC_MEASURE_VOLTAGE_T *pSignal)  </B>
 * 
 * @brief Function to rectify the input AC voltage
 * @param Pointer to the data structure containing measured voltage
 * @return Rectified signal
 * @example
 * <code>
 * PFC_SignalRectification();
 * </code>
 */
int16_t PFC_SignalRectification(PFC_MEASURE_VOLTAGE_T *pSignal)
{   
    int16_t output = pSignal->vac - pSignal->offsetVac;
    
    if(output < 0)
    {
        output = -(output);
    }            
    return(output);
}
/**
 * <B> Function: PFC_SquaredRMSCalculate(PFC_RMS_SQUARE_T *pData,int16_t input)  </B>
 * 
 * @brief Function to calculate RMS value of an input signal
 * @param Pointer to the data structure containing variables related to RMS 
 * calculation, current value of signal  
 * @return none.
 * @example
 * <code>
 * PFC_SquaredRMSCalculate(PFC_RMS_SQUARE_T *pData,int16_t input);
 * </code>
 */
static void PFC_SquaredRMSCalculate(PFC_RMS_SQUARE_T *pData,int16_t input)
{       
    pData->sum += (int16_t) (__builtin_mulss(input,input) >> 15);
    if(pData->samples < pData->sampleLimit)
    {
       pData->samples++;  
    }
    else
    {
       pData->sqrOutput = (int16_t)(__builtin_divsd(pData->sum,pData->sampleLimit));
       pData->status    = 1;
       pData->samples   = 0;
       pData->sum       = 0;
    }
}
/**
 * <B> Function: PFC_Average(PFC_AVG_T *pData,int16_t input)  </B>
 * 
 * @brief Function to calculate moving average value of an input Signal
 * @param Pointer to the data structure containing variables related to average 
 * calculation, current value of signal 
 * @return none.
 * @example
 * <code>
 * PFC_Average(PFC_AVG_T *pData,int16_t input);
 * </code>
 */
static void PFC_Average(PFC_AVG_T *pData,int16_t input)
{
    pData->sum = pData->sum + input;
    pData->samples++;
    if(pData->samples >= pData->sampleLimit) 
    {
        pData->output  = (int16_t)( __builtin_divsd(pData->sum,pData->sampleLimit));
        pData->status  = 1;
        pData->sum     = 0;
        pData->samples = 0; 
    }
}


/**
 * <B> Function: PFC_FaultCheck(PFC_T *pData)  </B>
 * 
 * @brief Function to check the different fault status
 * @param Pointer to the data structure containing PFC related variables
 * @return none
 * @example
 * <code>
 * PFC_FaultCheck(PFC_T *pData);
 * </code>
 */
void PFC_FaultCheck(PFC_T *pData)
{   
    /*Check the condition for output over voltage*/
    if(pData->vdcAVG.output >= PFC_OUTPUT_OVER_VOLTAGE_LIMIT)
    {
        pData->faultStatus += PFC_FAULT_OP_OV;    
    }
    /*Check the condition for input under voltage*/
    if(pData->vacRMS.sqrOutput < PFC_INPUT_UNDER_VOLTAGE_LIMIT_LO)
    {
        pData->faultStatus += PFC_FAULT_IP_UV;
    }
    /*Check the condition for input over voltage*/
    if(pData->vacRMS.sqrOutput >= PFC_INPUT_OVER_VOLTAGE_LIMIT_HI)
    {
        pData->faultStatus += PFC_FAULT_IP_OV;
    }
}
// </editor-fold>
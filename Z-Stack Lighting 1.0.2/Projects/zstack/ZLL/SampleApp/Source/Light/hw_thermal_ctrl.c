/**************************************************************************************************
  Filename:       hw_thermal_ctrl.c
  Revised:        $Date: 2012-10-18 14:18:50 -0700 (Thu, 18 Oct 2012) $
  Revision:       $Revision: 31861 $


  Description:    This file contains the hardware specific thermal shutdown
                  control code.

  Copyright 2013 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
  This device will be like a Light device.  This application is not
  intended to be a Light device, but will use the device description
  to implement this sample code.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "OSAL_Timers.h"
#include "zll_samplelight.h"
#include "hw_thermal_ctrl.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#if defined (HAL_BOARD_CC2530EB_REV17) || defined (HAL_BOARD_ZLIGHT)
#define SAMPLELIGHT_THERMAL_2530_COEFFICIET  2
#define SAMPLELIGHT_THERMAL_2530_OFFSET      475
#endif

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static byte lampAppTaskID;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint16 hwThermal_tempMeasure( void );

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      hwThermal_Init
 *
 * @brief   Monitor temperature, shutdown light if reaches threshold
 *
 * @param   taskID -     sample application task ID
 * @param   start -      launch monitoring
 *
 * @return  none
 */
void hwThermal_Init( byte taskID, bool start )
{
  lampAppTaskID = taskID;
  if ( start )
  {
    hwThermal_Monitor( TRUE );
  }
}


/*********************************************************************
 * @fn      hwThermal_Monitor
 *
 * @brief   Monitor temperature, shutdown light if reaches threshold
 *
 * @param   enable -     True to start monitoring, False to stop
 *
 * @return  none
 */
void hwThermal_Monitor( bool enable )
{
  uint16 currTemp = hwThermal_tempMeasure();
  static bool thermalShutdown = FALSE;
  if ( enable == FALSE )
  {
    thermalShutdown = FALSE;
    if ( zllSampleLight_OnOff == LIGHT_ON )
    {
      hwLight_UpdateOnOff( LIGHT_ON );
    }
    osal_stop_timerEx( lampAppTaskID, SAMPLELIGHT_THERMAL_SAMPLE_EVT );
    return;
  }

  if ( currTemp > SAMPLELIGHT_THERMAL_THRESHOLD )
  {
    hwLight_UpdateOnOff( LIGHT_OFF );
    thermalShutdown = TRUE;
  }
  else if ( thermalShutdown && ( currTemp < SAMPLELIGHT_THERMAL_THRESHOLD ) )
  {
    thermalShutdown = FALSE;
    if ( zllSampleLight_OnOff == LIGHT_ON )
    {
      hwLight_UpdateOnOff( LIGHT_ON );
    }
  }
  osal_start_timerEx( lampAppTaskID, SAMPLELIGHT_THERMAL_SAMPLE_EVT,
                      SAMPLELIGHT_THERMAL_SAMPLE_INTERVAL );
}

/*********************************************************************
 * @fn      hwThermal_tempMeasure
 *
 * @brief   Measure the current temperature
 *
 * @param   none
 *
 * @return  temperature in celsius
 */
static uint16 hwThermal_tempMeasure( void )
{
  uint16 tempSample;
  TR0 |= 0x01;      // TR0 must be modified prior to ATEST,
  ATEST |= 0x01;    // otherwise P0_0 and P0_1 forced with 0
  tempSample = HalAdcRead( HAL_ADC_CHN_TEMP, HAL_ADC_RESOLUTION_12 );
  TR0 &= ~0x01;
  ATEST &= ~0x01;
  tempSample = (tempSample - SAMPLELIGHT_THERMAL_2530_OFFSET)/SAMPLELIGHT_THERMAL_2530_COEFFICIET;
#if 0 // Debug
#if (HAL_LCD == TRUE)
  HalLcdWriteStringValue("temp(C):",tempSample, 10, HAL_LCD_LINE_3);
#endif
#endif
  return tempSample;
}

/****************************************************************************
****************************************************************************/



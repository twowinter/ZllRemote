/**************************************************************************************************
  Filename:       zll_effects_ctrl.c
  Revised:        $Date: 2013-12-02 13:41:13 -0800 (Mon, 02 Dec 2013) $
  Revision:       $Revision: 36340 $


  Description:    This file contains the application-level ZigBee Light Link
                  effects contrl.


  Copyright 2012-2013 Texas Instruments Incorporated. All rights reserved.

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
#include "zll_effects_ctrl.h"
#include "hal_led.h"
#include "hw_light_ctrl.h"
#include "zll_samplelight.h"
#ifdef ZCL_LEVEL_CTRL
#include "zcl_level_ctrl.h"
#endif

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define HUE_GREEN   0x55
#define HUE_ORANGE  0x1C

/*********************************************************************
 * TYPEDEFS
 */
typedef struct
{
  uint16 clusterID;
  uint8 commandID;
  uint8 effectID;
  uint8 effectVariant;
  uint8 state;
} currentEffect_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static currentEffect_t _currentEffect;
static byte zclLight_TaskID;
static zclGCB_OnOff_t zllEffects_OnOffCB;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn          zllEffects_Init
 *
 * @brief
 *
 * @param       none
 *
 * @return      none
 */
void zllEffects_Init( byte taskID, zclGCB_OnOff_t OnOffCB )
{
  zclLight_TaskID = taskID;
  zllEffects_OnOffCB = OnOffCB;
  _currentEffect.state = 0;
}

/*********************************************************************
 * @fn      zllEffects_Blink
 *
 * @brief   Perform "blink" effect on light.
 *
 * @param   none
 *
 * @return  none
 */
void zllEffects_Blink( void )
{
  zllEffects_InitiateEffect( ZCL_CLUSTER_ID_GEN_IDENTIFY,
                             COMMAND_IDENTIFY_TRIGGER_EFFECT,
                             EFFECT_ID_BLINK, 0 );
}

/*********************************************************************
 * @fn      zllEffects_InitiateEffect
 *
 * @brief   Initiate light effect
 *
 * @param   clusterID -     Cluster ID (e.g. Identify or On/Off)
 * @param   commandID -     Command ID
 * @param   effectID -      Effect ID
 * @param   effectVariant - Effect Variant
 *
 * @return  none
 */
void zllEffects_InitiateEffect( uint16 clusterID, uint8 commandID, uint8 effectID, uint8 effectVariant )
{
  _currentEffect.clusterID = clusterID;
  _currentEffect.commandID = commandID;
  _currentEffect.effectID = effectID;
  _currentEffect.effectVariant = effectVariant;
  _currentEffect.state = 0xFF; //start
  hwLight_Refresh( REFRESH_BYPASS );

  if ( !( (commandID == COMMAND_IDENTIFY_TRIGGER_EFFECT ) && ( effectID == EFFECT_ID_FINISH_EFFECT ) ) )
  {
    osal_set_event( zclLight_TaskID, SAMPLELIGHT_EFFECT_PROCESS_EVT );
  }
}

/*********************************************************************
 * @fn      zllEffects_ProcessEffect
 *
 * @brief   Process ongoing light effect
 *
 * @param   none
 *
 * @return  none
 */
void zllEffects_ProcessEffect()
{
  if ( ( _currentEffect.clusterID == ZCL_CLUSTER_ID_GEN_IDENTIFY )
       && ( _currentEffect.commandID == COMMAND_IDENTIFY_TRIGGER_EFFECT ) )
  {
    if ( _currentEffect.state == 0 ) // end of effect
    {
      // revert effect bypass
      hwLight_Refresh( REFRESH_RESUME );
      return;
    }
    switch ( _currentEffect.effectID )
    {
    case EFFECT_ID_BLINK:
      {
        if ( _currentEffect.state == 0xFF )
        {
          _currentEffect.state = 0;
#ifdef ZLL_HW_LED_LAMP
          hwLight_UpdateOnOff ( ( zllSampleLight_OnOff == LIGHT_ON ) ? LIGHT_OFF : LIGHT_ON );
#else
          HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
#endif
          osal_start_timerEx( zclLight_TaskID, SAMPLELIGHT_EFFECT_PROCESS_EVT, 200 );
        }
      }
      break;

    case EFFECT_ID_BREATHE:
      {
        if ( _currentEffect.state == 0xFF )
        {
          _currentEffect.state = 14;
        }
        else
        {
          _currentEffect.state--;
        }
        hwLight_UpdateOnOff( ( _currentEffect.state & 0x01 ) ? LIGHT_ON : LIGHT_OFF );
        osal_start_timerEx( zclLight_TaskID, SAMPLELIGHT_EFFECT_PROCESS_EVT, 1000 );
      }
      break;

    case EFFECT_ID_OKAY:
      {
#ifdef ZCL_COLOR_CTRL
        _currentEffect.state = 0;
        hwLight_UpdateOnOff( LIGHT_ON );
#ifdef ZLL_HW_LED_LAMP
        hwLight_UpdateLampColorHueSat( HUE_GREEN, 0xFF, zclLevel_CurrentLevel );
        osal_start_timerEx( zclLight_TaskID, SAMPLELIGHT_EFFECT_PROCESS_EVT, 1000 );
#endif //ZLL_HW_LED_LAMP
#else //ZCL_COLOR_CTRL
        if ( _currentEffect.state == 0xFF )
        {
          _currentEffect.state = 3;
        }
        else
        {
          _currentEffect.state--;
        }
        hwLight_UpdateOnOff( ( _currentEffect.state & 0x01 ) ? LIGHT_ON : LIGHT_OFF );
        osal_start_timerEx( zclLight_TaskID, SAMPLELIGHT_EFFECT_PROCESS_EVT, 500 );
#endif
      }
      break;

    case EFFECT_ID_CHANNEL_CHANGE:
      {
        hwLight_UpdateOnOff( LIGHT_ON );
#ifdef ZCL_COLOR_CTRL
        _currentEffect.state = 0;
#ifdef ZLL_HW_LED_LAMP
        hwLight_UpdateLampColorHueSat( HUE_ORANGE, 0xFF, zclLevel_CurrentLevel );
        osal_start_timerEx( zclLight_TaskID, SAMPLELIGHT_EFFECT_PROCESS_EVT, 8000 );
#endif //ZLL_HW_LED_LAMP
#else //ZCL_COLOR_CTRL
        if ( _currentEffect.state == 0xFF )
        {
          _currentEffect.state = 1;
        }
        else
        {
          _currentEffect.state--;
        }
        if ( _currentEffect.state > 0 )
        {
#ifdef ZCL_LEVEL_CTRL
          hwLight_UpdateLampLevel( LEVEL_MAX ); // maximum brightness
#endif
          osal_start_timerEx( zclLight_TaskID, SAMPLELIGHT_EFFECT_PROCESS_EVT, 500 );
        }
        else
        {
#ifdef ZCL_LEVEL_CTRL
          hwLight_UpdateLampLevel( LEVEL_MIN ); // minimum brightness
#endif
          osal_start_timerEx( zclLight_TaskID, SAMPLELIGHT_EFFECT_PROCESS_EVT, 7500 );
        }
#endif
      }
      break;

    case EFFECT_ID_FINISH_EFFECT:
    case EFFECT_ID_STOP_EFFECT:
    default: //unknown effects should not block normal operation
      {
        _currentEffect.state = 0;
        hwLight_Refresh( REFRESH_RESUME );
      }
      break;
    }
  }
  else if ( ( _currentEffect.clusterID == ZCL_CLUSTER_ID_GEN_ON_OFF )
            && ( _currentEffect.commandID == COMMAND_OFF_WITH_EFFECT ) )
  {
#ifdef ZCL_LEVEL_CTRL
    static uint8 step = 0;
    static uint8 level = 0;
#else
    _currentEffect.state = 0;
#endif //ZCL_LEVEL_CTRL
    if ( _currentEffect.state == 0 ) // end of effect
    {
      // Turn off the light
      zllEffects_OnOffCB( COMMAND_OFF );
      // revert effect bypass
      hwLight_Refresh( REFRESH_RESUME );
      return;
    }
#ifdef ZCL_LEVEL_CTRL
    switch ( _currentEffect.effectID )
    {
      case EFFECT_ID_DELAY_ALL_OFF:
        {
          if ( _currentEffect.effectVariant == 0 )
          {
            if ( _currentEffect.state == 0xFF )
            {
              _currentEffect.state = 7;
              level = zclLevel_CurrentLevel;
              step = MAX( (level/8), 1 );
            }
            else
            {
              _currentEffect.state--;
            }
            level = ( (level-LEVEL_MIN) > step) ? (level - step) : LEVEL_MIN;
          }
          else if ( _currentEffect.effectVariant == 1 )
          {
            _currentEffect.state = 0;
          }
          else if ( _currentEffect.effectVariant == 2 )
          {
            if ( _currentEffect.state == 0xFF )
            {
              _currentEffect.state = 127;
              level = zclLevel_CurrentLevel;
              step = MAX( (level/16), 1 );
            }
            else
            {
              _currentEffect.state--;
            }
            if ( _currentEffect.state == 120 )
            {
              step = 1;
            }
            level = ( (level-LEVEL_MIN) > step) ? (level - step) : LEVEL_MIN;
          }
          else //unknown variant
          {
             _currentEffect.state = 0;
          }
        }
        break;

      case EFFECT_ID_DYING_LIGHT:
        {
          if ( _currentEffect.state == 0xFF )
          {
            _currentEffect.state = 14;
            level = zclLevel_CurrentLevel;
            step = MAX( (zclLevel_CurrentLevel/25), 1 );
          }
          else
          {
            _currentEffect.state--;
          }

          if ( _currentEffect.state > 10 )
          {
            level = ( (LEVEL_MAX-level) > step) ? (level + step) : LEVEL_MAX;
          }
          else
          {
            step = MAX( (zclLevel_CurrentLevel/10), 1 );
            level = ( (level-LEVEL_MIN) > step) ? (level - step) : LEVEL_MIN;
          }
        }
        break;
      default: //unknown effect
        {
          _currentEffect.state = 0;
        }
    }
#ifdef ZLL_HW_LED_LAMP
#ifdef ZCL_COLOR_CTRL
    hwLight_UpdateLampColorHueSat( zclColor_CurrentHue, zclColor_CurrentSaturation, level );
#else
    hwLight_UpdateLampLevel( level );
#endif //ZCL_COLOR_CTRL
#endif //ZLL_HW_LED_LAMP
    osal_start_timerEx( zclLight_TaskID, SAMPLELIGHT_EFFECT_PROCESS_EVT, 100 );
#endif //ZCL_LEVEL_CTRL
  }
}

/****************************************************************************
****************************************************************************/



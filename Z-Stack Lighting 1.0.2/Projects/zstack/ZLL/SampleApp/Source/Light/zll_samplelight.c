/**************************************************************************************************
  Filename:       zll_samplelight.c
  Revised:        $Date: 2013-12-05 14:36:32 -0800 (Thu, 05 Dec 2013) $
  Revision:       $Revision: 36404 $


  Description:    Zigbee Cluster Library - Light Link (ZLL) Light Sample
                  Application.


  Copyright 2010-2013 Texas Instruments Incorporated. All rights reserved.

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
#include "onboard.h"

#include "nwk_util.h"


#include "zll_target.h"
#include "zll_samplelight.h"
#include "zll_effects_ctrl.h"

#ifdef THERMAL_SHUTDOWN
#include "hw_thermal_ctrl.h"
#endif

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_timer.h"

#ifdef HAL_BOARD_ZLIGHT
#include "osal_clock.h"
#else
// NWK key printout
#include "osal_nv.h"
#endif

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define FACTORY_RESET_KEY     HAL_KEY_RIGHT
#define CLASSIC_COMMISS_KEY   HAL_KEY_UP
#define PERMIT_JOIN_KEY       HAL_KEY_DOWN
#define DISPLAY_NWK_KEY_KEY   HAL_KEY_LEFT

#define KEY_HOLD_SHORT_INTERVAL    1
#define KEY_HOLD_LONG_INTERVAL     5
#define PERMIT_JOIN_DURATION       60


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zllSampleLight_TaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */
#ifdef ZLL_1_0_HUB_COMPATIBILITY
extern ZStatus_t zll_RegisterSimpleDesc( SimpleDescriptionFormat_t *simpleDesc );
#endif

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zllSampleLight_HandleKeys( byte shift, byte keys );
static void zllSampleLight_BasicResetCB( void );
static void zllSampleLight_IdentifyCB( zclIdentify_t *pCmd );
static void zllSampleLight_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void zllSampleLight_OnOffCB( uint8 cmd );
static void zllSampleLight_OnOff_OffWithEffectCB( zclOffWithEffect_t *pCmd );
static void zllSampleLight_OnOff_OnWithRecallGlobalSceneCB( void );
static void zllSampleLight_OnOff_OnWithTimedOffCB( zclOnWithTimedOff_t *pCmd );
static void zllSampleLight_ProcessIdentifyTimeChange( void );
static void zllSampleLight_ProcessOnWithTimedOffTimer( void );
static void zllSampleLight_IdentifyEffectCB( zclIdentifyTriggerEffect_t *pCmd );

// This callback is called to process attribute not handled in ZCL
static ZStatus_t zllSampleLight_AttrReadWriteCB( uint16 clusterId, uint16 attrId,
                                       uint8 oper, uint8 *pValue, uint16 *pLen );

static uint8 zllSampleLight_SceneStoreCB( zclSceneReq_t *pReq );
static void zllSampleLight_SceneRecallCB( zclSceneReq_t *pReq );

#if ( HAL_LCD == TRUE )
static void zllSampleLight_PrintNwkKey( uint8 reverse );
#endif

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zllSampleLight_GenCmdCBs =
{
  zllSampleLight_BasicResetCB,            // Basic Cluster Reset command
  zllSampleLight_IdentifyCB,              // Identify command
#ifdef ZCL_EZMODE
  NULL,                                   // Identify EZ-Mode Invoke command
  NULL,                                   // Identify Update Commission State command
#endif
  zllSampleLight_IdentifyEffectCB,        // Identify Trigger Effect command
  zllSampleLight_IdentifyQueryRspCB,      // Identify Query Response command
  zllSampleLight_OnOffCB,                 // On/Off cluster commands
  zllSampleLight_OnOff_OffWithEffectCB,   // On/Off cluster enhanced command Off with Effect
  zllSampleLight_OnOff_OnWithRecallGlobalSceneCB, // On/Off cluster enhanced command On with Recall Global Scene
  zllSampleLight_OnOff_OnWithTimedOffCB,  // On/Off cluster enhanced command On with Timed Off
#ifdef ZCL_LEVEL_CTRL
  zclLevel_MoveToLevelCB,                 // Level Control Move to Level command
  zclLevel_MoveCB,                        // Level Control Move command
  zclLevel_StepCB,                        // Level Control Step command
  zclLevel_StopCB,                        // Level Control Stop command
#endif
#ifdef ZCL_GROUPS
  NULL,                                   // Group Response commands
#endif
#ifdef ZCL_SCENES
  zllSampleLight_SceneStoreCB,            // Scene Store Request command
  zllSampleLight_SceneRecallCB,           // Scene Recall Request command
  NULL,                                   // Scene Response command
#endif
#if ZCL_ALARMS
  NULL,                                   // Alarm (Response) commands
#endif
#ifdef SE_UK_EXT
  NULL,                                   // Get Event Log command
  NULL,                                   // Publish Event Log command
#endif
  NULL,                                   // RSSI Location command
  NULL                                    // RSSI Location Response command
};

#ifdef ZCL_COLOR_CTRL
static zclLighting_AppCallbacks_t zllSampleLight_LightingCmdCBs =
{
  zclColor_MoveToHueCB,   //Move To Hue Command
  zclColor_MoveHueCB,   //Move Hue Command
  zclColor_StepHueCB,   //Step Hue Command
  zclColor_MoveToSaturationCB,   //Move To Saturation Command
  zclColor_MoveSaturationCB,   //Move Saturation Command
  zclColor_StepSaturationCB,   //Step Saturation Command
  zclColor_MoveToHueAndSaturationCB,   //Move To Hue And Saturation  Command
  zclColor_MoveToColorCB, // Move To Color Command
  zclColor_MoveColorCB,   // Move Color Command
  zclColor_StepColorCB,   // STEP To Color Command
  NULL,                                     // Move To Color Temperature Command
  zclColor_EnhMoveToHueCB,// Enhanced Move To Hue
  zclColor_MoveEnhHueCB,  // Enhanced Move Hue;
  zclColor_StepEnhHueCB,  // Enhanced Step Hue;
  zclColor_MoveToEnhHueAndSaturationCB, // Enhanced Move To Hue And Saturation;
  zclColor_SetColorLoopCB, // Color Loop Set Command
  zclColor_StopCB,        // Stop Move Step;
};
#endif //ZCL_COLOR_CTRL

/*********************************************************************
 * @fn          zllSampleLight_Init
 *
 * @brief       Initialization function for the Sample Light App Task.
 *
 * @param       task_id
 *
 * @return      none
 */
void zllSampleLight_Init( byte task_id )
{
  zllSampleLight_TaskID = task_id;

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( SAMPLELIGHT_ENDPOINT, &zllSampleLight_GenCmdCBs );

#ifdef ZLL_HW_LED_LAMP
  HalTimer1Init(0);
#endif //ZLL_HW_LED_LAMP

  zllEffects_Init(zllSampleLight_TaskID, zllSampleLight_OnOffCB);

#ifdef ZCL_LEVEL_CTRL
  zclLevel_init(zllSampleLight_TaskID, zllSampleLight_OnOffCB);
#else
  #ifdef ZLL_HW_LED_LAMP
    halTimer1SetChannelDuty (WHITE_LED, PWM_FULL_DUTY_CYCLE); //initialize on/off LED to full power
  #endif
#endif //ZCL_LEVEL_CTRL

#ifdef ZCL_COLOR_CTRL
  // Register the ZCL Lighting Cluster Library callback functions
  zclLighting_RegisterCmdCallbacks( SAMPLELIGHT_ENDPOINT, &zllSampleLight_LightingCmdCBs );
  zclColor_init(zllSampleLight_TaskID);
#endif //#ifdef ZCL_COLOR_CTRL

  // Register the application's attribute list
  zcl_registerAttrList( SAMPLELIGHT_ENDPOINT, SAMPLELIGHT_NUM_ATTRIBUTES, zllSampleLight_Attrs );

  // Register the application's callback function to read the Scene Count attribute.
  zcl_registerReadWriteCB( SAMPLELIGHT_ENDPOINT, zllSampleLight_AttrReadWriteCB, NULL );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( zllSampleLight_TaskID );

  zllTarget_RegisterApp( &zllSampleLight_SimpleDesc, &zllSampleLight_DeviceInfo );

  zllTarget_RegisterIdentifyCB( zllSampleLight_IdentifyCB );

  zllTarget_InitDevice();

  zllSampleLight_OnOffCB( zllSampleLight_OnOff );

#ifdef ZLL_1_0_HUB_COMPATIBILITY
  zclGeneral_RegisterCmdCallbacks( SAMPLELIGHT_ENDPOINT2, &zllSampleLight_GenCmdCBs );
#ifdef ZCL_COLOR_CTRL
  zclLighting_RegisterCmdCallbacks( SAMPLELIGHT_ENDPOINT2, &zllSampleLight_LightingCmdCBs );
#endif //ZCL_COLOR_CTRL
  zcl_registerAttrList( SAMPLELIGHT_ENDPOINT2, SAMPLELIGHT_NUM_ATTRIBUTES, zllSampleLight_Attrs );
  zll_RegisterSimpleDesc( &zllSampleLight_SimpleDesc2 );
#endif //ZLL_1_0_HUB_COMPATIBILITY

#ifdef THERMAL_SHUTDOWN
  hwThermal_Init( zllSampleLight_TaskID, TRUE );
#endif
}

/*********************************************************************
 * @fn          zclSample_event_loop
 *
 * @brief       Event Loop Processor for the Sample Light App Task.
 *
 * @param       task_id
 * @param       events - events bitmap
 *
 * @return      unprocessed events bitmap
 */
uint16 zllSampleLight_event_loop( uint8 task_id, uint16 events )
{
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    afIncomingMSGPacket_t *pMsg;

    if ( (pMsg = (afIncomingMSGPacket_t *)osal_msg_receive( zllSampleLight_TaskID )) )
    {
      switch ( pMsg->hdr.event )
      {
        case KEY_CHANGE:
          zllSampleLight_HandleKeys( ((keyChange_t *)pMsg)->state, ((keyChange_t *)pMsg)->keys );
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

#ifdef THERMAL_SHUTDOWN
  if ( events & SAMPLELIGHT_THERMAL_SAMPLE_EVT )
  {
    hwThermal_Monitor( TRUE );
    return ( events ^ SAMPLELIGHT_THERMAL_SAMPLE_EVT );
  }
#endif

  if ( events & SAMPLELIGHT_IDENTIFY_TIMEOUT_EVT )
  {
    zllSampleLight_ProcessIdentifyTimeChange();

    return ( events ^ SAMPLELIGHT_IDENTIFY_TIMEOUT_EVT );
  }

  if ( events & SAMPLELIGHT_EFFECT_PROCESS_EVT )
  {
    zllEffects_ProcessEffect();
    return ( events ^ SAMPLELIGHT_EFFECT_PROCESS_EVT );
  }

  if ( events & SAMPLELIGHT_ON_TIMED_OFF_TIMER_EVT )
  {
    zllSampleLight_ProcessOnWithTimedOffTimer();
    return ( events ^ SAMPLELIGHT_ON_TIMED_OFF_TIMER_EVT );
  }

#ifdef ZCL_LEVEL_CTRL
    //update the level
    zclLevel_process(&events);
#endif //ZCL_COLOR_CTRL

#ifdef ZCL_COLOR_CTRL
    //update the color
    zclColor_process(&events);
    zclColor_processColorLoop(&events);
#endif //ZCL_COLOR_CTRL


  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      zllSampleLight_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events.
 *
 * @return  none
 */
static void zllSampleLight_HandleKeys( byte shift, byte keys )
{
  (void)shift;  // Intentionally unreferenced parameter
#ifdef HAL_BOARD_ZLIGHT
  // Zlight has only a single button
  static uint32 keyPressTime = 0;
  if ( keys )
  {
    keyPressTime = osal_getClock();
  }
  else //key released
  {
    if ( keyPressTime )
    {
      keyPressTime = ( osal_getClock() - keyPressTime );
      if ( keyPressTime <= KEY_HOLD_SHORT_INTERVAL )
      {
        zllTarget_PermitJoin( PERMIT_JOIN_DURATION );
      }
      else if ( keyPressTime > KEY_HOLD_LONG_INTERVAL )
      {
        zllTarget_ClassicalCommissioningStart();
      }
      else
      {
        zllTarget_ResetToFactoryNew();
      }
      keyPressTime = 0;
    }
  }
#else //HAL_BOARD_CC2530EB ?
  if ( keys & CLASSIC_COMMISS_KEY )
  {
    zllTarget_ClassicalCommissioningStart();
  }

  if ( keys & FACTORY_RESET_KEY )
  {
    zllTarget_ResetToFactoryNew();
  }

  if ( keys & PERMIT_JOIN_KEY )
  {
    zllTarget_PermitJoin( PERMIT_JOIN_DURATION );
    HalLcdWriteString( "PermitJoin", HAL_LCD_LINE_3 );
  }

  if ( keys & DISPLAY_NWK_KEY_KEY )
  {
#if ( HAL_LCD == TRUE )
    zllSampleLight_PrintNwkKey( FALSE );
#endif //( HAL_LCD == TRUE )
  }

#endif //HAL_BOARD_ZLIGHT
}

#if (HAL_LCD == TRUE)
/*********************************************************************
 * @fn      zllSampleLight_PrintNwkKey
 *
 * @brief   Print current NWK key on the LCD screen
 *
 * @param   reverse - TRUE to print the bytes in reverse order
 *
 * @return  none
 */
static void zllSampleLight_PrintNwkKey( uint8 reverse )
{
  nwkKeyDesc KeyInfo;
  uint8 lcd_buf[SEC_KEY_LEN+1];
  uint8 lcd_buf1[SEC_KEY_LEN+1];
  uint8 lcd_buf2[SEC_KEY_LEN+1];
  uint8 i;
  static uint8 clear = 1;

  clear ^= 1;
  if ( clear )
  {
    nwk_Status( NWK_STATUS_ROUTER_ADDR, _NIB.nwkDevAddress );
    nwk_Status( NWK_STATUS_PARENT_ADDR, _NIB.nwkCoordAddress );
    HalLcdWriteString( "", HAL_LCD_LINE_3 );
    return;
  }

  // Swap active and alternate keys writing them directly to NV
  osal_nv_read(ZCD_NV_NWK_ACTIVE_KEY_INFO, 0,
               sizeof(nwkKeyDesc), &KeyInfo);

  for (i = 0; i < SEC_KEY_LEN/2; i++)
  {
    uint8 key = KeyInfo.key[i];
    _itoa(KeyInfo.key[i], &lcd_buf[i*2], 16);
    if( (key & 0xF0) == 0)
    {
      //_itoa shift to significant figures, so swaps bytes if 0x0?
      lcd_buf[(i*2) + 1] = lcd_buf[i*2];
      lcd_buf[i*2] = '0';
    }
    if( (key & 0xF) == 0)
    {
      lcd_buf[(i*2) + 1] = '0';
    }
  }

  if ( reverse )
  {
    //print in daintree format. Reverse bytes.
    for (i = 0; i < (SEC_KEY_LEN); i+=2)
    {
      lcd_buf2[SEC_KEY_LEN - (i+2)] = lcd_buf[i];
      lcd_buf2[SEC_KEY_LEN - (i+1)] = lcd_buf[i+1];
    }
  }
  else
  {
    osal_memcpy(lcd_buf1, lcd_buf, SEC_KEY_LEN);
  }

  for (i = 0; i < SEC_KEY_LEN/2; i++)
  {
    uint8 key = KeyInfo.key[i + SEC_KEY_LEN/2];
    _itoa(KeyInfo.key[i + SEC_KEY_LEN/2], &lcd_buf[i*2], 16);
    if( (key & 0xF0) == 0)
    {
      //_itoa shift to significant figures, so swaps bytes if 0x0?
      lcd_buf[(i*2) + 1] = lcd_buf[i*2];
      lcd_buf[i*2] = '0';
    }
    if( (key & 0xF) == 0)
    {
      lcd_buf[(i*2) + 1] = '0';
    }
  }

  if ( reverse )
  {
    //print in daintree format. Reverse bytes.
    for (i = 0; i < (SEC_KEY_LEN); i+=2)
    {
      lcd_buf1[SEC_KEY_LEN - (i+2)] = lcd_buf[i];
      lcd_buf1[SEC_KEY_LEN - (i+1)] = lcd_buf[i+1];
    }
  }
  else
  {
    osal_memcpy(lcd_buf2, lcd_buf, SEC_KEY_LEN);
  }

  lcd_buf1[SEC_KEY_LEN] = '\0';
  lcd_buf2[SEC_KEY_LEN] = '\0';
  if ( reverse )
  {
    HalLcdWriteString( "NWK KEY (Rev.):", HAL_LCD_LINE_1 );
  }
  else
  {
    HalLcdWriteString( "NWK KEY: ", HAL_LCD_LINE_1 );
  }
  HalLcdWriteString( (char*)lcd_buf1, HAL_LCD_LINE_2 );
  HalLcdWriteString( (char*)lcd_buf2, HAL_LCD_LINE_3 );
}
#endif //(HAL_LCD == TRUE)

/*********************************************************************
 * @fn      zllSampleLight_ProcessIdentifyTimeChange
 *
 * @brief   Called to process any change to the IdentifyTime attribute.
 *
 * @param   none
 *
 * @return  none
 */
static void zllSampleLight_ProcessIdentifyTimeChange( void )
{
  if ( zllSampleLight_IdentifyTime > 0 )
  {
    zllSampleLight_IdentifyTime--;
    zllEffects_Blink();
    osal_start_timerEx( zllSampleLight_TaskID, SAMPLELIGHT_IDENTIFY_TIMEOUT_EVT, 1000 );
  }
}

/*********************************************************************
 * @fn      zllSampleLight_ProcessOnWithTimedOffTimer
 *
 * @brief   Called to process On with Timed Off attributes changes over time.
 *
 * @param   none
 *
 * @return  none
 */
static void zllSampleLight_ProcessOnWithTimedOffTimer( void )
{
  if ( ( zllSampleLight_OnOff == LIGHT_ON ) && ( zllSampleLight_OnTime > 0 ) )
  {
    zllSampleLight_OnTime--;
    if ( zllSampleLight_OnTime <= 0 )
    {
      zllSampleLight_OffWaitTime = 0x00;
      zllSampleLight_OnOffCB( COMMAND_OFF );
    }
  }
  if ( ( zllSampleLight_OnOff == LIGHT_OFF ) && ( zllSampleLight_OffWaitTime > 0 ) )
  {
    zllSampleLight_OffWaitTime--;
    if ( zllSampleLight_OffWaitTime <= 0 )
    {
      osal_stop_timerEx( zllSampleLight_TaskID, SAMPLELIGHT_ON_TIMED_OFF_TIMER_EVT);
      return;
    }
  }

  if ( ( zllSampleLight_OnTime > 0 ) || ( zllSampleLight_OffWaitTime > 0 ) )
  {
    osal_start_timerEx( zllSampleLight_TaskID, SAMPLELIGHT_ON_TIMED_OFF_TIMER_EVT, 100 );
  }
}

/*********************************************************************
 * @fn      zllSampleLight_AttrReadWriteCB
 *
 * @brief   Read/write callbackfor read/wtire attrs tha have NULL dataPtr
 *
 *          Note: This function gets called only when the pointer
 *                'dataPtr' to the Scene Count attribute value is
 *                NULL in the attribute database registered with
 *                the ZCL.
 *
 * @param   clusterId - cluster that attribute belongs to
 * @param   attrId - attribute to be read or written
 * @param   oper - ZCL_OPER_LEN, ZCL_OPER_READ, or ZCL_OPER_WRITE
 * @param   pValue - pointer to attribute value
 * @param   pLen - length of attribute value read
 *
 * @return  status
 */
ZStatus_t zllSampleLight_AttrReadWriteCB( uint16 clusterId, uint16 attrId,
                                       uint8 oper, uint8 *pValue, uint16 *pLen )
{
  ZStatus_t status = ZCL_STATUS_SUCCESS;

#if defined ZCL_SCENES
  //SceneCount Attr
  if( (clusterId == ZCL_CLUSTER_ID_GEN_SCENES) &&
     (attrId == ATTRID_SCENES_COUNT) )
  {
    status = zclGeneral_ReadSceneCountCB(clusterId, attrId, oper, pValue, pLen);
  } else
#endif //ZCL_SCENES
  //IdentifyTime Attr
  if( (clusterId == ZCL_CLUSTER_ID_GEN_IDENTIFY) &&
     (attrId == ATTRID_IDENTIFY_TIME) )
  {
    switch ( oper )
    {
      case ZCL_OPER_LEN:
        *pLen = 2; // uint16
        break;

      case ZCL_OPER_READ:
        pValue[0] = LO_UINT16( zllSampleLight_IdentifyTime );
        pValue[1] = HI_UINT16( zllSampleLight_IdentifyTime );

        if ( pLen != NULL )
        {
          *pLen = 2;
        }
        break;

      case ZCL_OPER_WRITE:
      {
        zclIdentify_t cmd;
        cmd.identifyTime = BUILD_UINT16( pValue[0], pValue[1] );

        zllSampleLight_IdentifyCB( &cmd );

        break;
      }

      default:
        status = ZCL_STATUS_SOFTWARE_FAILURE; // should never get here!
        break;
    }
  }
  else
  {
    status = ZCL_STATUS_SOFTWARE_FAILURE; // should never get here!
  }

  return ( status );
}
/*********************************************************************
 * @fn      zllSampleLight_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to default values.
 *
 * @param   none
 *
 * @return  none
 */
static void zllSampleLight_BasicResetCB( void )
{
  // Reset all attributes to default values
}

/*********************************************************************
 * @fn      zllSampleLight_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   srcAddr - source address and endpoint of the response message
 * @param   identifyTime - the number of seconds to identify yourself
 *
 * @return  none
 */
static void zllSampleLight_IdentifyCB( zclIdentify_t *pCmd )
{
  zllSampleLight_IdentifyTime = pCmd->identifyTime;
  zllSampleLight_ProcessIdentifyTimeChange();
}


/*********************************************************************
 * @fn      zllSampleLight_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   srcAddr - source address and endpoint of the response message
 * @param   identifyTime - the number of seconds to identify yourself
 *
 * @return  none
 */
static void zllSampleLight_IdentifyEffectCB( zclIdentifyTriggerEffect_t *pCmd )
{
  HalLcdWriteStringValue( "IdentifyEffId", pCmd->effectId, 16, HAL_LCD_LINE_1 );
  zllEffects_InitiateEffect( ZCL_CLUSTER_ID_GEN_IDENTIFY, COMMAND_IDENTIFY_TRIGGER_EFFECT,
                                 pCmd->effectId, pCmd->effectVariant );
}

/*********************************************************************
 * @fn      zllSampleLight_IdentifyQueryRspCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Query Response Command for this application.
 *
 * @param   srcAddr - requestor's address
 * @param   timeout - number of seconds to identify yourself (valid for query response)
 *
 * @return  none
 */
static void zllSampleLight_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp )
{
  // Query Response (with timeout value)
  (void)pRsp;
}

/*********************************************************************
 * @fn      zllSampleLight_OnOffCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an On/Off Command for this application.
 *
 * @param   cmd - COMMAND_ON, COMMAND_OFF, or COMMAND_TOGGLE
 *
 * @return  none
 */
static void zllSampleLight_OnOffCB( uint8 cmd )
{
  // Turn on the light
  if ( cmd == COMMAND_ON )
  {
    zllSampleLight_OnOff = LIGHT_ON;
    zllSampleLight_GlobalSceneCtrl = TRUE;
    if ( zllSampleLight_OnTime == 0 )
    {
      zllSampleLight_OffWaitTime = 0;
    }
  }

  // Turn off the light
  else if ( cmd == COMMAND_OFF )
  {
    zllSampleLight_OnOff = LIGHT_OFF;
    //zllSampleLight_GlobalSceneCtrl = FALSE; //see ZLL spec 11-0037-03 6.6.1.2.1
    zllSampleLight_OnTime = 0;
  }

  // Toggle the light
  else
  {
    if ( zllSampleLight_OnOff == LIGHT_OFF )
    {
      zllSampleLight_OnOff = LIGHT_ON;
      zllSampleLight_GlobalSceneCtrl = TRUE;
      if ( zllSampleLight_OnTime == 0 )
      {
        zllSampleLight_OffWaitTime = 0;
      }
    }
    else
    {
      zllSampleLight_OnOff = LIGHT_OFF;
      zllSampleLight_OnTime = 0;
    }
  }

  hwLight_UpdateOnOff( zllSampleLight_OnOff );

  zllSampleLight_SceneValid = 0;
}


/*********************************************************************
 * @fn      zllSampleLight_OffWithEffect
 *
 * @brief   Callback from the ZCL General Cluster Library when it
 *          received an Off with Effect Command for this application.
 *
 * @param   pCmd - Off with Effect parameters
 *
 * @return  none
 */
static void zllSampleLight_OnOff_OffWithEffectCB( zclOffWithEffect_t *pCmd )
{
  HalLcdWriteStringValueValue( "OffWithEff", pCmd->effectId, 16,
                               pCmd->effectVariant, 16, HAL_LCD_LINE_1 );
  if( zllSampleLight_GlobalSceneCtrl )
  {
    zclSceneReq_t req;
    req.scene = &zllSampleLight_GlobalScene;

    zllSampleLight_SceneStoreCB( &req );
    zllSampleLight_GlobalSceneCtrl = FALSE;
  }
  zllEffects_InitiateEffect( ZCL_CLUSTER_ID_GEN_ON_OFF, COMMAND_OFF_WITH_EFFECT,
                                 pCmd->effectId, pCmd->effectVariant );
}

/*********************************************************************
 * @fn      zllSampleLight_OnOff_OnWithRecallGlobalSceneCB
 *
 * @brief   Callback from the ZCL General Cluster Library when it
 *          received an On with Recall Global Scene Command for this application.
 *
 * @param   none
 *
 * @return  none
 */
static void zllSampleLight_OnOff_OnWithRecallGlobalSceneCB( void )
{
  if( !zllSampleLight_GlobalSceneCtrl )
  {
    zclSceneReq_t req;
    req.scene = &zllSampleLight_GlobalScene;

    zllSampleLight_SceneRecallCB( &req );

    zllSampleLight_GlobalSceneCtrl = TRUE;
  }
  // If the GlobalSceneControl attribute is equal to TRUE, discard the command
}

/*********************************************************************
 * @fn      zllSampleLight_OnOff_OnWithTimedOffCB
 *
 * @brief   Callback from the ZCL General Cluster Library when it
 *          received an On with Timed Off Command for this application.
 *
 * @param   pCmd - On with Timed Off parameters
 *
 * @return  none
 */
static void zllSampleLight_OnOff_OnWithTimedOffCB( zclOnWithTimedOff_t *pCmd )
{
  if ( ( pCmd->onOffCtrl.bits.acceptOnlyWhenOn == TRUE )
      && ( zllSampleLight_OnOff == LIGHT_OFF ) )
  {
    return;
  }

  if ( ( zllSampleLight_OffWaitTime > 0 ) && ( zllSampleLight_OnOff == LIGHT_OFF ) )
  {
    zllSampleLight_OffWaitTime = MIN( zllSampleLight_OffWaitTime, pCmd->offWaitTime );
  }
  else
  {
    uint16 maxOnTime = MAX( zllSampleLight_OnTime, pCmd->onTime );
    zllSampleLight_OnOffCB( COMMAND_ON );
    zllSampleLight_OnTime = maxOnTime;
    zllSampleLight_OffWaitTime = pCmd->offWaitTime;
  }

  if ( ( zllSampleLight_OnTime < 0xFFFF ) && ( zllSampleLight_OffWaitTime < 0xFFFF ) )
  {
    osal_start_timerEx( zllSampleLight_TaskID, SAMPLELIGHT_ON_TIMED_OFF_TIMER_EVT, 100 );
  }
}

#define COLOR_SCN_X_Y_ATTRS_SIZE     ( sizeof(zclColor_CurrentX) + sizeof(zclColor_CurrentY) )
#define COLOR_SCN_HUE_SAT_ATTRS_SIZE ( sizeof(zclColor_EnhancedCurrentHue) + sizeof(zclColor_CurrentSaturation) )
#define COLOR_SCN_LOOP_ATTRS_SIZE    ( sizeof(zclColor_ColorLoopActive) + sizeof(zclColor_ColorLoopDirection) + sizeof(zclColor_ColorLoopTime) )
/*********************************************************************
 * @fn      zllSampleLight_SceneStoreCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received a Scene Store Request Command for
 *          this application.
 *          Stores current attributes in the scene's extension fields.
 *          Extension field sets =
 *          {{Cluster ID 1, length 1, {extension field set 1}}, {{Cluster ID 2,
 *            length 2, {extension field set 2}}, ...}
 *
 * @param   pReq - pointer to a request holding scene data
 *
 * @return  TRUE if extField is filled out, FALSE if not filled
 *          and there is no need to save the scene
 */
static uint8 zllSampleLight_SceneStoreCB( zclSceneReq_t *pReq )
{
  uint8 *pExt;

  pReq->scene->extLen = SAMPLELIGHT_SCENE_EXT_FIELD_SIZE;
  pExt = pReq->scene->extField;

  // Build an extension field for On/Off cluster
  *pExt++ = LO_UINT16( ZCL_CLUSTER_ID_GEN_ON_OFF );
  *pExt++ = HI_UINT16( ZCL_CLUSTER_ID_GEN_ON_OFF );
  *pExt++ = sizeof(zllSampleLight_OnOff); // length

  // Store the value of onOff attribute
  *pExt++ = zllSampleLight_OnOff;

#ifdef ZCL_LEVEL_CTRL
  // Build an extension field for Level Control cluster
  *pExt++ = LO_UINT16( ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL );
  *pExt++ = HI_UINT16( ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL );
  *pExt++ = sizeof(zclLevel_CurrentLevel); // length

  // Store the value of currentLevel attribute
  *pExt++ = zclLevel_CurrentLevel;
#endif //ZCL_LEVEL_CTRL

#ifdef ZCL_COLOR_CTRL
  // Build an extension field for Color Control cluster
  *pExt++ = LO_UINT16( ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL );
  *pExt++ = HI_UINT16( ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL );
  *pExt++ = COLOR_SCN_X_Y_ATTRS_SIZE + COLOR_SCN_HUE_SAT_ATTRS_SIZE + COLOR_SCN_LOOP_ATTRS_SIZE; // length

  // Restored color mode is determined by whether stored currentX, currentY values are 0.
  if ( zclColor_ColorMode == COLOR_MODE_CURRENT_X_Y )
  {
    *pExt++ = LO_UINT16( zclColor_CurrentX );
    *pExt++ = HI_UINT16( zclColor_CurrentX );
    *pExt++ = LO_UINT16( zclColor_CurrentY );
    *pExt++ = HI_UINT16( zclColor_CurrentY );
    pExt += COLOR_SCN_HUE_SAT_ATTRS_SIZE + COLOR_SCN_LOOP_ATTRS_SIZE; // ignore other parameters
  }
  else
  {
    // nullify currentX and currentY to mark hue/sat color mode
    osal_memset( pExt, 0x00, COLOR_SCN_X_Y_ATTRS_SIZE );
    pExt += COLOR_SCN_X_Y_ATTRS_SIZE;
    *pExt++ = LO_UINT16( zclColor_EnhancedCurrentHue );
    *pExt++ = HI_UINT16( zclColor_EnhancedCurrentHue );
    *pExt++ = zclColor_CurrentSaturation;
    *pExt++ = zclColor_ColorLoopActive;
    *pExt++ = zclColor_ColorLoopDirection;
    *pExt++ = LO_UINT16( zclColor_ColorLoopTime );
    *pExt++ = HI_UINT16( zclColor_ColorLoopTime );
  }
#endif //ZCL_COLOR_CTRL

  // Add more clusters here

  return ( TRUE );
}

/*********************************************************************
 * @fn      zllSampleLight_SceneRecallCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received a Scene Recall Request Command for
 *          this application.
 *          Restores attributes values from the scene's extension fields.
 *          Extension field sets =
 *          {{Cluster ID 1, length 1, {extension field set 1}}, {{Cluster ID 2,
 *            length 2, {extension field set 2}}, ...}
 *
 * @param   pReq - pointer to a request holding scene data
 *
 * @return  none
 */
static void zllSampleLight_SceneRecallCB( zclSceneReq_t *pReq )
{
  int8 remain;
  uint16 clusterID;
  uint8 *pExt = pReq->scene->extField;

  while ( pExt < pReq->scene->extField + pReq->scene->extLen )
  {
    clusterID =  BUILD_UINT16( pExt[0], pExt[1] );
    pExt += 2; // cluster ID
    remain = *pExt++;

    if ( clusterID == ZCL_CLUSTER_ID_GEN_ON_OFF )
    {
      // Update onOff attibute with the recalled value
      if ( remain > 0 )
      {
        zllSampleLight_OnOffCB( *pExt++ ); // Apply the new value
        remain--;
      }
    }
#ifdef ZCL_LEVEL_CTRL
    else if ( clusterID == ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL )
    {
      // Update currentLevel attribute with the recalled value
      if ( remain > 0 )
      {
        zclLCMoveToLevel_t levelCmd;

        levelCmd.level = *pExt++;
        levelCmd.transitionTime = pReq->scene->transTime; // whole seconds only
        levelCmd.withOnOff = 0;
        zclLevel_MoveToLevelCB( &levelCmd );
        remain--;
      }
    }
#endif //ZCL_LEVEL_CTRL
#ifdef ZCL_COLOR_CTRL
    else if ( clusterID == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL )
    {
      // Update currentX and currentY attributes with the recalled values
      if ( remain >= COLOR_SCN_X_Y_ATTRS_SIZE )
      {
        zclCCMoveToColor_t colorCmd = {0};

        colorCmd.colorX = BUILD_UINT16( pExt[0], pExt[1] );
        colorCmd.colorY = BUILD_UINT16( pExt[2], pExt[3] );
        pExt += COLOR_SCN_X_Y_ATTRS_SIZE;
        remain -= COLOR_SCN_X_Y_ATTRS_SIZE;
        if ( ( colorCmd.colorX != 0 ) || ( colorCmd.colorY != 0 ) )
        {
          // COLOR_MODE_CURRENT_X_Y
          colorCmd.transitionTime = (10 * pReq->scene->transTime) + pReq->scene->transTime100ms; // in 1/10th seconds
          zclColor_MoveToColorCB( &colorCmd );
          // for non-zero X,Y other hue/sat and loop parameters are ignored (CCB 1683)
        }
        else if ( remain >= COLOR_SCN_HUE_SAT_ATTRS_SIZE )
        {
          // ENHANCED_COLOR_MODE_ENHANCED_CURRENT_HUE_SATURATION
          zclCCEnhancedMoveToHueAndSaturation_t cmd = {0};

          cmd.enhancedHue = BUILD_UINT16( pExt[0], pExt[1] );
          pExt += 2;
          cmd.saturation = *pExt++;
          cmd.transitionTime = (10 * pReq->scene->transTime) + pReq->scene->transTime100ms; // in 1/10th seconds
          zclColor_MoveToEnhHueAndSaturationCB( &cmd );
          remain -= COLOR_SCN_HUE_SAT_ATTRS_SIZE;

          if ( remain >= COLOR_SCN_LOOP_ATTRS_SIZE )
          {
            zclCCColorLoopSet_t newColorLoopSetCmd = {0};
            newColorLoopSetCmd.updateFlags.bits.action = TRUE;
            newColorLoopSetCmd.action = ( (*pExt++) ? LIGHTING_COLOR_LOOP_ACTION_ACTIVATE_FROM_ENH_CURR_HUE : LIGHTING_COLOR_LOOP_ACTION_DEACTIVATE );
            newColorLoopSetCmd.updateFlags.bits.direction = TRUE;
            newColorLoopSetCmd.direction = *pExt++;
            newColorLoopSetCmd.updateFlags.bits.time = TRUE;
            newColorLoopSetCmd.time = BUILD_UINT16( pExt[0], pExt[1] );
            pExt += 2;
            zclColor_SetColorLoopCB( &newColorLoopSetCmd );
            remain -= COLOR_SCN_LOOP_ATTRS_SIZE;
          }
        }
      }
    }
#endif //ZCL_COLOR_CTRL

    // Add more clusters here

    pExt += remain; // remain should be 0 if all extension fields are processed
  }

  zllSampleLight_CurrentScene = pReq->scene->ID;
  zllSampleLight_CurrentGroup = pReq->scene->groupID;
  zllSampleLight_GlobalSceneCtrl = TRUE;
  SCENE_VALID();
}


/****************************************************************************
****************************************************************************/



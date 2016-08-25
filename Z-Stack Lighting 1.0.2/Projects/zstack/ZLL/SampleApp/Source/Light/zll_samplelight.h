/**************************************************************************************************
  Filename:       zll_samplelight.h
  Revised:        $Date: 2013-10-15 13:18:05 -0700 (Tue, 15 Oct 2013) $
  Revision:       $Revision: 35686 $

  Description:    This file contains the Zigbee Cluster Library - Light Link
                  (ZLL) Light Sample Application.


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

#ifndef ZLL_SAMPLELIGHT_H
#define ZLL_SAMPLELIGHT_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ll.h"

#ifndef ZCL_ON_OFF
#error ZCL_ON_OFF should be globally defined to process on/off commands.
#endif
#ifndef ZCL_IDENTIFY
#error ZCL_IDENTIFY should be globally defined to process identify commands.
#endif
#ifndef ZCL_GROUPS
#error ZCL_GROUPS should be globally defined to process group commands.
#endif
#ifndef ZCL_SCENES
#error ZCL_SCENES should be globally defined to process scene commands.
#endif
#ifndef ZCL_LIGHT_LINK_ENHANCE
#error ZCL_LIGHT_LINK_ENHANCE should be globally defined to process light link commands.
#endif

#include "hw_light_ctrl.h"

#ifdef ZCL_LEVEL_CTRL
#include "zcl_level_ctrl.h"
#endif //ZCL_LEVEL_CTRL

#ifdef ZCL_COLOR_CTRL
#include "zcl_color_ctrl.h"
  #ifndef ZCL_LEVEL_CTRL
    #error ZCL_LEVEL_CTRL should be globally defined in color lighting devices.
  #endif
#endif //ZCL_LEVEL_CTRL

/*********************************************************************
 * CONSTANTS
 */
#define SAMPLELIGHT_ENDPOINT            11
#define SAMPLELIGHT_NUM_GRPS            0

#ifdef ZLL_1_0_HUB_COMPATIBILITY
   // Compatibility with Hub operation for ZLL v1.0 is achieved by adding
   // parallel EP with ZLL device ID for ZLL simple descriptor discovery
#define SAMPLELIGHT_ENDPOINT2           12
#endif

#ifndef ZLL_DEVICEID
  #ifdef ZCL_COLOR_CTRL
    #define ZLL_DEVICEID  ZLL_DEVICEID_COLOR_LIGHT
    #define HA_DEVICEID   ZCL_HA_DEVICEID_COLORED_DIMMABLE_LIGHT
  #else
    #ifdef ZCL_LEVEL_CTRL
      #define ZLL_DEVICEID  ZLL_DEVICEID_DIMMABLE_LIGHT
      #define HA_DEVICEID   ZCL_HA_DEVICEID_DIMMABLE_LIGHT
    #else
      #define ZLL_DEVICEID  ZLL_DEVICEID_ON_OFF_LIGHT
      #define HA_DEVICEID  ZCL_HA_DEVICEID_ON_OFF_LIGHT
    #endif
  #endif
#endif

/*
  Number of attributes:
  9 (Basic) + 1 (Identify) + 5 (Scene) + 1 (Group) + 4 (On/Off) = 20,
  + 2 (Level) = 22   for Dimmable Light
  + 15 (Color) = 37  for Color Light
  + 9 (3 LEDs) = 46  for HW reference

  The size of the stored scene extension field(s) is:
   2 + 1 + 1 for On/Off cluster (onOff attibute) = 4
   + 2 + 1 + 1 for Level Control cluster (currentLevel attribute) = 8  for Dimmable Light
   + 2 + 1 + 11 for Color Control cluster (CurrentX/CurrentY/EnhancedCurrentHue/CurrentSaturation/
                colorLoopActive/colorLoopDirection/colorLoopTime attributes) = 22  for Color Light
*/
#ifdef ZCL_COLOR_CTRL
  #ifdef ZLL_HW_LED_LAMP
    #define SAMPLELIGHT_NUM_ATTRIBUTES      46
  #else
    #define SAMPLELIGHT_NUM_ATTRIBUTES      37
  #endif
  #define SAMPLELIGHT_SCENE_EXT_FIELD_SIZE  22
#else
  #ifdef ZCL_LEVEL_CTRL
    #define SAMPLELIGHT_NUM_ATTRIBUTES      22
    #define SAMPLELIGHT_SCENE_EXT_FIELD_SIZE 8
  #else
    #define SAMPLELIGHT_NUM_ATTRIBUTES      20
   #define SAMPLELIGHT_SCENE_EXT_FIELD_SIZE  4
  #endif
#endif

#define LIGHT_OFF                            0x00
#define LIGHT_ON                             0x01

// Application Events
#define SAMPLELIGHT_IDENTIFY_TIMEOUT_EVT     0x0001
#define SAMPLELIGHT_EFFECT_PROCESS_EVT       0x0002
#define SAMPLELIGHT_ON_TIMED_OFF_TIMER_EVT   0x0004
#define SAMPLELIGHT_LEVEL_PROCESS_EVT        0x0008
#define SAMPLELIGHT_COLOR_PROCESS_EVT        0x0010
#define SAMPLELIGHT_COLOR_LOOP_PROCESS_EVT   0x0020
#define SAMPLELIGHT_THERMAL_SAMPLE_EVT       0x0040

/*********************************************************************
 * MACROS
 */
#define SCENE_VALID() zllSampleLight_SceneValid = 1;
#define SCENE_INVALID() zllSampleLight_SceneValid = 0;

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */
extern SimpleDescriptionFormat_t zllSampleLight_SimpleDesc;
extern SimpleDescriptionFormat_t zllSampleLight_SimpleDesc2;
extern zclLLDeviceInfo_t zllSampleLight_DeviceInfo;

extern CONST zclAttrRec_t zllSampleLight_Attrs[];

extern uint8  zllSampleLight_OnOff;
extern uint8  zllSampleLight_GlobalSceneCtrl;
extern uint16 zllSampleLight_OnTime;
extern uint16 zllSampleLight_OffWaitTime;
extern zclGeneral_Scene_t  zllSampleLight_GlobalScene;

extern uint16 zllSampleLight_IdentifyTime;

// Scene Cluster (server) -----------------------------------------------------
extern uint8 zllSampleLight_CurrentScene;
extern uint16 zllSampleLight_CurrentGroup;
extern uint8 zllSampleLight_SceneValid;

/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void zllSampleLight_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 zllSampleLight_event_loop( byte task_id, UINT16 events );


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZLL_SAMPLELIGHT_H */

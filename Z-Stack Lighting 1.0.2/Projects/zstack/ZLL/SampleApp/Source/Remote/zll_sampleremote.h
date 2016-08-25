/**************************************************************************************************
  Filename:       zll_sampleremote.h
  Revised:        $Date: 2013-10-25 17:41:33 -0700 (Fri, 25 Oct 2013) $
  Revision:       $Revision: 35813 $

  Description:    This file contains the Zigbee Cluster Library - Light Link
                  (ZLL) Remote Sample Application.


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

#ifndef ZLL_SAMPLEREMOTE_H
#define ZLL_SAMPLEREMOTE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "zcl.h"

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
#warning ZCL_SCENES should be globally defined to process scene commands.
#endif
#ifndef ZCL_LIGHT_LINK_ENHANCE
#error ZCL_LIGHT_LINK_ENHANCE should be globally defined to process light link commands.
#endif
#ifdef ZCL_COLOR_CTRL
  #ifndef ZCL_LEVEL_CTRL
    #error ZCL_LEVEL_CTRL should be globally defined in color lighting devices.
  #endif
#endif //ZCL_LEVEL_CTRL

#ifdef ZLL_TESTAPP
#include "zll_testapp.h"
#endif

/*********************************************************************
 * CONSTANTS
 */
#define SAMPLEREMOTE_ENDPOINT                11
#define SAMPLEREMOTE_NUM_GRPS                0
#define SAMPLEREMOTE_MAX_ATTRIBUTES          10

#define MAX_LINKED_TARGETS                   10
#define MAX_LINKED_GROUPS                    3
#define ZCD_NV_ZLL_REMOTE_LINK_TARGETS       0x0401
#define ZCD_NV_ZLL_REMOTE_CTRL_GROUPS        0x0402

#define LIGHT_OFF                            0x00
#define LIGHT_ON                             0x01

#ifndef ZLL_DEVICEID
  #ifdef ZCL_COLOR_CTRL
    #ifdef ZCL_SCENES
      #define ZLL_DEVICEID  ZLL_DEVICEID_COLOR_SCENE_CONTROLLER
    #else
      #define ZLL_DEVICEID  ZLL_DEVICEID_COLOR_CONTORLLER
    #endif
    #define HA_DEVICEID   ZCL_HA_DEVICEID_COLOR_DIMMER_SWITCH
  #else
    #ifdef ZCL_SCENES
      #define ZLL_DEVICEID  ZLL_DEVICEID_NON_COLOR_SCENE_CONTROLLER
    #else
      #define ZLL_DEVICEID  ZLL_DEVICEID_NON_COLOR_CONTORLLER
    #endif
    #ifdef ZCL_LEVEL_CTRL
      #define HA_DEVICEID   ZCL_HA_DEVICEID_DIMMER_SWITCH
    #else
      #define HA_DEVICEID  ZCL_HA_DEVICEID_ON_OFF_LIGHT_SWITCH
    #endif
  #endif
#endif

// ZLL Commissioning Utility commands initiation enablement
#define ZLL_UTILITY_SEND_EPINFO_ENABLED
#define ZLL_UTILITY_SEND_GETEPLIST_ENABLED
#define ZLL_UTILITY_SEND_GETGRPIDS_ENABLED

#define SAMPLEREMOTE_CMD_IDENTIFY_TIME     1  //seconds
#define SAMPLEREMOTE_CMD_TRANS_TIME        10 //deciseconds
#define SAMPLEREMOTE_CMD_LEVEL_STEP        25
#define SAMPLEREMOTE_CMD_SAT_STEP          10
#define SAMPLEREMOTE_CMD_HUE_STEP          10
#define SAMPLEREMOTE_CMD_LEVEL_MOVE_RATE   20 //per second
#define SAMPLEREMOTE_CMD_HUE_MOVE_RATE     20
#define SAMPLEREMOTE_CMD_SAT_MOVE_RATE     20


/*********************************************************************
 * MACROS
 */
/*********************************************************************
 * TYPEDEFS
 */
typedef struct
{
  uint16 Addr;  // linked target's short address
  uint16 profileID;
  uint16 deviceID;
  uint8 deviceVersion;
  uint8 EP;     // linked target's end-point
} zllRemoteLinkedTarget_t;

typedef struct
{
  zllRemoteLinkedTarget_t arr[MAX_LINKED_TARGETS];
} zllRemoteLinkedTargetList_t;

typedef struct
{
  uint16 arr[MAX_LINKED_GROUPS];
} zllRemoteControlledGroupsList_t;

/*********************************************************************
 * VARIABLES
 */
extern SimpleDescriptionFormat_t zllSampleRemote_SimpleDesc;

extern zclLLDeviceInfo_t zllSampleRemote_DeviceInfo;

extern CONST zclAttrRec_t zllSampleRemote_Attrs[];

extern uint16 zllSampleRemote_IdentifyTime;

/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void zllSampleRemote_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 zllSampleRemote_event_loop( byte task_id, UINT16 events );


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZLL_SAMPLEREMOTE_H */

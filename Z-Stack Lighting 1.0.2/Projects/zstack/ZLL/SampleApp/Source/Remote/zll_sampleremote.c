/**************************************************************************************************
  Filename:       zll_sampleremote.c
  Revised:        $Date: 2013-12-11 12:59:46 -0800 (Wed, 11 Dec 2013) $
  Revision:       $Revision: 36555 $


  Description:    Zigbee Cluster Library - Light Link (ZLL) Remote Sample
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
  PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
  This device will be like a Controller device.  This application is not
  intended to be a Controller device, but will use the device description
  to implement this sample code.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#if defined ( NV_RESTORE )
#include "OSAL_Nv.h"
#endif
#include "OSAL_Clock.h"

#include "ZDApp.h"

#if defined ( INTER_PAN )
#include "stub_aps.h"
#endif

#include "zcl_lighting.h"
#include "zll_initiator.h"

#if defined ( MT_APP_FUNC )
#include "MT_APP.h"
#include "zll_rpc.h"
#endif

#include "zll_sampleremote.h"

#include "onboard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_adc.h"

#if ((defined HAL_BOARD_CC2530ARC) || (defined HAL_BOARD_CC2530ZLLRC))
#include "hal_buzzer.h"
#define BUZZER_FEEDBACK
#endif

#if (defined HAL_BOARD_CC2530ZLLRC)
#define AUTO_SEND_ADD_GROUP_AFTER_TL
#define SAMPLEREMOTE_UNIQUE_GROUP
#endif

#if (defined HAL_BOARD_CC2530ARC)
#define SAMPLE_REMOTE_VDD_MIN HalAdcCheckVdd(VDD_MIN_XNV)
#else
#define SAMPLE_REMOTE_VDD_MIN HalAdcCheckVdd(VDD_MIN_NV)
#endif

/*********************************************************************
 * MACROS
 */
#if defined (HAL_BOARD_CC2530ARC)
  #if defined (HAL_BOARD_CC2530ARC_REV_B)
    #error ARC Rev B is not supported, use rev C
  #else // ZLight ARC
    #define TOUCH_LINK_KEY         0x20
    #define FACTORY_RESET_KEY      0x10
    #define SEND_FACTORY_RESET_KEY 0x21
    #define LEVEL_UP_KEY           0x27
    #define LEVEL_DN_KEY           0x26
    #define ON_KEY                 0x07
    #define OFF_KEY                0x06
    #define GROUP_ADD_KEY          0x28
    #define GROUP_REMOVE_KEY       0
    #define DEV_SEL_UP_KEY         0x22
    #define DEV_SEL_DN_KEY         0x02
    #define GROUP_1_SEL_KEY        0x0e
    #define GROUP_2_SEL_KEY        0x1e
    #define GROUP_3_SEL_KEY        0x2e
    #define SCENE_STORE_KEY        0x08
    #define SCENE_RECALL_KEY       0x18
    #define SCENE_1_SEL_KEY        0x0d
    #define SCENE_2_SEL_KEY        0x1d
    #define SCENE_3_SEL_KEY        0x2d
    #define HUE_UP_KEY             0x16
    #define HUE_DN_KEY             0x03
    #define SAT_UP_KEY             0x23
    #define SAT_DN_KEY             0x12
    #define HUE_RED_KEY            0x0c
    #define HUE_GREEN_KEY          0x1c
    #define HUE_YELL_KEY           0x1b
    #define HUE_BLUE_KEY           0x2b
    #define CH_CHANGE_KEY          0
    #define CLASSIC_COMMISS_KEY    0x25
    #define HA_BIND_KEY            0x05
    #define RELEASE_KEY            HAL_KEY_CODE_NOKEY
  #endif //(HAL_BOARD_CC2530ARC_REV_B)

#elif defined (HAL_BOARD_CC2530ZLLRC)
  /*
  Col
            1 (0x4)     2 (0x2)     3 (0x1)
  Row
  1 (0x08)  0x0C        0x0A        0x09
  2 (0x10)  0x14        0x12        0x11
  3 (0x20)  0x24        0x22        0x21
  4 (0x40)  0x44        0x42        0x41
  5 (0x80)  0x84        0x82        0x81
  */

  #define TOUCH_LINK_KEY            0x0d // on+off : TouchLink
  #define FACTORY_RESET_KEY         0x0a // Reset To Factory New
  #define SEND_FACTORY_RESET_KEY    0 // Send Reset To Factory New
  #define LEVEL_UP_KEY              0x14 // LevelUp
  #define LEVEL_DN_KEY              0x24 // LevelDown
  #define ON_KEY                    0x0C // On
  #define OFF_KEY                   0x09 // Off
  #define GROUP_ADD_KEY             0x42 // AddGroup
  #define GROUP_REMOVE_KEY          0x4B // Add+off : Remove Group
  #define DEV_SEL_UP_KEY            0x41 // Device Select Next
  #define DEV_SEL_DN_KEY            0x44 // Device Select Previous
  #define GROUP_1_SEL_KEY           0 // Group 1 Select
  #define GROUP_2_SEL_KEY           0 // Group 2 Select
  #define GROUP_3_SEL_KEY           0 // Group 3 Select
  #define SCENE_STORE_KEY           0 // Scene Store
  #define SCENE_RECALL_KEY          0 // Scene Recall
  #define SCENE_1_SEL_KEY           0x84 // Scene 1 Select
  #define SCENE_2_SEL_KEY           0x82 // Scene 2 Select
  #define SCENE_3_SEL_KEY           0x81 // Scene 3 Select
  #define HUE_UP_KEY                0x12 // Hue Up
  #define HUE_DN_KEY                0x22 // Hue Down
  #define SAT_UP_KEY                0x11 // Sat Up
  #define SAT_DN_KEY                0x21 // Sat Down
  #define HUE_RED_KEY               0 // Red Hue
  #define HUE_GREEN_KEY             0 // GREEN Hue
  #define HUE_YELL_KEY              0 // YELLOW Hue
  #define HUE_BLUE_KEY              0 // BLUE Hue
  #define CH_CHANGE_KEY             0 // Change Channel
  #define CLASSIC_COMMISS_KEY       0x85
  #define HA_BIND_KEY               0
  #define RELEASE_KEY               HAL_KEY_CODE_NOKEY

#elif defined (HAL_BOARD_CC2530EB_REV17) //HAL_BOARD_CC2530EB

  #define TOUCH_LINK_KEY      HAL_KEY_LEFT
  #define FACTORY_RESET_KEY   HAL_KEY_RIGHT
  #define SEND_FACTORY_RESET_KEY   0
  #define LEVEL_UP_KEY        (HAL_KEY_UP | HAL_KEY_SW_6)
  #define LEVEL_DN_KEY        (HAL_KEY_DOWN | HAL_KEY_SW_6)
  #define ON_KEY              HAL_KEY_UP
  #define OFF_KEY             HAL_KEY_DOWN
  #define GROUP_ADD_KEY       HAL_KEY_CENTER
  #define GROUP_REMOVE_KEY    0
  #define DEV_SEL_UP_KEY      (HAL_KEY_RIGHT | HAL_KEY_SW_6)
  #define DEV_SEL_DN_KEY      (HAL_KEY_LEFT | HAL_KEY_SW_6)
  #define GROUP_1_SEL_KEY     0
  #define GROUP_2_SEL_KEY     0
  #define GROUP_3_SEL_KEY     0
  #define SCENE_STORE_KEY     0
  #define SCENE_RECALL_KEY    0
  #define SCENE_1_SEL_KEY     0
  #define SCENE_2_SEL_KEY     0
  #define SCENE_3_SEL_KEY     0
  #define HUE_UP_KEY          0
  #define HUE_DN_KEY          0
  #define SAT_UP_KEY          0
  #define SAT_DN_KEY          0
  #define HUE_RED_KEY         0
  #define HUE_GREEN_KEY       0
  #define HUE_YELL_KEY        0
  #define HUE_BLUE_KEY        0
  #define CH_CHANGE_KEY       0
  #define CLASSIC_COMMISS_KEY (HAL_KEY_CENTER | HAL_KEY_SW_6)
  #define HA_BIND_KEY         0
  #define RELEASE_KEY         0

#else
  #error Invalid platform definition
#endif //HAL_BOARD_CC2530ARC

#define RED_HUE             0x02
#define GREEN_HUE           0x55
#define BLUE_HUE            0x98
#define YELL_HUE            0x2F


/*********************************************************************
 * CONSTANTS
 */

// Application Events
#define SAMPLEREMOTE_IDENTIFY_TIMEOUT_EVT    0x0001
#define SAMPLEREMOTE_SEND_GRP_ADD_EVT        0x0002
#define SAMPLEREMOTE_GRP_REMOVE_EVT          0x0004

#define MOVE_FLAG_LEVEL     0x1
#define MOVE_FLAG_HUE       0x2
#define MOVE_FLAG_SAT       0x4

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zllSampleRemote_TaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */
extern void zll_ItemInit( uint16 id, uint16 len, void *pBuf );

/*********************************************************************
 * LOCAL VARIABLES
 */
static afAddrType_t zllSampleRemote_DstAddr;
static zllRemoteLinkedTargetList_t linkedTargets;
static uint8 linkedAddrNextIdx = 0;
static uint8 linkedAddrSelIdx = 0;
static uint8 linkedAddrNum = 0;
static zllRemoteControlledGroupsList_t controlledGroups;
static uint16 groupSelId = 0;
static uint8 sceneSelId = 1;
static uint8 sampleRemoteSeqNum = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
#if defined ( MT_APP_FUNC )
static void zllSampleRemote_ProcessAppMsg( uint8 srcEP, uint8 len, uint8 *msg );
#endif
static void zllSampleRemote_HandleKeys( byte shift, byte keys );
static ZStatus_t zllSampleRemote_DeviceSelect( bool next );
static void zllSampleRemote_BasicResetCB( void );
static void zllSampleRemote_IdentifyCB( zclIdentify_t *pCmd );
static void zllSampleRemote_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void zllSampleRemote_GroupRspCB( zclGroupRsp_t *pRsp );
static void zllSampleRemote_ProcessIdentifyTimeChange( void );

// Functions to process ZCL Foundation incoming Command/Response messages
static void zllSampleRemote_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
static ZStatus_t zllSampleRemote_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif

// This callback is called to process a Get Group Identifiers Request command
static ZStatus_t zllSampleRemote_GetGrpIDsReqCB( afAddrType_t *srcAddr, zclLLGetGrpIDsReq_t *pReq, uint8 seqNum );

// This callback is called to process a Get Endpoint List Request command
static ZStatus_t zllSampleRemote_GetEPListReqCB( afAddrType_t *srcAddr, zclLLGetEPListReq_t *pReq, uint8 seqNum );

// This callback is called to process a Get Endpoint Info Request command
static ZStatus_t zllSampleRemote_GetEndpointInfoCB( afAddrType_t *srcAddr, zclLLEndpointInfo_t *pReq );

// This callback is called to process a Get Group Identifiers Response command
static ZStatus_t zllSampleRemote_GetGrpIDsRspCB( afAddrType_t *srcAddr, zclLLGetGrpIDsRsp_t *pRsp );

// This callback is called to process a Get Endpoint List Response command
static ZStatus_t zllSampleRemote_GetEPListRspCB( afAddrType_t *srcAddr, zclLLGetEPListRsp_t *pRsp );

// Target touch linked notification
static ZStatus_t zllSampleRemote_ProcessTL( epInfoRec_t *pRec );
static ZStatus_t zllSampleRemote_UpdateLinkedTarget( epInfoRec_t *pRec );
static void zllSampleRemote_InitLinkedTargets( void );
static uint8 zllSampleRemote_addControlledGroup( uint16 groupId );

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zllSampleRemote_GenCmdCBs =
{
  zllSampleRemote_BasicResetCB,           // Basic Cluster Reset command
  zllSampleRemote_IdentifyCB,             // Identify command
#ifdef ZCL_EZMODE
  NULL,                                   // Identify EZ-Mode Invoke command
  NULL,                                   // Identify Update Commission State command
#endif
  NULL,                                   // Identify Trigger Effect command
  zllSampleRemote_IdentifyQueryRspCB,     // Identify Query Response command
  NULL,                                   // On/Off cluster commands
  NULL,                                   // On/Off cluster enhanced command Off with Effect
  NULL,                                   // On/Off cluster enhanced command On with Recall Global Scene
  NULL,                                   // On/Off cluster enhanced command On with Timed Off
#ifdef ZCL_LEVEL_CTRL
  NULL,                                   // Level Control Move to Level command
  NULL,                                   // Level Control Move command
  NULL,                                   // Level Control Step command
  NULL,                                   // Level Control Stop command
#endif
#ifdef ZCL_GROUPS
  zllSampleRemote_GroupRspCB,             // Group Response commands
#endif
#ifdef ZCL_SCENES
  NULL,                                   // Scene Store Request command
  NULL,                                   // Scene Recall Request command
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

// ZLL Command Callbacks table
static zclLL_AppCallbacks_t zllSampleRemote_LLCmdCBs =
{
  // Received Server Commands
  zllSampleRemote_GetGrpIDsReqCB,         // Get Group Identifiers Request command
  zllSampleRemote_GetEPListReqCB,         // Get Endpoint List Request command

  // Received Client Commands
  zllSampleRemote_GetEndpointInfoCB,      // Endpoint Information command
  zllSampleRemote_GetGrpIDsRspCB,         // Get Group Identifiers Response command
  zllSampleRemote_GetEPListRspCB          // Get Endpoint List Response command
};

/*********************************************************************
 * @fn          zllSampleRemote_Init
 *
 * @brief       Initialization function for the Sample Remote App task.
 *
 * @param       task_id
 *
 * @return      none
 */
void zllSampleRemote_Init( byte task_id )
{
  zllSampleRemote_TaskID = task_id;

  // Set destination address to indirect
  zllSampleRemote_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  zllSampleRemote_DstAddr.endPoint = 0;
  zllSampleRemote_DstAddr.addr.shortAddr = 0;

  // init linked list in NV
  zllSampleRemote_InitLinkedTargets();

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_GenCmdCBs );

  // Register for ZCL Light Link Cluster Library callback functions
  zclLL_RegisterCmdCallbacks( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_LLCmdCBs );

  // Register the application's attribute list
  zcl_registerAttrList( SAMPLEREMOTE_ENDPOINT, SAMPLEREMOTE_MAX_ATTRIBUTES, zllSampleRemote_Attrs );

  // Register the Application to receive the unprocessed Foundation command/response messages
  zllInitiator_RegisterForMsg( zllSampleRemote_TaskID );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( zllSampleRemote_TaskID );

  zllInitiator_RegisterApp( &zllSampleRemote_SimpleDesc, &zllSampleRemote_DeviceInfo );

  zllInitiator_RegisterIdentifyCB( zllSampleRemote_IdentifyCB );

  zllInitiator_RegisterNotifyTLCB( zllSampleRemote_ProcessTL );

  zllInitiator_RegisterResetAppCB ( zllSampleRemote_BasicResetCB );

  zllInitiator_InitDevice();

#ifdef BUZZER_FEEDBACK
  HalBuzzerRing(200,NULL);
#endif //BUZZER_FEEDBACK
}

/*********************************************************************
 * @fn          zclSample_event_loop
 *
 * @brief       Event Loop Processor for the Sample Remote App task.
 *
 * @param       task_id
 * @param       events - events bitmap
 *
 * @return      unprocessed events bitmap
 */
uint16 zllSampleRemote_event_loop( uint8 task_id, uint16 events )
{
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    afIncomingMSGPacket_t *pMsg;

    if ( (pMsg = (afIncomingMSGPacket_t *)osal_msg_receive( zllSampleRemote_TaskID )) )
    {
      switch ( pMsg->hdr.event )
      {
#if defined (MT_APP_FUNC)
        case MT_SYS_APP_MSG:
          // Message received from MT
         zllSampleRemote_ProcessAppMsg( ((mtSysAppMsg_t *)pMsg)->endpoint,
                                        ((mtSysAppMsg_t *)pMsg)->appDataLen,
                                        ((mtSysAppMsg_t *)pMsg)->appData );
          break;
#endif
        case ZCL_INCOMING_MSG:
          // Incoming ZCL Foundation command/response messages
          zllSampleRemote_ProcessIncomingMsg( (zclIncomingMsg_t *)pMsg );
          break;

        case KEY_CHANGE:
          zllSampleRemote_HandleKeys( ((keyChange_t *)pMsg)->state, ((keyChange_t *)pMsg)->keys );
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

  if ( events & SAMPLEREMOTE_IDENTIFY_TIMEOUT_EVT )
  {
    if ( zllSampleRemote_IdentifyTime > 0 )
      zllSampleRemote_IdentifyTime--;
    zllSampleRemote_ProcessIdentifyTimeChange();

    return ( events ^ SAMPLEREMOTE_IDENTIFY_TIMEOUT_EVT );
  }

#if defined ZCL_GROUPS
  if ( events & SAMPLEREMOTE_SEND_GRP_ADD_EVT )
  {
    uint8 emptyGroupName[2] = {0};
#ifdef SAMPLEREMOTE_UNIQUE_GROUP
    groupSelId = controlledGroups.arr[0];
#endif
    if ( groupSelId == 0 )
    {
      groupSelId = 1;
    }
    zclGeneral_SendGroupAdd( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr, groupSelId, emptyGroupName,
                             FALSE, sampleRemoteSeqNum++ );

    return ( events ^ SAMPLEREMOTE_SEND_GRP_ADD_EVT );
  }

  if ( events & SAMPLEREMOTE_GRP_REMOVE_EVT )
  {
#ifdef BUZZER_FEEDBACK
    HalBuzzerRing( 200, NULL );
#endif //BUZZER_FEEDBACK
    return ( events ^ SAMPLEREMOTE_GRP_REMOVE_EVT );
  }
#endif //ZCL_GROUPS

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      zllSampleRemote_ProcessTL
 *
 * @brief   Process notification data of target being touch-linked:
 *          - Add target to linked targets list
 *          - Provide user notification
 *          - initiate post-TL procedures (send EP Info to end-device)
 *
 * @param   pRec - Target's enpoint information record
 *
 * @return  status
 */
static ZStatus_t zllSampleRemote_ProcessTL( epInfoRec_t *pRec )
{
  zllSampleRemote_UpdateLinkedTarget( pRec );

#if ( HAL_LED == TRUE )
  // Light LED
  HalLedSet( HAL_LED_2, HAL_LED_MODE_ON );
  HalLedSet( HAL_LED_1, HAL_LED_MODE_OFF );
#endif

#ifdef BUZZER_FEEDBACK
  HalBuzzerRing( 400, NULL );
#endif //BUZZER_FEEDBACK

  //check if this is a light.
  if( pRec->deviceID < ZLL_DEVICEID_COLOR_CONTORLLER )
  {
    HalLcdWriteStringValue( "TL Light:", pRec->nwkAddr, 16, HAL_LCD_LINE_3 );
#ifdef AUTO_SEND_ADD_GROUP_AFTER_TL
    osal_start_timerEx( zllSampleRemote_TaskID, SAMPLEREMOTE_SEND_GRP_ADD_EVT,
                        ( _NIB.nwkState == NWK_ENDDEVICE ? 1000 : 3000 ) );
#endif //AUTO_SEND_ADD_GROUP_AFTER_TL
  }
  else
  {
    HalLcdWriteStringValue( "TL Ctrlr:", pRec->nwkAddr, 16, HAL_LCD_LINE_3 );

#ifdef ZLL_UTILITY_SEND_EPINFO_ENABLED
    zllInitiator_SendEPInfo( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr, sampleRemoteSeqNum++ );
#endif //ZLL_UTILITY_SEND_EPINFO_ENABLED
  }

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      zllSampleRemote_UpdateLinkedTarget
 *
 * @brief   Add or update target in linked targets list
 *
 * @param   pRec - Target's enpoint information record
 *
 * @return  status
 */
static ZStatus_t zllSampleRemote_UpdateLinkedTarget( epInfoRec_t *pRec )
{
  uint8 idx;
  ZStatus_t status;
  for ( idx = 0; idx < MAX_LINKED_TARGETS; idx++ )
  {
    if ( ( linkedTargets.arr[idx].Addr == pRec->nwkAddr )
        && ( linkedTargets.arr[idx].EP == pRec->endpoint ) )
    {
      break; // found existing entry, overwrite.
    }
  }
  //this target is not in our records
  if ( idx == MAX_LINKED_TARGETS )
  {
    idx = linkedAddrNextIdx;
    linkedAddrNextIdx++;
    if( linkedAddrNextIdx > (MAX_LINKED_TARGETS-1) )
    {
      //wrap around and overwrite previous address
      linkedAddrNextIdx = 0;
    }
    if ( linkedAddrNum < MAX_LINKED_TARGETS )
    {
      linkedAddrNum++;
    }
  }
  linkedTargets.arr[idx].Addr = pRec->nwkAddr;
  linkedTargets.arr[idx].profileID = pRec->profileID;
  linkedTargets.arr[idx].deviceID = pRec->deviceID;
  linkedTargets.arr[idx].deviceVersion = pRec->version;
  linkedTargets.arr[idx].EP = pRec->endpoint;
  linkedAddrSelIdx = idx;

  // update linkedAddr in NV
#if defined ( NV_RESTORE )
  status = osal_nv_write( ZCD_NV_ZLL_REMOTE_LINK_TARGETS, 0, sizeof( linkedTargets ), &linkedTargets );
#endif

  zllSampleRemote_DstAddr.endPoint = pRec->endpoint;
  zllSampleRemote_DstAddr.addrMode = afAddr16Bit;
  zllSampleRemote_DstAddr.addr.shortAddr = pRec->nwkAddr;
#if defined( INTER_PAN )
  zllSampleRemote_DstAddr.panId = _NIB.nwkPanId;
#endif

  return status;
}

/*********************************************************************
 * @fn      zllSampleRemote_InitLinkedTargets()
 *
 * @brief   Initialize linked targets and controlled groups lists in NV.
 *
 * @param   none
 *
 * @return  none
 */
static void zllSampleRemote_InitLinkedTargets( void )
{
  uint8 i;
  osal_memset( &linkedTargets, 0xFF, sizeof(linkedTargets));
  zll_ItemInit( ZCD_NV_ZLL_REMOTE_LINK_TARGETS, sizeof(zllRemoteLinkedTargetList_t), &linkedTargets );
  linkedAddrNum = 0;
  for (i = 0; i < MAX_LINKED_TARGETS; i++ )
  {
    if ( linkedTargets.arr[i].Addr != 0xFFFF )
    {
      linkedAddrNum++;
    }
  }
  if ( linkedAddrNum < MAX_LINKED_TARGETS )
  {
    linkedAddrNextIdx = linkedAddrNum;
  }

  // init controlled groups list
  osal_memset( &controlledGroups, 0x00, sizeof(controlledGroups));
  zll_ItemInit( ZCD_NV_ZLL_REMOTE_CTRL_GROUPS, sizeof(zllRemoteControlledGroupsList_t), &controlledGroups );

#ifdef SAMPLEREMOTE_UNIQUE_GROUP
  // set first group as unique
  if ( controlledGroups.arr[0] == 0 )
  {
    do { controlledGroups.arr[0] = osal_rand(); }
    while ( controlledGroups.arr[0] <= 1 );
    osal_nv_write( ZCD_NV_ZLL_REMOTE_CTRL_GROUPS, 0, sizeof( controlledGroups ), &controlledGroups );
  }
#endif

  groupSelId = controlledGroups.arr[0];
}

#if defined ( MT_APP_FUNC )
/*********************************************************************
 * @fn      zllSampleRemote_ProcessAppMsg
 *
 * @brief   Process Test messages
 *
 * @param   srcEP - Sending Apps endpoint
 * @param   len - number of bytes
 * @param   msg - pointer to message
 *          0 - lo byte destination address
 *          1 - hi byte destination address
 *          2 - destination endpoint
 *          3 - lo byte cluster ID
 *          4 - hi byte cluster ID
 *          5 - message length
 *          6 - destination address mode (first byte of data)
 *          7 - zcl command frame
 *
 * @return  none
 */
static void zllSampleRemote_ProcessAppMsg( uint8 srcEP, uint8 len, uint8 *msg )
{
  afAddrType_t dstAddr;
  uint16 clusterID;
  zclFrameHdr_t hdr;
  uint8 *pData;
  uint8 dataLen;

#if defined( INTER_PAN )
  dstAddr.panId = _NIB.nwkPanId;
#endif

  dstAddr.addr.shortAddr = BUILD_UINT16( msg[0], msg[1] );
  msg += 2;
  dstAddr.endPoint = *msg++;
  clusterID = BUILD_UINT16( msg[0], msg[1] );
  msg += 2;
  dataLen = *msg++; // Length of message (Z-Tool can support up to 255 octets)
  dstAddr.addrMode = (afAddrMode_t)(*msg++);
  dataLen--; // Length of ZCL frame

  // Begining of ZCL frame
  pData = zclParseHdr( &hdr, msg );
  dataLen -= (uint8)( pData - msg );

#ifdef ZLL_TESTAPP
    zllTestApp_ProcessAppMsg( srcEP, &dstAddr, clusterID, &hdr, dataLen, pData );
#else
    if ( ZCL_CLUSTER_ID_LL( clusterID ) ||
         ZCL_CLUSTER_ID_GEN( clusterID ) ||
         ZCL_CLUSTER_ID_LIGHTING( clusterID ) )
    {
      // send MT_APP ZCL command as-is
      zcl_SendCommand( srcEP, &dstAddr, clusterID, hdr.commandID,
                       (hdr.fc.type == ZCL_FRAME_TYPE_SPECIFIC_CMD),
                       hdr.fc.direction, hdr.fc.disableDefaultRsp, hdr.manuCode,
                       hdr.transSeqNum, dataLen, pData );
    }
#endif //ZLL_TESTAPP
}
#endif // MT_APP_FUNC

/*********************************************************************
 * @fn      zllSampleRemote_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events.
 *
 * @return  none
 */
static void zllSampleRemote_HandleKeys( byte shift, byte keys )
{
  static uint8 moveFlag = 0;
  static uint32 sceneKeyPressTime = 0;

  if(shift)
  {
    keys |= HAL_KEY_SW_6;
  }

  // ARC and ZLLRC send HAL_KEY_CODE_NOKEY to signify the key is released
  // If we do not put this check then the key press generates multiple event when
  // key ints are enabled on deep sleeping remotes.
#if ( ((defined HAL_BOARD_CC2530ARC)||(defined HAL_BOARD_CC2530ZLLRC)) && (defined POWER_SAVING) )
  static byte prevKey = 0;
  if( keys ==  prevKey)
  {
    return;
  }
  else
  {
    prevKey = keys;
  }
#endif

  if( keys == RELEASE_KEY)
  {
#if defined ZCL_LEVEL_CTRL
    if( moveFlag & MOVE_FLAG_LEVEL)
    {
      //send level stop
      zclGeneral_SendLevelControlStop( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                        FALSE, sampleRemoteSeqNum++ );
    }
#endif
#if defined ZCL_COLOR_CTRL
    if( moveFlag & MOVE_FLAG_HUE)
    {
      zclLighting_ColorControl_Send_MoveHueCmd( SAMPLEREMOTE_ENDPOINT,
                                  &zllSampleRemote_DstAddr, LIGHTING_MOVE_HUE_STOP,
                                  0, FALSE, sampleRemoteSeqNum++ );
    }
    if( moveFlag & MOVE_FLAG_SAT)
    {
      //send level stop
      zclLighting_ColorControl_Send_MoveSaturationCmd( SAMPLEREMOTE_ENDPOINT,
                                    &zllSampleRemote_DstAddr, LIGHTING_MOVE_SATURATION_STOP,
                                    0, FALSE, sampleRemoteSeqNum++ );
    }
#endif
    moveFlag = 0;
#ifdef ZCL_SCENES
    // Scene conroller with no RECALL and STORE keys may use press time to determine functionality
    if ( sceneKeyPressTime > 0 )
    {
      uint16 timeDiff = ( osal_getClock() - sceneKeyPressTime );
      sceneKeyPressTime = 0;
#ifdef SAMPLEREMOTE_UNIQUE_GROUP
      zllSampleRemote_DstAddr.addr.shortAddr = controlledGroups.arr[0];
      groupSelId = controlledGroups.arr[0];
      zllSampleRemote_DstAddr.addrMode = (afAddrMode_t)AddrGroup;
#endif //SAMPLEREMOTE_UNIQUE_GROUP
      if ( timeDiff < 3 )
      {
        //short press : scene recall
        zclGeneral_SendSceneRecall( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                    groupSelId, sceneSelId, FALSE, sampleRemoteSeqNum++ );
      }
      else
      {
        //long press : scene store
        zclGeneral_SendSceneStore( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                   groupSelId, sceneSelId, FALSE, sampleRemoteSeqNum++ );
#ifdef BUZZER_FEEDBACK
        HalBuzzerRing( 300, NULL );
#endif
        return;
      }
    }
#endif //ZCL_SCENES
    if (!SAMPLE_REMOTE_VDD_MIN)
    {
      return;
    }
  }
  else
  {
    // Rejoin if orphaned, should be done upon exit from deep sleep.
    zllInitiator_OrphanedRejoin();

#if defined ZCL_LEVEL_CTRL
    if ( keys == LEVEL_UP_KEY )
    {
      if ( RELEASE_KEY )
      {
        zclGeneral_SendLevelControlMoveWithOnOff( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                                 LEVEL_MOVE_UP, SAMPLEREMOTE_CMD_LEVEL_MOVE_RATE,
                                                 FALSE, sampleRemoteSeqNum++ );

        //set flag so the stop command is sent on the key release
        moveFlag |= MOVE_FLAG_LEVEL;

      }
      else
      {
        zclGeneral_SendLevelControlStepWithOnOff( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                                 LEVEL_STEP_UP, SAMPLEREMOTE_CMD_LEVEL_STEP,
                                                 SAMPLEREMOTE_CMD_TRANS_TIME, FALSE, sampleRemoteSeqNum++ );
      }
    }

    if ( keys == LEVEL_DN_KEY )
    {
      if ( RELEASE_KEY )
      {
        zclGeneral_SendLevelControlMoveWithOnOff( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                                 LEVEL_MOVE_DOWN, SAMPLEREMOTE_CMD_LEVEL_MOVE_RATE,
                                                 FALSE, sampleRemoteSeqNum++ );

        //set flag so the stop command is sent on the key release
        moveFlag |= MOVE_FLAG_LEVEL;
      }
      else
      {
        zclGeneral_SendLevelControlStepWithOnOff( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                                 LEVEL_STEP_DOWN, SAMPLEREMOTE_CMD_LEVEL_STEP,
                                                 SAMPLEREMOTE_CMD_TRANS_TIME, FALSE, sampleRemoteSeqNum++ );
      }
    }
#endif //ZCL_LEVEL_CTRL

    if ( keys == ON_KEY )
    {
      zclGeneral_SendOnOff_CmdOn( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr, FALSE, sampleRemoteSeqNum++ );
    }

    if ( keys == FACTORY_RESET_KEY )
    {
      zllInitiator_ResetToFactoryNew();
    }

    if ( keys == SEND_FACTORY_RESET_KEY )
    {
      zllInitiator_ResetToFNSelectedTarget();
    }

    if ( keys == OFF_KEY )
    {
      //send on
      zclGeneral_SendOnOff_CmdOff( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr, FALSE, sampleRemoteSeqNum++ );
    }

    if ( keys == TOUCH_LINK_KEY )
    {
      if (!SAMPLE_REMOTE_VDD_MIN)
      {
#if HAL_LCD
        HalLcdWriteString( "Low Batteries", HAL_LCD_LINE_3 );
#endif
        // return with no buzzer feedback to alert the user.
        return;
      }
      zllInitiator_StartDevDisc();
    }

    if ( keys == DEV_SEL_UP_KEY )
    {
      if ( zllSampleRemote_DeviceSelect(TRUE) == ZSuccess )
      {
        HalLcdWriteStringValue( "Dev Sel:", zllSampleRemote_DstAddr.addr.shortAddr, 16, HAL_LCD_LINE_3 );
      }
      else
      {
        HalLcdWriteString( "Dev Sel: no trgt", HAL_LCD_LINE_3 );
      }
    }

    if ( keys == DEV_SEL_DN_KEY )
    {
      if ( zllSampleRemote_DeviceSelect(FALSE) == ZSuccess )
      {
        HalLcdWriteStringValue( "Dev Sel:", zllSampleRemote_DstAddr.addr.shortAddr, 16, HAL_LCD_LINE_3 );
      }
      else
      {
        HalLcdWriteString( "Dev Sel: no trgt", HAL_LCD_LINE_3 );
      }
    }

#ifdef ZCL_GROUPS
    if ( keys == GROUP_ADD_KEY )
    {
      uint8 emptyGroupName[2] = {0};
      if ( groupSelId == 0 )
      {
        groupSelId = 1;
      }
      zclGeneral_SendGroupAdd( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr, groupSelId,
                              emptyGroupName, FALSE, sampleRemoteSeqNum++ );
    }

    if ( keys == GROUP_REMOVE_KEY )
    {
      if ( groupSelId == 0 )
      {
        groupSelId = 1;
      }
      zclGeneral_SendGroupRemove( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                 groupSelId, FALSE, sampleRemoteSeqNum++ );

      if ( zllSampleRemote_DstAddr.addrMode == (afAddrMode_t)AddrGroup )
      {
        //no response is expected when command is sent to a group; create self feedback
#ifdef BUZZER_FEEDBACK
        HalBuzzerRing( 200, NULL );
        osal_start_timerEx( zllSampleRemote_TaskID, SAMPLEREMOTE_GRP_REMOVE_EVT, 400 );
        return;
#endif
      }
    }

    if ( keys == GROUP_1_SEL_KEY )
    {
      zllSampleRemote_DstAddr.addr.shortAddr = 1;
      groupSelId = 1;
      zllSampleRemote_DstAddr.addrMode = (afAddrMode_t)AddrGroup;

      zclGeneral_SendIdentify( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                              SAMPLEREMOTE_CMD_IDENTIFY_TIME, FALSE, sampleRemoteSeqNum++ );
    }

    if ( keys == GROUP_2_SEL_KEY )
    {
      zllSampleRemote_DstAddr.addr.shortAddr = 2;
      groupSelId = 2;
      zllSampleRemote_DstAddr.addrMode = (afAddrMode_t)AddrGroup;

      zclGeneral_SendIdentify( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                              SAMPLEREMOTE_CMD_IDENTIFY_TIME, FALSE, sampleRemoteSeqNum++ );
    }

    if ( keys == GROUP_3_SEL_KEY )
    {
      zllSampleRemote_DstAddr.addr.shortAddr = 3;
      groupSelId = 3;
      zllSampleRemote_DstAddr.addrMode = (afAddrMode_t)AddrGroup;

      zclGeneral_SendIdentify( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                              SAMPLEREMOTE_CMD_IDENTIFY_TIME, FALSE, sampleRemoteSeqNum++ );
    }
#endif //ZCL_GROUPS
#ifdef ZCL_SCENES
    if ( keys == SCENE_1_SEL_KEY )
    {
      sceneSelId = 1;
      if ( ( RELEASE_KEY ) && ( !SCENE_RECALL_KEY ) )
      {
        sceneKeyPressTime = osal_getClock();
      }
    }

    if ( keys == SCENE_2_SEL_KEY )
    {
      sceneSelId = 2;
      if ( ( RELEASE_KEY ) && ( !SCENE_RECALL_KEY ) )
      {
        sceneKeyPressTime = osal_getClock();
      }
    }

    if ( keys == SCENE_3_SEL_KEY )
    {
      sceneSelId = 3;
      if ( ( RELEASE_KEY ) && ( !SCENE_RECALL_KEY ) )
      {
        sceneKeyPressTime = osal_getClock();
      }
    }

    if ( keys == SCENE_STORE_KEY )
    {
      if ( groupSelId == 0 )
      {
        groupSelId = 1;
      }
      zclGeneral_SendSceneStore( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                groupSelId, sceneSelId,
                                FALSE, sampleRemoteSeqNum++ );
    }

    if ( keys == SCENE_RECALL_KEY )
    {
      if ( groupSelId == 0 )
      {
        groupSelId = 1;
      }
      zclGeneral_SendSceneRecall( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                 groupSelId, sceneSelId,
                                 FALSE, sampleRemoteSeqNum++ );
    }
#endif //ZCL_SCENES
#ifdef ZCL_COLOR_CTRL
    if ( keys == HUE_UP_KEY )
    {
      if ( RELEASE_KEY )
      {
        zclLighting_ColorControl_Send_MoveHueCmd( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                                 LIGHTING_MOVE_HUE_UP, SAMPLEREMOTE_CMD_HUE_MOVE_RATE,
                                                 FALSE, sampleRemoteSeqNum++ );

        //set flag so the stop command is sent on the key release
        moveFlag |= MOVE_FLAG_HUE;
      }
      else
      {
        zclLighting_ColorControl_Send_StepHueCmd( SAMPLEREMOTE_ENDPOINT,
                                                 &zllSampleRemote_DstAddr, LIGHTING_STEP_HUE_UP,
                                                 SAMPLEREMOTE_CMD_HUE_STEP, SAMPLEREMOTE_CMD_TRANS_TIME,
                                                 FALSE, sampleRemoteSeqNum++ );
      }
    }

    if ( keys == HUE_DN_KEY )
    {
      if ( RELEASE_KEY )
      {
        zclLighting_ColorControl_Send_MoveHueCmd( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                                 LIGHTING_MOVE_HUE_DOWN, SAMPLEREMOTE_CMD_HUE_MOVE_RATE,
                                                 FALSE, sampleRemoteSeqNum++ );

        //set flag so the stop command is sent on the key release
        moveFlag |= MOVE_FLAG_HUE;

      }
      else
      {
        zclLighting_ColorControl_Send_StepHueCmd( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                                 LIGHTING_STEP_HUE_DOWN, SAMPLEREMOTE_CMD_HUE_STEP,
                                                 SAMPLEREMOTE_CMD_TRANS_TIME, FALSE, sampleRemoteSeqNum++ );
      }
    }

    if ( keys == SAT_UP_KEY )
    {
      if ( RELEASE_KEY )
      {
        zclLighting_ColorControl_Send_MoveSaturationCmd( SAMPLEREMOTE_ENDPOINT,
                                                        &zllSampleRemote_DstAddr, LIGHTING_MOVE_SATURATION_UP,
                                                        SAMPLEREMOTE_CMD_SAT_MOVE_RATE,
                                                        FALSE, sampleRemoteSeqNum++ );

        //set flag so the stop command is sent on the key release
        moveFlag |= MOVE_FLAG_SAT;

      }
      else
      {
        zclLighting_ColorControl_Send_StepSaturationCmd( SAMPLEREMOTE_ENDPOINT,
                                                        &zllSampleRemote_DstAddr, LIGHTING_STEP_HUE_UP,
                                                        SAMPLEREMOTE_CMD_SAT_STEP, SAMPLEREMOTE_CMD_TRANS_TIME,
                                                        FALSE, sampleRemoteSeqNum++ );
      }
    }

    if ( keys == SAT_DN_KEY )
    {
      if ( RELEASE_KEY )
      {
        zclLighting_ColorControl_Send_MoveSaturationCmd( SAMPLEREMOTE_ENDPOINT,
                                                        &zllSampleRemote_DstAddr, LIGHTING_MOVE_SATURATION_DOWN,
                                                        SAMPLEREMOTE_CMD_SAT_MOVE_RATE,
                                                        FALSE, sampleRemoteSeqNum++ );
        //set flag so the stop command is sent on the key release
        moveFlag |= MOVE_FLAG_SAT;
      }
      else
      {
        zclLighting_ColorControl_Send_StepSaturationCmd( SAMPLEREMOTE_ENDPOINT,
                                                        &zllSampleRemote_DstAddr, LIGHTING_STEP_HUE_DOWN,
                                                        SAMPLEREMOTE_CMD_SAT_STEP, SAMPLEREMOTE_CMD_TRANS_TIME,
                                                        FALSE, sampleRemoteSeqNum++ );
      }
    }

    if ( keys == HUE_RED_KEY )
    {
      zclLighting_ColorControl_Send_MoveToHueCmd ( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                                  RED_HUE, LIGHTING_MOVE_TO_HUE_DIRECTION_SHORTEST_DISTANCE,
                                                  SAMPLEREMOTE_CMD_TRANS_TIME, FALSE, sampleRemoteSeqNum++ );
    }

    if ( keys == HUE_GREEN_KEY )
    {
      zclLighting_ColorControl_Send_MoveToHueCmd( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                                 GREEN_HUE, LIGHTING_MOVE_TO_HUE_DIRECTION_SHORTEST_DISTANCE,
                                                 SAMPLEREMOTE_CMD_TRANS_TIME, FALSE, sampleRemoteSeqNum++ );
    }

    if ( keys == HUE_BLUE_KEY )
    {
      zclLighting_ColorControl_Send_MoveToHueCmd( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                                 BLUE_HUE, LIGHTING_MOVE_TO_HUE_DIRECTION_SHORTEST_DISTANCE,
                                                 SAMPLEREMOTE_CMD_TRANS_TIME, FALSE, sampleRemoteSeqNum++ );
    }

    if ( keys == HUE_YELL_KEY )
    {
      zclLighting_ColorControl_Send_MoveToHueCmd( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                                                 YELL_HUE, LIGHTING_MOVE_TO_HUE_DIRECTION_SHORTEST_DISTANCE,
                                                 SAMPLEREMOTE_CMD_TRANS_TIME, FALSE, sampleRemoteSeqNum++ );
    }
#endif //ZCL_COLOR_CTRL

    if ( keys == CH_CHANGE_KEY )
    {
      zllInitiator_ChannelChange( 0 );
    }

    if ( keys == CLASSIC_COMMISS_KEY )
    {
      zllInitiator_ClassicalCommissioningStart();
    }
  }

#ifdef BUZZER_FEEDBACK
  HalBuzzerRing( 1, NULL );
#endif
}

/*********************************************************************
 * @fn          zllSampleRemote_DeviceSelect
 *
 * @brief       Change zllSampleRemote_DstAddr to the next device in the list.
 *              If a group was previously selected, cycle through it as well.
 *
 * @param       next - TRUE to select next, FALSE to select previous
 *
 * @return      Failure if no devices in list
 */
static ZStatus_t zllSampleRemote_DeviceSelect( bool next )
{
  if ( linkedAddrNum == 0 )
  {
    return ZFailure;
  }

  if ( next )
  {
    if( (linkedAddrSelIdx+1) >= linkedAddrNum )
    {
      if( (groupSelId == 0) || ( zllSampleRemote_DstAddr.addrMode == (afAddrMode_t)AddrGroup ) )
      {
         linkedAddrSelIdx = 0;

        zllSampleRemote_DstAddr.addrMode = afAddr16Bit;
        zllSampleRemote_DstAddr.addr.shortAddr = linkedTargets.arr[linkedAddrSelIdx].Addr;
        zllSampleRemote_DstAddr.endPoint =  linkedTargets.arr[linkedAddrSelIdx].EP;
      }
      else
      {
        zllSampleRemote_DstAddr.addrMode = (afAddrMode_t)AddrGroup;
        zllSampleRemote_DstAddr.addr.shortAddr = groupSelId;
        zllSampleRemote_DstAddr.endPoint =  AF_BROADCAST_ENDPOINT;
      }
    }
    else
    {
      linkedAddrSelIdx++;

      zllSampleRemote_DstAddr.addrMode = afAddr16Bit;
      zllSampleRemote_DstAddr.addr.shortAddr = linkedTargets.arr[linkedAddrSelIdx].Addr;
      zllSampleRemote_DstAddr.endPoint =  linkedTargets.arr[linkedAddrSelIdx].EP;
    }
  }
  else //previous
  {
    if( linkedAddrSelIdx == 0 )
    {
      if ( ( (groupSelId == 0) || zllSampleRemote_DstAddr.addrMode == (afAddrMode_t)AddrGroup ) )
      {
         linkedAddrSelIdx = linkedAddrNum-1;

        zllSampleRemote_DstAddr.addrMode = afAddr16Bit;
        zllSampleRemote_DstAddr.addr.shortAddr = linkedTargets.arr[linkedAddrSelIdx].Addr;
        zllSampleRemote_DstAddr.endPoint =  linkedTargets.arr[linkedAddrSelIdx].EP;
      }
      else
      {
        zllSampleRemote_DstAddr.addrMode = (afAddrMode_t)AddrGroup;
        zllSampleRemote_DstAddr.addr.shortAddr = groupSelId;
        zllSampleRemote_DstAddr.endPoint =  AF_BROADCAST_ENDPOINT;
      }
    }
    else
    {
      linkedAddrSelIdx--;

      zllSampleRemote_DstAddr.addrMode = afAddr16Bit;
      zllSampleRemote_DstAddr.addr.shortAddr = linkedTargets.arr[linkedAddrSelIdx].Addr;
      zllSampleRemote_DstAddr.endPoint =  linkedTargets.arr[linkedAddrSelIdx].EP;
    }
  }
  zclGeneral_SendIdentify( SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr,
                           SAMPLEREMOTE_CMD_IDENTIFY_TIME, FALSE, sampleRemoteSeqNum++ );

  return ZSuccess;
}

/*********************************************************************
 * @fn      zllSampleRemote_ProcessIdentifyTimeChange
 *
 * @brief   Called to process any change to the IdentifyTime attribute.
 *
 * @param   none
 *
 * @return  none
 */
static void zllSampleRemote_ProcessIdentifyTimeChange( void )
{
  if ( zllSampleRemote_IdentifyTime > 0 )
  {
    osal_start_timerEx( zllSampleRemote_TaskID, SAMPLEREMOTE_IDENTIFY_TIMEOUT_EVT, 1000 );
#if ( HAL_LED == TRUE )
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
#endif
#ifdef BUZZER_FEEDBACK
    HalBuzzerRing( 500, NULL );
#endif //BUZZER_FEEDBACK
  }
  else
  {
#if ( HAL_LED == TRUE )
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
#endif
    osal_stop_timerEx( zllSampleRemote_TaskID, SAMPLEREMOTE_IDENTIFY_TIMEOUT_EVT );
  }
}

/*********************************************************************
 * @fn      zllSampleRemote_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to default values.
 *
 * @param   none
 *
 * @return  none
 */
static void zllSampleRemote_BasicResetCB( void )
{
  // Reset all attributes to default values
  linkedAddrNum = 0;
  linkedAddrNextIdx = 0;
  linkedAddrSelIdx = 0;
  osal_memset( &linkedTargets, 0xFF, sizeof(linkedTargets));
  osal_memset( &controlledGroups, 0x00, sizeof(controlledGroups));
#if defined ( NV_RESTORE )
  osal_nv_write( ZCD_NV_ZLL_REMOTE_LINK_TARGETS, 0, sizeof( linkedTargets ), &linkedTargets );
  osal_nv_write( ZCD_NV_ZLL_REMOTE_CTRL_GROUPS, 0, sizeof( controlledGroups ), &controlledGroups );
#endif
}

/*********************************************************************
 * @fn      zllSampleRemote_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   srcAddr - source address and endpoint of the response message
 * @param   identifyTime - the number of seconds to identify yourself
 *
 * @return  none
 */
static void zllSampleRemote_IdentifyCB( zclIdentify_t *pCmd )
{
  zllSampleRemote_IdentifyTime = pCmd->identifyTime;
  zllSampleRemote_ProcessIdentifyTimeChange();
}

/*********************************************************************
 * @fn      zllSampleRemote_IdentifyQueryRspCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Query Response Command for this application.
 *
 * @param   srcAddr - requestor's address
 * @param   timeout - number of seconds to identify yourself (valid for query response)
 *
 * @return  none
 */
static void zllSampleRemote_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp )
{
  // Query Response (with timeout value)
  (void)pRsp;
}


/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      zllSampleRemote_ProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
static void zllSampleRemote_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg)
{
  switch ( pInMsg->zclHdr.commandID )
  {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      zllSampleRemote_ProcessInReadRspCmd( pInMsg );
      break;
#endif
    default:
      break;
  }

  if ( pInMsg->attrCmd )
    osal_mem_free( pInMsg->attrCmd );
}

#ifdef ZCL_READ
/*********************************************************************
 * @fn      zllSampleRemote_ProcessInReadRspCmd
 *
 * @brief   Process the ZCL Read Attributes Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  status
 */
static ZStatus_t zllSampleRemote_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  // Notify the originator of the results of the original read attributes
  // attempt and, for each successfull request, the value of the requested
  // attribute

#if defined ( MT_APP_FUNC )
  uint8 i, *msg, *pBuf, dataLength;
  uint8 len = 0;
  zclReadRspCmd_t *pReadRsp = (zclReadRspCmd_t *)(pInMsg->attrCmd);

  for (i = 0; i < pReadRsp->numAttr; i++)
  {
    if ( pReadRsp->attrList[i].status == ZSuccess )
    {
      dataLength = zclGetAttrDataLength( pReadRsp->attrList[i].dataType, pReadRsp->attrList[i].data );
    }

    // prepend srcAddr, endpoint, cluster ID, and data length
    len += sizeof ( uint16 )     // attribute ID
        + sizeof ( uint8 )      // attribute read status
        + sizeof ( uint8 )      // attribute data type
        + dataLength;
  }

  msg = osal_mem_alloc( len );
  if ( msg != NULL )
  {
    pBuf = msg;

    for (i = 0; i < pReadRsp->numAttr; i++)
    {
      // Attribute information
      *pBuf++ = LO_UINT16( pReadRsp->attrList[i].attrID );
      *pBuf++ = HI_UINT16( pReadRsp->attrList[i].attrID );
      *pBuf++ = pReadRsp->attrList[i].status;
      *pBuf++ = pReadRsp->attrList[i].dataType;

      if ( pReadRsp->attrList[i].status == ZSuccess )
      {
        dataLength = zclGetAttrDataLength( pReadRsp->attrList[i].dataType, pReadRsp->attrList[i].data );
        pBuf = osal_memcpy( pBuf, pReadRsp->attrList[i].data, dataLength );
      }
    }

    MT_ZllSendZCLCmd( SAMPLEREMOTE_ENDPOINT, pInMsg->srcAddr.addr.shortAddr,
                      pInMsg->srcAddr.endPoint, pInMsg->clusterId, &pInMsg->zclHdr, len, msg );

    osal_mem_free( msg );

    return ( ZSuccess );
  }
#endif //MT_APP_FUNC

  return ( ZFailure );
}
#endif // ZCL_READ


/*********************************************************************
 * @fn      zllSampleRemote_GetGrpIDsReqCB
 *
 * @brief   This callback is called to process a Get Group Identifiers
 *          Request command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 * @param   seqNum - transaction sequence number
 *
 * @return  ZStatus_t
 */
static ZStatus_t zllSampleRemote_GetGrpIDsReqCB( afAddrType_t *srcAddr, zclLLGetGrpIDsReq_t *pReq, uint8 seqNum )
{
  zclLLGetGrpIDsRsp_t Rsp = {0};
  grpInfoRec_t *pGrpIDInfoRec = NULL;
  uint8 i;

  for (i = 0; i < MAX_LINKED_GROUPS; i++)
  {
    if ( controlledGroups.arr[Rsp.total] != 0 )
    {
      Rsp.total++;
    }
  }

  Rsp.startIndex = pReq->startIndex;
  if ( Rsp.total <= Rsp.startIndex )
  {
    Rsp.cnt = 0;
  }
  else
  {
    Rsp.cnt = Rsp.total - Rsp.startIndex;
    pGrpIDInfoRec = osal_mem_alloc( Rsp.cnt * sizeof(grpInfoRec_t) );
    if ( pGrpIDInfoRec != NULL )
    {
      Rsp.grpInfoRec = pGrpIDInfoRec;
      for ( i = Rsp.startIndex; i < (Rsp.cnt + Rsp.startIndex); i++ )
      {
        Rsp.grpInfoRec[i].grpID = controlledGroups.arr[i];
        Rsp.grpInfoRec[i].grpType = 0; //unsupported in current spec
      }
    }
    else
    {
      Rsp.cnt = 0;
    }
  }

  zclLL_Send_GetGrpIDsRsp( SAMPLEREMOTE_ENDPOINT, srcAddr, &Rsp, TRUE ,seqNum );

  if ( pGrpIDInfoRec != NULL )
  {
    osal_mem_free( pGrpIDInfoRec );
  }
  return ( ZSuccess );
}


/*********************************************************************
 * @fn      zllSampleRemote_GetEPListReqCB
 *
 * @brief   This callback is called to process a Get Endpoint List
 *          Request command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 * @param   seqNum - transaction sequence number
 *
 * @return  ZStatus_t
 */
static ZStatus_t zllSampleRemote_GetEPListReqCB( afAddrType_t *srcAddr, zclLLGetEPListReq_t *pReq, uint8 seqNum )
{
  zclLLGetEPListRsp_t Rsp = {0};
  epInfoRec_t *pInfoRec = NULL;
  uint8 InfoRecIdx, LinkedTargetAddrIdx;
  ZStatus_t status;

  LinkedTargetAddrIdx = pReq->startIndex;
  Rsp.total = linkedAddrNum;
  Rsp.startIndex = LinkedTargetAddrIdx;
  Rsp.cnt = Rsp.total - Rsp.startIndex;
  if ( Rsp.cnt > 0 )
  {
    pInfoRec = osal_mem_alloc( Rsp.cnt * sizeof(epInfoRec_t) );
    if ( pInfoRec != NULL )
    {
      Rsp.epInfoRec = pInfoRec;

      for(InfoRecIdx = 0; LinkedTargetAddrIdx < linkedAddrNum; InfoRecIdx++, LinkedTargetAddrIdx++)
      {
        pInfoRec[InfoRecIdx].nwkAddr = linkedTargets.arr[LinkedTargetAddrIdx].Addr;
        pInfoRec[InfoRecIdx].endpoint = linkedTargets.arr[LinkedTargetAddrIdx].EP;
        pInfoRec[InfoRecIdx].profileID = linkedTargets.arr[LinkedTargetAddrIdx].profileID;
        pInfoRec[InfoRecIdx].deviceID = linkedTargets.arr[LinkedTargetAddrIdx].deviceID;
        pInfoRec[InfoRecIdx].version = linkedTargets.arr[LinkedTargetAddrIdx].deviceVersion;
      }
    }
    else
    {
      return ( ZFailure );
    }
  }
  else
  {
    Rsp.cnt = 0;
  }

  status = zclLL_Send_GetEPListRsp( SAMPLEREMOTE_ENDPOINT, srcAddr, &Rsp, TRUE ,seqNum );
  if ( pInfoRec != NULL )
  {
    osal_mem_free( pInfoRec );
  }

  return status;
}

/*********************************************************************
 * @fn      zllSampleRemote_GetEndpointInfoCB
 *
 * @brief   This callback is called to process a Endpoint info command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t zllSampleRemote_GetEndpointInfoCB( afAddrType_t *srcAddr, zclLLEndpointInfo_t *pReq )
{
  zclLLGetGrpIDsReq_t zclLLGetGrpIDsReq;
  zclLLGetEPListReq_t zclLLGetEPListReq;
  static afAddrType_t DstAddr;

  DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  DstAddr.endPoint = pReq->endpoint;
  DstAddr.addr.shortAddr = pReq->nwkAddr;
  DstAddr.panId = _NIB.nwkPanId;
  nwk_states_t nwkState = _NIB.nwkState;
  _NIB.nwkState = NWK_ENDDEVICE;

  zclLLGetGrpIDsReq.startIndex = 0;
  zclLLGetEPListReq.startIndex = 0;

#ifdef ZLL_UTILITY_SEND_GETGRPIDS_ENABLED
  zclLL_Send_GetGrpIDsReq( SAMPLEREMOTE_ENDPOINT, &DstAddr, &zclLLGetGrpIDsReq, TRUE, sampleRemoteSeqNum++ );
#endif //ZLL_UTILITY_SEND_GETGRPIDS_ENABLED
#ifdef ZLL_UTILITY_SEND_GETEPLIST_ENABLED
  zclLL_Send_GetEPListReq( SAMPLEREMOTE_ENDPOINT, &DstAddr, &zclLLGetEPListReq, TRUE, sampleRemoteSeqNum++ );
#endif //ZLL_UTILITY_SEND_GETEPLIST_ENABLED

  _NIB.nwkState = nwkState;

  return ( ZSuccess );

}

/*********************************************************************
 * @fn      zllSampleRemote_GetEPListRspCB
 *
 * @brief   This callback is called to process a Get EP List Response.
 *
 * @param   srcAddr - sender's address
 * @param   pRsp - parsed command
 *
 * @return  status
 */
static ZStatus_t zllSampleRemote_GetEPListRspCB( afAddrType_t *srcAddr, zclLLGetEPListRsp_t *pRsp )
{
  uint8 i;
  ZStatus_t stat = ZSuccess;
  for ( i = 0; i < pRsp->cnt; i++ )
  {
    if ( ( pRsp->epInfoRec[i].nwkAddr == NLME_GetShortAddr() ) && ( pRsp->epInfoRec[i].endpoint == SAMPLEREMOTE_ENDPOINT ) )
    {
      continue;
    }
    stat = zllSampleRemote_UpdateLinkedTarget( &(pRsp->epInfoRec[i]) );
    if ( stat != ZSuccess )
    {
      break;
    }
  }
  return stat;
}

/*********************************************************************
 * @fn      zllSampleRemote_GetEPListRspCB
 *
 * @brief   This callback is called to process a Get EP List Response.
 *
 * @param   srcAddr - sender's address
 * @param   pRsp - parsed command
 *
 * @return  status
 */
static ZStatus_t zllSampleRemote_GetGrpIDsRspCB( afAddrType_t *srcAddr, zclLLGetGrpIDsRsp_t *pRsp)
{
  uint8 i;
  for ( i = 0; i < pRsp->cnt; i++ )
  {
    if ( zllSampleRemote_addControlledGroup( pRsp->grpInfoRec[i].grpID ) == FALSE )
    {
      return ( ZFailure );
    }
  }
  return ( ZSuccess );
}

/*********************************************************************
 * @fn          zllSampleRemote_GroupRspCB
 *
 * @brief       This callback is called to process Groups cluster responses.
 *              It is used to add groups to the controlled groups list.
 *
 * @param       pRsp - pointer to the response command parsed data struct
 *
 * @return      none
 */
static void zllSampleRemote_GroupRspCB( zclGroupRsp_t *pRsp )
{
  if ( pRsp->status != ZCL_STATUS_SUCCESS )
  {
    return;
  }
  if ( pRsp->cmdID == COMMAND_GROUP_ADD_RSP )
  {
    // update target group list
    zllSampleRemote_addControlledGroup( pRsp->grpList[0] );
#ifdef BUZZER_FEEDBACK
    HalBuzzerRing( 200, NULL );
#endif
  }
  if ( pRsp->cmdID == COMMAND_GROUP_REMOVE_RSP )
  {
#ifdef BUZZER_FEEDBACK
    HalBuzzerRing( 200, NULL );
    osal_start_timerEx( zllSampleRemote_TaskID, SAMPLEREMOTE_GRP_REMOVE_EVT, 200 );
#endif
  }
}

/*********************************************************************
 * @fn          zllSampleRemote_addControlledGroup
 *
 * @brief       Add group ID to the controlled groups list.
 *
 * @param       groupId - the groupID to add.
 *
 * @return      TRUE if added or already exists, FALSE if no space left
 */
static uint8 zllSampleRemote_addControlledGroup( uint16 groupId )
{
  for (uint8 i = 0; i < MAX_LINKED_GROUPS; i++)
  {
    if ( controlledGroups.arr[i] == groupId )
    {
      return TRUE;
    }
    else if ( controlledGroups.arr[i] == 0 )
    {
      controlledGroups.arr[i] = groupId;
#if defined ( NV_RESTORE )
      osal_nv_write( ZCD_NV_ZLL_REMOTE_CTRL_GROUPS, 0, sizeof( controlledGroups ), &controlledGroups );
#endif
      return TRUE;
    }
  }
  return FALSE;
}


/****************************************************************************
****************************************************************************/



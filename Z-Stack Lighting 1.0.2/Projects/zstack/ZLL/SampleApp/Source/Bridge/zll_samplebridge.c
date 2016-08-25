/**************************************************************************************************
  Filename:       zll_samplebridge.c
  Revised:        $Date: 2013-04-05 10:52:56 -0700 (Fri, 05 Apr 2013) $
  Revision:       $Revision: 33786 $


  Description:    Zigbee Cluster Library - Light Link (ZLL) Bridge Sample
                  Application.


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
#include "ZDApp.h"
#include "ZDObject.h"

#if defined ( INTER_PAN )
#include "stub_aps.h"
#endif

#include "zll_initiator.h"

#if defined ( MT_APP_FUNC )
#include "MT_APP.h"
#include "zll_rpc.h"
#endif

#include "zll_samplebridge.h"

#include "onboard.h"
#ifdef HAL_BOARD_CC2530USB
#include "osal_clock.h"
#endif
/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"

/*********************************************************************
 * MACROS
 */

#ifdef HAL_BOARD_CC2530USB
  #define TOUCH_LINK_KEY      0
  #define FACTORY_RESET_KEY   (HAL_KEY_SW_1 | HAL_KEY_SW_2)
  #define CLASSIC_COMMISS_KEY HAL_KEY_SW_2
  #define PERMIT_JOIN_KEY     HAL_KEY_SW_1
  #define ON_KEY              0
  #define OFF_KEY             0
  #define DEVICE_DISCOVERY    0
  #define DEV_SEL_UP_KEY      0
  #define DEV_SEL_DN_KEY      0
#else // HAL_BOARD_CC2530EB
  #define TOUCH_LINK_KEY      HAL_KEY_LEFT
  #define FACTORY_RESET_KEY   HAL_KEY_RIGHT
  #define CLASSIC_COMMISS_KEY (HAL_KEY_UP | HAL_KEY_SW_6)
  #define PERMIT_JOIN_KEY     (HAL_KEY_DOWN | HAL_KEY_SW_6)
  #define ON_KEY              HAL_KEY_UP
  #define OFF_KEY             HAL_KEY_DOWN
  #define DEVICE_DISCOVERY    HAL_KEY_CENTER
  #define DEV_SEL_UP_KEY      (HAL_KEY_RIGHT | HAL_KEY_SW_6)
  #define DEV_SEL_DN_KEY      (HAL_KEY_LEFT | HAL_KEY_SW_6)
#endif

#define PERMIT_JOIN_DURATION       60
#define DEVICE_DISCOVERY_DELAY     2000

/*********************************************************************
 * CONSTANTS
 */

// Application Events
#define SAMPLEBRIDGE_IDENTIFY_TIMEOUT_EVT    0x0001
#define SAMPLEBRIDGE_DEV_ANNCE_EVT           0x0002

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zllSampleBridge_TaskID;


/*********************************************************************
 * GLOBAL FUNCTIONS
 */
extern void zll_ItemInit( uint16 id, uint16 len, void *pBuf );
extern void initiatorSelectNwkParams( void );

/*********************************************************************
 * LOCAL VARIABLES
 */
static afAddrType_t zllSampleBridge_DstAddr;
static zllBridgeLinkedTargetList_t linkedTargets;
static uint8 linkedAddrNextIdx = 0;
static uint8 linkedAddrSelIdx = 0;
static uint8 linkedAddrNum = 0;
static zllBridgeControlledGroupsList_t controlledGroups;
static uint8 sampleBridgeSeqNum = 0;
static uint16 lastDevAnnceAddr = INVALID_NODE_ADDR;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
#if defined ( MT_APP_FUNC )
static void zllSampleBridge_ProcessAppMsg( uint8 srcEP, uint8 len, uint8 *msg );
#endif
static void zllSampleBridge_HandleKeys( byte shift, byte keys );
static void zllSampleBridge_BasicResetCB( void );
static void zllSampleBridge_IdentifyCB( zclIdentify_t *pCmd );
static void zllSampleBridge_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void zllSampleBridge_GroupRspCB( zclGroupRsp_t *pRsp );
static void zllSampleBridge_ProcessIdentifyTimeChange( void );
static ZStatus_t zllSampleBridge_SendActiveEPReq( uint16 dstAddr );

// Functions to process ZCL Foundation incoming Command/Response messages
static void zllSampleBridge_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
static ZStatus_t zllSampleBridge_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_WRITE
static uint8 zllSampleBridge_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif
static uint8 zllSampleBridge_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#ifdef ZCL_DISCOVER
static uint8 zllSampleBridge_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif

// This callback is called to process a Get Group Identifiers Request command
static ZStatus_t zllSampleBridge_GetGrpIDsReqCB( afAddrType_t *srcAddr, zclLLGetGrpIDsReq_t *pReq, uint8 seqNum );

// This callback is called to process a Get Endpoint List Request command
static ZStatus_t zllSampleBridge_GetEPListReqCB( afAddrType_t *srcAddr, zclLLGetEPListReq_t *pReq, uint8 seqNum );

// This callback is called to process a Get Endpoint Info Request command
static ZStatus_t zllSampleBridge_GetEndpointInfoCB( afAddrType_t *srcAddr, zclLLEndpointInfo_t *pRsp );

// Target touch linked notification
static ZStatus_t zllSampleBridge_ProcessTL( epInfoRec_t *pRec );
static ZStatus_t zllSampleBridge_UpdateLinkedTarget( epInfoRec_t *pRec );
static void zllSampleBridge_InitLinkedTargets( void );
static uint8 zllSampleBridge_addControlledGroup( uint16 groupId );
// Device Discovery
static void zllSampleBridge_ProcessZDOMsg( zdoIncomingMsg_t *inMsg );
static bool zllSampleBridge_SelectTargetSimpleDesc( SimpleDescriptionFormat_t *pSimpleDesc );

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zllSampleBridge_GenCmdCBs =
{
  zllSampleBridge_BasicResetCB,           // Basic Cluster Reset command
  zllSampleBridge_IdentifyCB,             // Identify command
#ifdef ZCL_EZMODE
  NULL,                                   // Identify EZ-Mode Invoke command
  NULL,                                   // Identify Update Commission State command
#endif
  NULL,                                   // Identify Trigger Effect command
  zllSampleBridge_IdentifyQueryRspCB,     // Identify Query Response command
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
  zllSampleBridge_GroupRspCB,             // Group Response commands
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
static zclLL_AppCallbacks_t zllSampleBridge_LLCmdCBs =
{
  // Received Server Commands
  zllSampleBridge_GetGrpIDsReqCB,         // Get Group Identifiers Request command
  zllSampleBridge_GetEPListReqCB,         // Get Endpoint List Request command

  // Received Client Commands
  zllSampleBridge_GetEndpointInfoCB,      // Endpoint Information command
  NULL,                                   // Get Group Identifiers Response command
  NULL                                    // Get Endpoint List Response command
};

/*********************************************************************
 * @fn          zllSampleBridge_Init
 *
 * @brief       Initialization function for the Sample Bridge App task.
 *
 * @param       task_id
 *
 * @return      none
 */
void zllSampleBridge_Init( byte task_id )
{
  zllSampleBridge_TaskID = task_id;

  // Set destination address to indirect
  zllSampleBridge_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  zllSampleBridge_DstAddr.endPoint = 0;
  zllSampleBridge_DstAddr.addr.shortAddr = 0;

  // init linked list in NV
  zllSampleBridge_InitLinkedTargets();

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( SAMPLEBRIDGE_ENDPOINT, &zllSampleBridge_GenCmdCBs );

  // Register for ZCL Light Link Cluster Library callback functions
  zclLL_RegisterCmdCallbacks( SAMPLEBRIDGE_ENDPOINT, &zllSampleBridge_LLCmdCBs );

  // Register the application's attribute list
  zcl_registerAttrList( SAMPLEBRIDGE_ENDPOINT, SAMPLEBRIDGE_MAX_ATTRIBUTES, zllSampleBridge_Attrs );

  // Register the Application to receive the unprocessed Foundation command/response messages
  zllInitiator_RegisterForMsg( zllSampleBridge_TaskID );

  // Register the application for ZDO messages for device discovery
  ZDO_RegisterForZDOMsg( zllSampleBridge_TaskID, Device_annce );
  ZDO_RegisterForZDOMsg( zllSampleBridge_TaskID, Active_EP_rsp );
  ZDO_RegisterForZDOMsg( zllSampleBridge_TaskID, Simple_Desc_rsp );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( zllSampleBridge_TaskID );

  zllInitiator_RegisterApp( &zllSampleBridge_SimpleDesc, &zllSampleBridge_DeviceInfo );

  zllInitiator_RegisterIdentifyCB( zllSampleBridge_IdentifyCB );

  zllInitiator_RegisterNotifyTLCB( zllSampleBridge_ProcessTL );

  zllInitiator_RegisterResetAppCB ( zllSampleBridge_BasicResetCB );

  zllInitiator_InitDevice();

}

/*********************************************************************
 * @fn          zclSample_event_loop
 *
 * @brief       Event Loop Processor for the Sample Bridge App task.
 *
 * @param       task_id
 * @param       events - events bitmap
 *
 * @return      unprocessed events bitmap
 */
uint16 zllSampleBridge_event_loop( uint8 task_id, uint16 events )
{
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    afIncomingMSGPacket_t *pMsg;

    if ( (pMsg = (afIncomingMSGPacket_t *)osal_msg_receive( zllSampleBridge_TaskID )) )
    {
      switch ( pMsg->hdr.event )
      {
#if defined (MT_APP_FUNC)
        case MT_SYS_APP_MSG:
          // Message received from MT
         zllSampleBridge_ProcessAppMsg( ((mtSysAppMsg_t *)pMsg)->endpoint,
                                        ((mtSysAppMsg_t *)pMsg)->appDataLen,
                                        ((mtSysAppMsg_t *)pMsg)->appData );
          break;
#endif
        case ZCL_INCOMING_MSG:
          // Incoming ZCL Foundation command/response messages
          zllSampleBridge_ProcessIncomingMsg( (zclIncomingMsg_t *)pMsg );
          break;

        case KEY_CHANGE:
          zllSampleBridge_HandleKeys( ((keyChange_t *)pMsg)->state, ((keyChange_t *)pMsg)->keys );
          break;

        case ZDO_CB_MSG:
          // ZDO sends the message that we registered for
          zllSampleBridge_ProcessZDOMsg( (zdoIncomingMsg_t *)pMsg );
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

  if ( events & SAMPLEBRIDGE_IDENTIFY_TIMEOUT_EVT )
  {
    if ( zllSampleBridge_IdentifyTime > 0 )
      zllSampleBridge_IdentifyTime--;
    zllSampleBridge_ProcessIdentifyTimeChange();

    return ( events ^ SAMPLEBRIDGE_IDENTIFY_TIMEOUT_EVT );
  }

  if ( events & SAMPLEBRIDGE_DEV_ANNCE_EVT )
  {
    if ( lastDevAnnceAddr != INVALID_NODE_ADDR )
    {
      zllSampleBridge_SendActiveEPReq( lastDevAnnceAddr );
      lastDevAnnceAddr = INVALID_NODE_ADDR;
    }
    return  ( events ^ SAMPLEBRIDGE_DEV_ANNCE_EVT );
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      zllSampleBridge_ProcessTL
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
static ZStatus_t zllSampleBridge_ProcessTL( epInfoRec_t *pRec )
{
  zllSampleBridge_DstAddr.endPoint = pRec->endpoint;
  zllSampleBridge_DstAddr.addrMode = afAddr16Bit;
  zllSampleBridge_DstAddr.addr.shortAddr = pRec->nwkAddr;
#if defined( INTER_PAN )
  zllSampleBridge_DstAddr.panId = _NIB.nwkPanId;
#endif

  zllSampleBridge_UpdateLinkedTarget( pRec );

#if ( HAL_LED == TRUE )
  // Light LED
  HalLedSet( HAL_LED_2, HAL_LED_MODE_ON );
  HalLedSet( HAL_LED_1, HAL_LED_MODE_OFF );
#endif

  //check if this is a light.
  if( pRec->deviceID < ZLL_DEVICEID_COLOR_CONTORLLER )
  {
    HalLcdWriteStringValue( "TL Light:", pRec->nwkAddr, 16, HAL_LCD_LINE_3 );
  }
  else
  {
    HalLcdWriteStringValue( "TL Ctrlr:", pRec->nwkAddr, 16, HAL_LCD_LINE_3 );

#ifdef ZLL_UTILITY_SEND_EPINFO_ENABLED
    zllInitiator_SendEPInfo( SAMPLEBRIDGE_ENDPOINT, &zllSampleBridge_DstAddr, sampleBridgeSeqNum++ );
#endif //ZLL_UTILITY_SEND_EPINFO_ENABLED
  }

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      zllSampleBridge_UpdateLinkedTarget
 *
 * @brief   Add or update target in linked targets list
 *
 * @param   pRec - Target's enpoint information record
 *
 * @return  status
 */
static ZStatus_t zllSampleBridge_UpdateLinkedTarget( epInfoRec_t *pRec )
{
  uint8 idx;
  ZStatus_t status = ZSuccess;
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
  osal_nv_write( ZCD_NV_ZLL_BRIDGE_LINK_TARGETS, 0, sizeof( linkedTargets ), &linkedTargets );
#endif
  return status;
}

/*********************************************************************
 * @fn      zllSampleBridge_InitLinkedTargets()
 *
 * @brief   Initialize linked targets and controlled groups lists in NV.
 *
 * @param   none
 *
 * @return  none
 */
static void zllSampleBridge_InitLinkedTargets( void )
{
  uint8 i;
  osal_memset( &linkedTargets, 0xFF, sizeof(linkedTargets));
  zll_ItemInit( ZCD_NV_ZLL_BRIDGE_LINK_TARGETS, sizeof(zllBridgeLinkedTargetList_t), &linkedTargets );
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
  zll_ItemInit( ZCD_NV_ZLL_BRIDGE_CTRL_GROUPS, sizeof(zllBridgeControlledGroupsList_t), &controlledGroups );
}

#if defined ( MT_APP_FUNC )
/*********************************************************************
 * @fn      zllSampleBridge_ProcessAppMsg
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
static void zllSampleBridge_ProcessAppMsg( uint8 srcEP, uint8 len, uint8 *msg )
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
 * @fn      zllSampleBridge_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events.
 *
 * @return  none
 */
static void zllSampleBridge_HandleKeys( byte shift, byte keys )
{
  if(shift)
  {
    keys |= HAL_KEY_SW_6;
  }

  if ( keys == ON_KEY )
  {
    zclGeneral_SendOnOff_CmdOn( SAMPLEBRIDGE_ENDPOINT, &zllSampleBridge_DstAddr, FALSE, sampleBridgeSeqNum++ );
  }

  if ( keys == FACTORY_RESET_KEY )
  {
    zllInitiator_ResetToFactoryNew();
  }

  if ( keys == OFF_KEY )
  {
    zclGeneral_SendOnOff_CmdOff( SAMPLEBRIDGE_ENDPOINT, &zllSampleBridge_DstAddr, FALSE, sampleBridgeSeqNum++ );
  }

  if ( keys == TOUCH_LINK_KEY )
  {
    zllInitiator_StartDevDisc();
  }

  if ( keys == DEV_SEL_UP_KEY )
  {
    if ( linkedAddrNum > 0 )
    {
      if( (linkedAddrSelIdx+1) >= linkedAddrNum )
      {
        linkedAddrSelIdx = 0;
      }
      else
      {
        linkedAddrSelIdx++;
      }
      zllSampleBridge_DstAddr.addrMode = afAddr16Bit;
      zllSampleBridge_DstAddr.addr.shortAddr = linkedTargets.arr[linkedAddrSelIdx].Addr;
      zllSampleBridge_DstAddr.endPoint =  linkedTargets.arr[linkedAddrSelIdx].EP;

      zclGeneral_SendIdentify( SAMPLEBRIDGE_ENDPOINT, &zllSampleBridge_DstAddr,
                               SAMPLEBRIDGE_CMD_IDENTIFY_TIME, FALSE, sampleBridgeSeqNum++ );

      HalLcdWriteStringValue( "Dev Sel:", zllSampleBridge_DstAddr.addr.shortAddr, 16, HAL_LCD_LINE_3 );
    }
    else
    {
      HalLcdWriteString( "Dev Sel: no trgt", HAL_LCD_LINE_3 );
    }
  }

  if ( keys == DEV_SEL_DN_KEY )
  {
    if ( linkedAddrNum > 0 )
    {
      if(linkedAddrSelIdx < 1)
      {
        linkedAddrSelIdx = (linkedAddrNum-1);
      }
      else
      {
        linkedAddrSelIdx--;
      }
      zllSampleBridge_DstAddr.addrMode = afAddr16Bit;
      zllSampleBridge_DstAddr.addr.shortAddr = linkedTargets.arr[linkedAddrSelIdx].Addr;
      zllSampleBridge_DstAddr.endPoint = linkedTargets.arr[linkedAddrSelIdx].EP;

      zclGeneral_SendIdentify( SAMPLEBRIDGE_ENDPOINT, &zllSampleBridge_DstAddr,
                               SAMPLEBRIDGE_CMD_IDENTIFY_TIME, FALSE, sampleBridgeSeqNum++ );

      HalLcdWriteStringValue( "Dev Sel:", zllSampleBridge_DstAddr.addr.shortAddr, 16, HAL_LCD_LINE_3 );
    }
    else
    {
      HalLcdWriteString( "Dev Sel: no trgt", HAL_LCD_LINE_3 );
    }
  }

  if ( keys == PERMIT_JOIN_KEY )
  {
    if ( zllInitiator_BridgeStartNetwork() != ZSuccess )
    {
      zllInitiator_PermitJoin( PERMIT_JOIN_DURATION );
      HalLcdWriteString( "PermitJoin", HAL_LCD_LINE_3 );
    }
  }

  if ( keys == CLASSIC_COMMISS_KEY )
  {
    zllInitiator_ClassicalCommissioningStart();
  }

}

/*********************************************************************
 * @fn      zllSampleBridge_ProcessIdentifyTimeChange
 *
 * @brief   Called to process any change to the IdentifyTime attribute.
 *
 * @param   none
 *
 * @return  none
 */
static void zllSampleBridge_ProcessIdentifyTimeChange( void )
{
  if ( zllSampleBridge_IdentifyTime > 0 )
  {
    osal_start_timerEx( zllSampleBridge_TaskID, SAMPLEBRIDGE_IDENTIFY_TIMEOUT_EVT, 1000 );
#if ( HAL_LED == TRUE )
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
#endif
  }
  else
  {
#if ( HAL_LED == TRUE )
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
#endif
    osal_stop_timerEx( zllSampleBridge_TaskID, SAMPLEBRIDGE_IDENTIFY_TIMEOUT_EVT );
  }
}

/*********************************************************************
 * @fn      zllSampleBridge_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to default values.
 *
 * @param   none
 *
 * @return  none
 */
static void zllSampleBridge_BasicResetCB( void )
{
  // Reset all attributes to default values
  linkedAddrNum = 0;
  linkedAddrNextIdx = 0;
  linkedAddrSelIdx = 0;
  osal_memset( &linkedTargets, 0xFF, sizeof(linkedTargets));
  osal_memset( &controlledGroups, 0x00, sizeof(controlledGroups));
#if defined ( NV_RESTORE )
  osal_nv_write( ZCD_NV_ZLL_BRIDGE_LINK_TARGETS, 0, sizeof( linkedTargets ), &linkedTargets );
  osal_nv_write( ZCD_NV_ZLL_BRIDGE_CTRL_GROUPS, 0, sizeof( controlledGroups ), &controlledGroups );
#endif
}

/*********************************************************************
 * @fn      zllSampleBridge_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   srcAddr - source address and endpoint of the response message
 * @param   identifyTime - the number of seconds to identify yourself
 *
 * @return  none
 */
static void zllSampleBridge_IdentifyCB( zclIdentify_t *pCmd )
{
  zllSampleBridge_IdentifyTime = pCmd->identifyTime;
  zllSampleBridge_ProcessIdentifyTimeChange();
}

/*********************************************************************
 * @fn      zllSampleBridge_IdentifyQueryRspCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Query Response Command for this application.
 *
 * @param   srcAddr - requestor's address
 * @param   timeout - number of seconds to identify yourself (valid for query response)
 *
 * @return  none
 */
static void zllSampleBridge_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp )
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
 * @fn      zllSampleBridge_ProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
static void zllSampleBridge_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg)
{
  switch ( pInMsg->zclHdr.commandID )
  {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      zllSampleBridge_ProcessInReadRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_WRITE
    case ZCL_CMD_WRITE_RSP:
      zllSampleBridge_ProcessInWriteRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_REPORT
    // See ZCL Test Applicaiton (zcl_testapp.c) for sample code on Attribute Reporting
    case ZCL_CMD_CONFIG_REPORT:
      //zllSampleBridge_ProcessInConfigReportCmd( pInMsg );
      break;

    case ZCL_CMD_CONFIG_REPORT_RSP:
      //zllSampleBridge_ProcessInConfigReportRspCmd( pInMsg );
      break;

    case ZCL_CMD_READ_REPORT_CFG:
      //zllSampleBridge_ProcessInReadReportCfgCmd( pInMsg );
      break;

    case ZCL_CMD_READ_REPORT_CFG_RSP:
      //zllSampleBridge_ProcessInReadReportCfgRspCmd( pInMsg );
      break;

    case ZCL_CMD_REPORT:
      //zllSampleBridge_ProcessInReportCmd( pInMsg );
      break;
#endif
    case ZCL_CMD_DEFAULT_RSP:
      zllSampleBridge_ProcessInDefaultRspCmd( pInMsg );
      break;
#ifdef ZCL_DISCOVER
    case ZCL_CMD_DISCOVER_ATTRS_RSP:
      zllSampleBridge_ProcessInDiscRspCmd( pInMsg );
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
 * @fn      zllSampleBridge_ProcessInReadRspCmd
 *
 * @brief   Process the ZCL Read Attributes Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  status
 */
static ZStatus_t zllSampleBridge_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
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

    MT_ZllSendZCLCmd( SAMPLEBRIDGE_ENDPOINT, pInMsg->srcAddr.addr.shortAddr,
                      pInMsg->srcAddr.endPoint, pInMsg->clusterId, &pInMsg->zclHdr, len, msg );

    osal_mem_free( msg );

    return ( ZSuccess );
  }
#endif //MT_APP_FUNC

  return ( ZFailure );
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
/*********************************************************************
 * @fn      zllSampleBridge_ProcessInWriteRspCmd
 *
 * @brief   Process the ZCL Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zllSampleBridge_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;

  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < writeRspCmd->numAttr; i++)
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }

  return TRUE;
}
#endif // ZCL_WRITE

/*********************************************************************
 * @fn      zllSampleBridge_ProcessInDefaultRspCmd
 *
 * @brief   Process the ZCL Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zllSampleBridge_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  // Device is notified of the Default Response command.
  (void)pInMsg;

  return TRUE;
}

#ifdef ZCL_DISCOVER
/*********************************************************************
 * @fn      zllSampleBridge_ProcessInDiscRspCmd
 *
 * @brief   Process the ZCL Discover Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zllSampleBridge_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverAttrsRspCmd_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverAttrsRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return TRUE;
}
#endif // ZCL_DISCOVER


/*********************************************************************
 * @fn      zllSampleBridge_GetGrpIDsReqCB
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
static ZStatus_t zllSampleBridge_GetGrpIDsReqCB( afAddrType_t *srcAddr, zclLLGetGrpIDsReq_t *pReq, uint8 seqNum )
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

  zclLL_Send_GetGrpIDsRsp( SAMPLEBRIDGE_ENDPOINT, srcAddr, &Rsp, TRUE ,seqNum );

  if ( pGrpIDInfoRec != NULL )
  {
    osal_mem_free( pGrpIDInfoRec );
  }
  return ( ZSuccess );
}


/*********************************************************************
 * @fn      zllSampleBridge_GetEPListReqCB
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
static ZStatus_t zllSampleBridge_GetEPListReqCB( afAddrType_t *srcAddr, zclLLGetEPListReq_t *pReq, uint8 seqNum )
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

  status = zclLL_Send_GetEPListRsp( SAMPLEBRIDGE_ENDPOINT, srcAddr, &Rsp, TRUE ,seqNum );
  if ( pInfoRec != NULL )
  {
    osal_mem_free( pInfoRec );
  }

  return status;
}

/*********************************************************************
 * @fn      zllSampleBridge_GetEndpointInfoCB
 *
 * @brief   This callback is called to process a Endpoint info command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t zllSampleBridge_GetEndpointInfoCB( afAddrType_t *srcAddr, zclLLEndpointInfo_t *pReq )
{
  zclLLGetGrpIDsReq_t zclLLGetGrpIDsReq;
  zclLLGetEPListReq_t zclLLGetEPListReq;
  static afAddrType_t DstAddr;

  DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  DstAddr.endPoint = pReq->endpoint;
  DstAddr.addr.shortAddr = pReq->nwkAddr;
  DstAddr.panId = _NIB.nwkPanId;
  nwk_states_t nwkState = _NIB.nwkState;
#if (ZSTACK_END_DEVICE_BUILD)
  _NIB.nwkState = NWK_ENDDEVICE;
#endif

  zclLLGetGrpIDsReq.startIndex = 0;
  zclLLGetEPListReq.startIndex = 0;

#ifdef ZLL_UTILITY_SEND_GETGRPIDS_ENABLED
  zclLL_Send_GetGrpIDsReq( SAMPLEBRIDGE_ENDPOINT, &DstAddr, &zclLLGetGrpIDsReq, TRUE, sampleBridgeSeqNum++ );
#endif //ZLL_UTILITY_SEND_GETGRPIDS_ENABLED
#ifdef ZLL_UTILITY_SEND_GETEPLIST_ENABLED
  zclLL_Send_GetEPListReq( SAMPLEBRIDGE_ENDPOINT, &DstAddr, &zclLLGetEPListReq, TRUE, sampleBridgeSeqNum++ );
#endif //ZLL_UTILITY_SEND_GETEPLIST_ENABLED

  _NIB.nwkState = nwkState;

  return ( ZSuccess );

}

/*********************************************************************
 * @fn          zllSampleBridge_GroupRspCB
 *
 * @brief       This callback is called to process Groups cluster responses.
 *              It is used to add groups to the controlled groups list.
 *
 * @param       pRsp - pointer to the response command parsed data struct
 *
 * @return      none
 */
static void zllSampleBridge_GroupRspCB( zclGroupRsp_t *pRsp )
{
  if ( pRsp->status != ZCL_STATUS_SUCCESS )
  {
    return;
  }
  if ( pRsp->cmdID == COMMAND_GROUP_ADD_RSP )
  {
    // update target group list
    zllSampleBridge_addControlledGroup( pRsp->grpList[0] );
  }
}

/*********************************************************************
 * @fn          zllSampleBridge_addControlledGroup
 *
 * @brief       Add group ID to the controlled groups list.
 *
 * @param       groupId - the groupID to add.
 *
 * @return      TRUE if added or already exists, FALSE if no space left
 */
static uint8 zllSampleBridge_addControlledGroup( uint16 groupId )
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
      osal_nv_write( ZCD_NV_ZLL_BRIDGE_CTRL_GROUPS, 0, sizeof( controlledGroups ), &controlledGroups );
#endif
      return TRUE;
    }
  }
  return FALSE;
}

/*********************************************************************
 * @fn          zllSampleBridge_ProcessZDOMsg
 *
 * @brief       Process ZDO messages for Device Discovery
 *
 * @param       inMsg - Incoming ZDO message
 *
 * @return      none
 */
static void zllSampleBridge_ProcessZDOMsg( zdoIncomingMsg_t *inMsg )
{
  static zAddrType_t addr;
  addr.addrMode = Addr16Bit;

  switch ( inMsg->clusterID )
  {
    case Device_annce:
      {
        ZDO_DeviceAnnce_t devAnnce;

        ZDO_ParseDeviceAnnce( inMsg, &devAnnce );
        if ( ( lastDevAnnceAddr != INVALID_NODE_ADDR ) && ( lastDevAnnceAddr != devAnnce.nwkAddr ) )
        {
          zllSampleBridge_SendActiveEPReq( lastDevAnnceAddr );
        }
        lastDevAnnceAddr = devAnnce.nwkAddr;
        osal_start_timerEx( zllSampleBridge_TaskID, SAMPLEBRIDGE_DEV_ANNCE_EVT, DEVICE_DISCOVERY_DELAY );
      }
      break;

    case Active_EP_rsp:
      {
        ZDO_ActiveEndpointRsp_t *pActiveEPs = NULL;
        pActiveEPs = ZDO_ParseEPListRsp( inMsg );
        if ( pActiveEPs->status == ZSuccess )
        {
          for (uint8 i=0; i < pActiveEPs->cnt; i++ )
          {
            addr.addr.shortAddr = pActiveEPs->nwkAddr;
            ZDP_SimpleDescReq( &addr, pActiveEPs->nwkAddr, pActiveEPs->epList[i], 0 );
          }
        }

        if ( pActiveEPs != NULL )
        {
          osal_mem_free( pActiveEPs );
        }
      }
      break;

    case Simple_Desc_rsp:
      {
        ZDO_SimpleDescRsp_t simpleDescRsp;
        simpleDescRsp.simpleDesc.pAppInClusterList = simpleDescRsp.simpleDesc.pAppOutClusterList = NULL;
        ZDO_ParseSimpleDescRsp( inMsg, &simpleDescRsp );

        if ( ( simpleDescRsp.status == ZDP_SUCCESS ) && ( zllSampleBridge_SelectTargetSimpleDesc( &(simpleDescRsp.simpleDesc) ) ) )
        {
          epInfoRec_t rec;
          rec.nwkAddr = simpleDescRsp.nwkAddr;
          rec.endpoint = simpleDescRsp.simpleDesc.EndPoint;
          rec.profileID = simpleDescRsp.simpleDesc.AppProfId;
          rec.deviceID = simpleDescRsp.simpleDesc.AppDeviceId;
          rec.version = simpleDescRsp.simpleDesc.AppDevVer;
          zllSampleBridge_UpdateLinkedTarget( &rec );
          HalLcdWriteStringValueValue( "linked:", simpleDescRsp.nwkAddr, 16, simpleDescRsp.simpleDesc.EndPoint, 16, HAL_LCD_LINE_3 );
        }
        if ( simpleDescRsp.simpleDesc.pAppInClusterList != NULL )
        {
          osal_mem_free( simpleDescRsp.simpleDesc.pAppInClusterList );
        }
        if ( simpleDescRsp.simpleDesc.pAppOutClusterList != NULL )
        {
          osal_mem_free( simpleDescRsp.simpleDesc.pAppOutClusterList );
        }
      }
      break;

    default:
      break;
  }
}

/*********************************************************************
 * @fn          zllSampleBridge_SendActiveEPReq
 *
 * @brief       Send out ZDP Active Endpoints Requst.
 *
 * @param       dstAddr - destination address
 *
 * @return      status
 */
static ZStatus_t zllSampleBridge_SendActiveEPReq( uint16 dstAddr )
{
  zAddrType_t addr;
  addr.addrMode = Addr16Bit;
  addr.addr.shortAddr = dstAddr;
  return ZDP_ActiveEPReq( &addr, dstAddr, 0 );
}

/*********************************************************************
 * @fn          zllSampleBridge_SelectTargetSimpleDesc
 *
 * @brief       Select or filter candidate device as controlled target.
 *
 * @param       pSimpleDesc - pointer to the device's application Simple Descriptor
 *
 * @return      TRUE if include in target list, FALSE if to discard (filter out)
 */
static bool zllSampleBridge_SelectTargetSimpleDesc( SimpleDescriptionFormat_t *pSimpleDesc )
{
  for (uint8 i=0; i<pSimpleDesc->AppNumInClusters; i++)
  {
    if ( pSimpleDesc->pAppInClusterList[i] == ZCL_CLUSTER_ID_GEN_ON_OFF )
    {
      return TRUE;
    }
#ifdef ZCL_LEVEL_CTRL
    if ( pSimpleDesc->pAppInClusterList[i] == ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL )
    {
      return TRUE;
    }
#endif
#ifdef ZCL_COLOR_CTRL
    if ( pSimpleDesc->pAppInClusterList[i] == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL )
    {
      return TRUE;
    }
#endif
  }
  return FALSE;
}


/****************************************************************************
****************************************************************************/



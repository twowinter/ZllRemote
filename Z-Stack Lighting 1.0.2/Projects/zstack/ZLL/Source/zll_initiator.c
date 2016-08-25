/**************************************************************************************************
  Filename:       zll_initiator.c
  Revised:        $Date: 2013-11-22 16:17:23 -0800 (Fri, 22 Nov 2013) $
  Revision:       $Revision: 36220 $

  Description:    Zigbee Cluster Library - Light Link Initiator.


  Copyright 2011-2013 Texas Instruments Incorporated. All rights reserved.

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
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Nv.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDSecMgr.h"
#include "ZDObject.h"
#include "nwk_util.h"

#if defined ( POWER_SAVING )
#include "OSAL_PwrMgr.h"
#endif

#if defined( INTER_PAN )
  #include "stub_aps.h"
#endif

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ll.h"
#include "zll.h"

#if defined ( MT_APP_FUNC )
#include "MT_APP.h"
#include "zll_rpc.h"
#endif

#include "zll_initiator.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define ZLL_INITIATOR_REJOIN_TIMEOUT             2500 // 2.5 sec

// for non-polling end-devices only
#define ZLL_INITIATOR_TEMP_POST_TL_POLL_RATE     1000

#define DEV_INFO_INVALID_EP                      0xFE

#define ZLL_ENDDEVICE_JOIN_EVT                             ZLL_NWK_JOIN_EVT
#define ZLL_TL_SCAN_BASE_EVT                               ZLL_AUX_1_EVT
#define ZLL_CFG_TARGET_EVT                                 ZLL_AUX_2_EVT
#define ZLL_W4_NWK_START_RSP_EVT                           ZLL_AUX_3_EVT
#define ZLL_W4_NWK_JOIN_RSP_EVT                            ZLL_AUX_4_EVT
#define ZLL_DISABLE_RX_EVT                                 ZLL_AUX_5_EVT
#define ZLL_W4_REJOIN_EVT                                  ZLL_AUX_6_EVT
#define ZLL_NOTIFY_APP_EVT                                 ZLL_AUX_7_EVT

#define ZLL_INITIATOR_NUM_SCAN_REQ_PRIMARY       8  // 5 times on 1st channel, plus once for each remianing primary channel
#define ZLL_INITIATOR_NUM_SCAN_REQ_EXTENDED      20 // (ZLL_NUM_SCAN_REQ_PRIMARY + sizeof(ZLL_SECONDARY_CHANNELS_SET))

/*********************************************************************
 * TYPEDEFS
 */
typedef union
{
  zclLLNwkStartRsp_t nwkStartRsp;
  zclLLNwkJoinRsp_t nwkJoinRsp;
} zclLLRsp_t;

typedef struct
{
  zclLLScanRsp_t scanRsp;
  afAddrType_t srcAddr;
  uint16 newNwkAddr;
  uint8 rxChannel;  // channel scan response was heard on
  int8 lastRssi;    // receieved RSSI
} targetCandidate_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8 zllInitiator_TaskID;

/*********************************************************************
 * EXTERNAL VARIABLES
 */
extern devStartModes_t devStartMode;
extern uint8 _tmpRejoinState;
extern uint8 zllJoinedHANetwork;

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static uint8 initiatorSeqNum;

// Touch Link channel tracking
static uint8 numScanReqSent;
static uint8 scanReqChannels;

// Network key sent to the target to start the network with
static uint8 keyIndexSent;
static uint8 encKeySent[SEC_KEY_LEN];
static uint32 responseIDSent;

// Info related to the received request
static zclLLNwkJoinReq_t joinReq;

// Info related to the received response
static targetCandidate_t selectedTarget;
static zclLLRsp_t rxRsp; // network start or join response

// Addresses used for sending/receiving messages
static afAddrType_t bcastAddr;

static uint16 savedPollRate;
static uint16 savedQueuedPollRate;
static uint16 savedResponsePollRate;
static uint8 savedRxOnIdle;

// Application callback
static zclGCB_BasicReset_t pfnResetAppCB = NULL;
static zclGCB_Identify_t pfnIdentifyCB = NULL;
static zll_NotifyAppTLCB_t pfnNotifyAppCB = NULL;
static zll_SelectDiscDevCB_t pfnSelectDiscDevCB = NULL;

static uint8 initiatorRegisteredMsgAppTaskID = TASK_NO_TASK;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

// This callback is called to process a Scan Request command
static ZStatus_t initiatorScanReqCB( afAddrType_t *srcAddr, zclLLScanReq_t *pReq, uint8 seqNum );

// This callback is called to process a Device Information Request command
static ZStatus_t initiatorDeviceInfoReqCB( afAddrType_t *srcAddr, zclLLDeviceInfoReq_t *pReq, uint8 seqNum );

// This callback is called to process an Identify Request command
static ZStatus_t initiatorIdentifyReqCB( afAddrType_t *srcAddr, zclLLIdentifyReq_t *pReq );

// This callback is called to process a Reset to Factory New Request command
static ZStatus_t initiatorResetToFNReqCB( afAddrType_t *srcAddr, zclLLResetToFNReq_t *pReq );

// This callback is called to process a Network Join End Device Request command
static ZStatus_t initiatorNwkJoinReqCB( afAddrType_t *srcAddr, zclLLNwkJoinReq_t *pReq, uint8 seqNum );

// This callback is called to process a Network Update Request command
static ZStatus_t initiatorNwkUpdateReqCB( afAddrType_t *srcAddr, zclLLNwkUpdateReq_t *pReq );

// This callback is called to process a Scan Response command
static ZStatus_t initiatorScanRspCB( afAddrType_t *srcAddr, zclLLScanRsp_t *pRsp );

// This callback is called to process a Device Information Response command
static ZStatus_t initiatorDeviceInfoRspCB( afAddrType_t *srcAddr, zclLLDeviceInfoRsp_t *pRsp );

// This callback is called to process a Network Start Response command
static ZStatus_t initiatorNwkStartRspCB( afAddrType_t *srcAddr, zclLLNwkStartRsp_t *pRsp );

// This callback is called to process a Network Join Router Response command
static ZStatus_t initiatorNwkJoinRtrRspCB( afAddrType_t *srcAddr, zclLLNwkJoinRsp_t *pRsp );

// This callback is called to process a Network Join End Device Response command
static ZStatus_t initiatorNwkJoinEDRspCB( afAddrType_t *srcAddr, zclLLNwkJoinRsp_t *pRsp );

// This callback is called to process a Leave Confirmation message
static void *initiatorZdoLeaveCnfCB( void *pParam );

static void initiatorProcessStateChange( devStates_t state );
void initiatorSelectNwkParams( void );
static void initiatorSetNwkToInitState( void );
static void initiatorJoinNwk( void );
static void initiatorReJoinNwk( devStartModes_t startMode );
static void initiatorSendScanReq( bool freshScan );
static ZStatus_t initiatorSendNwkStartReq( zclLLScanRsp_t *pRsp );
static ZStatus_t initiatorSendNwkJoinReq( zclLLScanRsp_t *pRsp );
static ZStatus_t initiatorSendNwkUpdateReq( zclLLScanRsp_t *pRsp );
static void initiatorClearSelectedTarget( void );

#if defined ( MT_APP_FUNC )
static ZStatus_t initiatorProcessRpcMsg( uint8 srcEP, uint8 len, uint8 *msg );
#endif

/*********************************************************************
 * ZLL Initiator Callback Table
 */
// Initiator Command Callbacks table
static zclLL_InterPANCallbacks_t zllInitiator_CmdCBs =
{
  // Received Server Commands
  initiatorScanReqCB,       // Scan Request command
  initiatorDeviceInfoReqCB, // Device Information Request command
  initiatorIdentifyReqCB,   // Identify Request command
  initiatorResetToFNReqCB,  // Reset to Factory New Request command
  NULL,                     // Network Start Request command
#if ( ZSTACK_ROUTER_BUILD )
  initiatorNwkJoinReqCB,    // Network Join Router Request command
  NULL,                     // Network Join End Device Request command
#else
  NULL,                     // Network Join Router Request command
  initiatorNwkJoinReqCB,    // Network Join End Device Request command
#endif
  initiatorNwkUpdateReqCB,  // Network Update Request command

  // Received Client Commands
  initiatorScanRspCB,       // Scan Response command
  initiatorDeviceInfoRspCB, // Device Information Response command
  initiatorNwkStartRspCB,   // Network Start Response command
  initiatorNwkJoinRtrRspCB, // Network Join Router Response command
  initiatorNwkJoinEDRspCB   // Network Join End Device Response command
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      zllInitiator_InitDevice
 *
 * @brief   Start the ZLL Initiator device in the network if it's not
 *          factory new. Otherwise, determine the network parameters
 *          and wait for a touchlink command.
 *
 * @param   none
 *
 * @return  status
 */
ZStatus_t zllInitiator_InitDevice( void )
{
  if ( !zll_IsFactoryNew() )
  {
    // Resume ZigBee functionality based on the info stored in NV
    initiatorReJoinNwk( MODE_RESUME );
  }
  else
  {
    initiatorSelectNwkParams();
#ifndef HOLD_AUTO_START
    zllInitiator_ClassicalCommissioningStart();
#endif
  }

#if defined ( POWER_SAVING )
  osal_pwrmgr_device( PWRMGR_BATTERY );
#endif

#if ( ZSTACK_ROUTER_BUILD )
  // Enable our receiver
  savedRxOnIdle = TRUE;
  ZMacSetReq( ZMacRxOnIdle, &savedRxOnIdle );
  zll_PermitJoin(0);
#endif

  return ( ZSuccess );
}


/*********************************************************************
 * @fn      zllInitiator_ResetToFactoryNew
 *
 * @brief   Call to Reset the Initiator to factory new
 *
 * @param   none
 *
 * @return  none
 */
void zllInitiator_ResetToFactoryNew()
{
  if ( pfnResetAppCB )
  {
    pfnResetAppCB();
  }
  zll_ResetToFactoryNew( TRUE );
}

/*********************************************************************
 * @fn      zllInitiator_RegisterIdentifyCB
 *
 * @brief   Register an Application's Identify callback function.
 *
 * @param   pfnIdentify - application callback
 *
 * @return  none
 */
void zllInitiator_RegisterIdentifyCB( zclGCB_Identify_t pfnIdentify )
{
  pfnIdentifyCB = pfnIdentify;
}

/*********************************************************************
 * @fn      zllInitiator_RegisterNotifyTLCB
 *
 * @brief   Register an Application's Touch-Link Notify callback function.
 *
 * @param   pfnNotifyApp - application callback
 *
 * @return  none
 */
void zllInitiator_RegisterNotifyTLCB( zll_NotifyAppTLCB_t pfnNotifyApp )
{
  pfnNotifyAppCB = pfnNotifyApp;
}

/*********************************************************************
 * @fn      zllInitiator_RegisterResetAppCB
 *
 * @brief   Register an Application's Reset callback function.
 *
 * @param   pfnResetApp - application callback
 *
 * @return  none
 */
void zllInitiator_RegisterResetAppCB( zclGCB_BasicReset_t pfnResetApp )
{
  pfnResetAppCB = pfnResetApp;
}

/*********************************************************************
 * @fn      zllInitiator_RegisterSelectDiscDevCB
 *
 * @brief   Register an Application's Selection callback function, to select
 *          a target from the discovered devices during a Touch-link scan.
 *
 * @param   pfnSelectDiscDev - application callback
 *
 * @return  none
 */
void zllInitiator_RegisterSelectDiscDevCB( zll_SelectDiscDevCB_t pfnSelectDiscDev )
{
  pfnSelectDiscDevCB = pfnSelectDiscDev;
}

/*********************************************************************
 * @fn      zllInitiator_StartDevDisc
 *
 * @brief   Start device discovery, scanning for other devices in the vicinity
 *          of the originator (initiating first part of the Touch-Link process).
 *          Device discovery shall only be initiated by address assignment capable
 *          devices. To perform device discovery, the initiator shall broadcast
 *          inter-PAN Scan Requests, spaced at an interval of
 *          ZLL_APLC_SCAN_TIME_BASE_DURATION seconds.
 *
 * @param   none
 *
 * @return  status
 */
ZStatus_t zllInitiator_StartDevDisc( void )
{
  osal_clear_event( ZDAppTaskID, ZDO_NETWORK_INIT ); // in case orphaned rejoin was called
  ZDApp_StopJoiningCycle();
  zllHAScanInitiated = FALSE;
  //abort any touchlink in progress and start the new dev discovery.
  zllInitiator_AbortTL();

  // To perform device discovery, switch to channel 11 and broadcast five
  // consecutive inter-PAN Scan Requests. Then switch to each remaining
  // ZLL channels in turn (i.e., 15, 20, and 25) and broadcast a single
  // inter-PAN Scan Request on each channel.
  if ( !osal_get_timeoutEx( zllInitiator_TaskID, ZLL_TL_SCAN_BASE_EVT ) )
  {
    uint8 x = TRUE;

    // Generate a new Transaction Id
    zllTransID = ( ( (uint32)osal_rand() ) << 16 ) + osal_rand();
    osal_start_timerEx( zllInitiator_TaskID, ZLL_TRANS_LIFETIME_EXPIRED_EVT,
                        ZLL_APLC_INTER_PAN_TRANS_ID_LIFETIME );

    if ( !zll_IsFactoryNew() )
    {
      // Turn off polling during touch-link procedure
      savedPollRate = zgPollRate;
      savedQueuedPollRate = zgQueuedPollRate;
      savedResponsePollRate = zgResponsePollRate;

      NLME_SetPollRate( 0 );
      NLME_SetQueuedPollRate( 0 );
      NLME_SetResponseRate( 0 );
    }

    // Set NWK task to idle
    nwk_setStateIdle( TRUE );

    // Remember current rx state
    ZMacGetReq( ZMacRxOnIdle, &savedRxOnIdle );

    // MAC receiver should be on during touch-link procedure
    ZMacSetReq( ZMacRxOnIdle, &x );

    scanReqChannels = ZLL_SCAN_PRIMARY_CHANNELS;
    numScanReqSent = 0;

    // Send out the first Scan Request
    initiatorSendScanReq( TRUE );

    return ( ZSuccess );
  }

  return ( ZFailure );
}

/*********************************************************************
 * @fn      zllInitiator_AbortTL
 *
 * @brief   Abort Touch-link device discovery.
 *          Successful execution could be done before Network Start/Join
 *          commands are sent. Until then, since no device parameters
 *          such as network settings are altered, the Touch-Link is
 *          still reversible.
 *
 * @param   none
 *
 * @return  status
 */
ZStatus_t zllInitiator_AbortTL( void )
{
  if ( ( osal_stop_timerEx( zllInitiator_TaskID, ZLL_TL_SCAN_BASE_EVT ) == SUCCESS )
       || ( osal_stop_timerEx( zllInitiator_TaskID, ZLL_CFG_TARGET_EVT ) == SUCCESS )
       || ( osal_stop_timerEx( zllInitiator_TaskID, ZLL_W4_NWK_START_RSP_EVT ) == SUCCESS )
       || ( osal_stop_timerEx( zllInitiator_TaskID, ZLL_W4_NWK_JOIN_RSP_EVT ) == SUCCESS ) )
  {
    initiatorSetNwkToInitState();
    zllTransID = 0;
    numScanReqSent = 0;
    initiatorClearSelectedTarget();

    return ( ZSuccess );
  }

  return ( ZFailure );
}

/*********************************************************************
 * @fn      initiatorProcessStateChange
 *
 * @brief   Process ZDO device state change
 *
 * @param   state - The device's network state
 *
 * @return  none
 */
static void initiatorProcessStateChange( devStates_t state )
{
  if ( ( state == DEV_ROUTER ) || ( state == DEV_END_DEVICE ) )
  {
    // Save the latest NIB to update our parent's address
    zll_UpdateNV( ZLL_UPDATE_NV_NIB );

    if ( ZSTACK_ROUTER_BUILD )
    {
      ZDP_DeviceAnnce( NLME_GetShortAddr(), NLME_GetExtAddr(),
                       ZDO_Config_Node_Descriptor.CapabilityFlags, 0 );
    }

    // We found our parent
    osal_stop_timerEx( zllInitiator_TaskID, ZLL_W4_REJOIN_EVT );
  }
  else if ( ( state == DEV_NWK_ORPHAN ) || ( state == DEV_NWK_DISC ) )
  {
    // Device has lost information about its parent; give it some time to rejoin
    if ( !osal_get_timeoutEx( zllInitiator_TaskID, ZLL_W4_REJOIN_EVT ) )
    {
      osal_start_timerEx( zllInitiator_TaskID, ZLL_W4_REJOIN_EVT,
                          (NUM_DISC_ATTEMPTS + 1) * ZLL_INITIATOR_REJOIN_TIMEOUT );
    }
  }
}

/*********************************************************************
 * @fn          zllInitiator_Init
 *
 * @brief       Initialization function for the ZLL Initiator task.
 *
 * @param       task_id - ZLL Initiator task id
 *
 * @return      none
 */
void zllInitiator_Init( uint8 task_id )
{
  // Save our own Task ID
  zllInitiator_TaskID = task_id;

  zll_SetZllTaskId( zllInitiator_TaskID );


  // Build a broadcast address for the Scan Request
  bcastAddr.addrMode = afAddrBroadcast;
  bcastAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVALL;
  bcastAddr.panId = 0xFFFF;
  bcastAddr.endPoint = STUBAPS_INTER_PAN_EP;

  // Initialize ZLL common variables
  zll_InitVariables( TRUE );

  savedPollRate = POLL_RATE;
  savedQueuedPollRate = QUEUED_POLL_RATE;
  savedResponsePollRate = RESPONSE_POLL_RATE;

  numScanReqSent = 0;
  initiatorClearSelectedTarget();
  scanReqChannels = ZLL_SCAN_PRIMARY_CHANNELS;

  initiatorSeqNum = 0;

  // Register to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( zllInitiator_TaskID );

  // Register for ZLL Initiator callbacks (for Inter-PAN commands)
  zclLL_RegisterInterPANCmdCallbacks( &zllInitiator_CmdCBs );

  // Register for Initiator to receive Leave Confirm
  ZDO_RegisterForZdoCB( ZDO_LEAVE_CNF_CBID, initiatorZdoLeaveCnfCB );

#if (ZSTACK_ROUTER_BUILD)
  // Register to process ZDO messages
  ZDO_RegisterForZDOMsg( zllInitiator_TaskID, Mgmt_Permit_Join_req );
  ZDO_RegisterForZDOMsg( zllInitiator_TaskID, Device_annce );
#endif
}

/*********************************************************************
 * @fn      zllInitiator_RegisterApp
 *
 * @brief   Register an Application's EndPoint with the ZLL profile.
 *
 * @param   simpleDesc - application simple description
 * @param   pDeviceInfo - ZLL application device info
 *
 * @return  ZSuccess - Registered
 *          ZInvalidParameter - invalid or duplicate endpoint
 *          ZMemError - not enough memory to add
 */
ZStatus_t zllInitiator_RegisterApp( SimpleDescriptionFormat_t *simpleDesc, zclLLDeviceInfo_t *pDeviceInfo )
{
  return zll_RegisterApp( simpleDesc, pDeviceInfo );
}

/*********************************************************************
 * @fn      zllInitiator_RegisterForMsg
 *
 * @brief   Register application task to receive unprocessed messages
 *          received by the initiator endpoint.
 *
 * @param   taskId - task Id of the Application where commands will be sent to
 *
 * @return  ZSuccess if task registration successful
 *********************************************************************/
ZStatus_t zllInitiator_RegisterForMsg( uint8 taskId )
{
  if ( initiatorRegisteredMsgAppTaskID == TASK_NO_TASK )
  {
    initiatorRegisteredMsgAppTaskID = taskId;
    return ( ZSuccess );
  }
  return ( ZFailure );
}

/*********************************************************************
 * @fn          zllInitiator_event_loop
 *
 * @brief       Event Loop Processor for ZLL Initiator.
 *
 * @param       task_id - task id
 * @param       events - event bitmap
 *
 * @return      unprocessed events
 */
uint16 zllInitiator_event_loop( uint8 task_id, uint16 events )
{
  if ( events & SYS_EVENT_MSG )
  {
    osal_event_hdr_t *pMsg;
    ZStatus_t stat = ZFailure;

    if ( (pMsg = (osal_event_hdr_t *)osal_msg_receive( task_id )) != NULL )
    {
      switch ( pMsg->event )
      {
#if defined (MT_APP_FUNC)
        case MT_SYS_APP_MSG:
          // Message received from MT
          stat = initiatorProcessRpcMsg( ((mtSysAppMsg_t *)pMsg)->endpoint,
                                         ((mtSysAppMsg_t *)pMsg)->appDataLen,
                                         ((mtSysAppMsg_t *)pMsg)->appData );
          break;
#endif
#if (ZSTACK_ROUTER_BUILD)
        case ZDO_CB_MSG:
          // ZDO sends the message that we registered for
          zll_RouterProcessZDOMsg( (zdoIncomingMsg_t *)pMsg );
          stat = ZSuccess;
          break;
#endif
        case ZDO_STATE_CHANGE:
          initiatorProcessStateChange( (devStates_t)pMsg->status );
          stat = ZSuccess;
          break;

        default:
          break;
      }

      if ( stat == ZSuccess )
      {
        // Release the OSAL message
        VOID osal_msg_deallocate( (uint8 *)pMsg );
      }
      else
      {
        // forward to the application
        osal_msg_send( initiatorRegisteredMsgAppTaskID, (uint8 *)pMsg );
      }
    }

    // return unprocessed events
    return ( events ^ SYS_EVENT_MSG );
  }

  if ( events & ZLL_TL_SCAN_BASE_EVT )
  {
    if ( ( ( scanReqChannels == ZLL_SCAN_PRIMARY_CHANNELS ) && ( numScanReqSent < ZLL_INITIATOR_NUM_SCAN_REQ_PRIMARY  ) ) ||
         ( ( scanReqChannels == ZLL_SCAN_SECONDARY_CHANNELS ) && ( numScanReqSent < ZLL_INITIATOR_NUM_SCAN_REQ_EXTENDED ) ) )
    {
      // Send another Scan Request on the next channel
      initiatorSendScanReq( FALSE );
    }
    else // Channels scan is complete
    {
      if ( ( scanReqChannels == ZLL_SCAN_PRIMARY_CHANNELS )
          && ( zll_IsFactoryNew() || ( zllJoinedHANetwork == TRUE ) ) )
      {
        // Extended scan is required, lets scan secondary channels
        scanReqChannels = ZLL_SCAN_SECONDARY_CHANNELS;

        // Send another Scan Request on the next channel
        initiatorSendScanReq( FALSE );
      }
      // See if we've received any Scan Responses back
      else if ( ( selectedTarget.lastRssi != ZLL_TL_WORST_RSSI )
               && ( selectedTarget.scanRsp.deviceInfo.endpoint != DEV_INFO_INVALID_EP ) )
      {
        // Make sure the responder is not a factory new remote if this controller is
        if ( !zll_IsFactoryNew() || ( selectedTarget.scanRsp.zllLinkInitiator == FALSE ) )
        {
          zclLLIdentifyReq_t req;

          // Tune to the channel that the Scan Response was heard on
          zll_SetChannel( selectedTarget.rxChannel );

          req.transID = selectedTarget.scanRsp.transID;
          req.IdDuration = 0xffff;

          // Send an Identify Request first
          zclLL_Send_IndentifyReq( ZLL_INTERNAL_ENDPOINT, &(selectedTarget.srcAddr), &req, initiatorSeqNum++ );

          // Notify our task to configure the target (if needed)
          osal_start_timerEx( zllInitiator_TaskID, ZLL_CFG_TARGET_EVT, ZLL_INITIATOR_IDENTIFY_INTERVAL );
        }
        // else wait for touch-link commands from the other initiator
      }
      else
      {
        // We did not manage to select any target
        // Let's just go back to our initial configuration
        osal_set_event( zllInitiator_TaskID, ZLL_DISABLE_RX_EVT );
      }
    }

    // return unprocessed events
    return ( events ^ ZLL_TL_SCAN_BASE_EVT );
  }

  if ( events & ZLL_CFG_TARGET_EVT )
  {
    ZStatus_t status = ZFailure;

    zclLLIdentifyReq_t req;

    req.transID = selectedTarget.scanRsp.transID;
    req.IdDuration = 0x0;

    // Send an Identify stop Request
    zclLL_Send_IndentifyReq( ZLL_INTERNAL_ENDPOINT, &(selectedTarget.srcAddr), &req, initiatorSeqNum++ );

    // Configure the target
    if ( zll_IsFactoryNew() )
    {
      // verify address ranges split possible if required
      if ( !selectedTarget.scanRsp.zllAddressAssignment
          || zll_IsValidSplitFreeRanges( selectedTarget.scanRsp.totalGrpIDs ) )
      {
        // Must be the first light; ask the light to start the network
        status = initiatorSendNwkStartReq( &(selectedTarget.scanRsp) );
      }
    }
    else if ( !zllJoinedHANetwork || ( _NIB.nwkPanId == selectedTarget.scanRsp.PANID) )
    {
      // Tune to the channel that the Scan Response was heard on
      zll_SetChannel( selectedTarget.rxChannel );

      // See if the target is part of our network
      if ( !ZLL_SAME_NWK( selectedTarget.scanRsp.PANID, selectedTarget.scanRsp.extendedPANID ) )
      {
        // verify address ranges split possible if required
        if ( !selectedTarget.scanRsp.zllAddressAssignment
            || zll_IsValidSplitFreeRanges( selectedTarget.scanRsp.totalGrpIDs ) )
        {
          // Ask the target to join our network
          status = initiatorSendNwkJoinReq( &(selectedTarget.scanRsp) );
        }
      }
      else if ( _NIB.nwkUpdateId != selectedTarget.scanRsp.nwkUpdateId )
      {
        uint8 updateId = zll_NewNwkUpdateId( _NIB.nwkUpdateId, selectedTarget.scanRsp.nwkUpdateId );

        // Target is already part of our network
        if ( updateId != _NIB.nwkUpdateId )
        {
          // Update our network update id and logical channel
          zll_ProcessNwkUpdate( updateId, selectedTarget.scanRsp.logicalChannel );

          // Rejoin this network
          _NIB.nwkState = NWK_REJOINING;
          initiatorReJoinNwk( MODE_REJOIN );

          // We're done here
          status = ZSuccess;
        }
        else
        {
          // Inform the target to update its network update id and logical channel
          initiatorSendNwkUpdateReq( &(selectedTarget.scanRsp) ); // there's no corresponding response!

          // Notify the application about this device
          osal_set_event( zllInitiator_TaskID, ZLL_NOTIFY_APP_EVT );
        }
      }
      //we are touchlinking to a light in our network, just send application the device info
      else if ( selectedTarget.scanRsp.zllLinkInitiator == FALSE )
      {
        epInfoRec_t rec;
        rec.nwkAddr = selectedTarget.scanRsp.nwkAddr;
        rec.endpoint = selectedTarget.scanRsp.deviceInfo.endpoint;
        rec.profileID = selectedTarget.scanRsp.deviceInfo.profileID;
        rec.deviceID = selectedTarget.scanRsp.deviceInfo.deviceID;
        rec.version = selectedTarget.scanRsp.deviceInfo.version;
        // Notify the application
        if ( pfnNotifyAppCB )
        {
          (*pfnNotifyAppCB)( &rec );
        }
#if defined ( MT_APP_FUNC )
        MT_ZllNotifyTL( &rec );
#endif
      }
    }

    if ( status == ZSuccess )
    {
      // Wait for a response (if any), which will over write this
      rxRsp.nwkStartRsp.status = ZLL_NETWORK_START_RSP_STATUS_FAILURE;
    }

    // return unprocessed events
    return ( events ^ ZLL_CFG_TARGET_EVT );
  }

  if ( events & ZLL_W4_NWK_START_RSP_EVT )
  {
    zclLLNwkStartRsp_t *pRsp = &(rxRsp.nwkStartRsp);

    if ( ( pRsp->status == ZLL_NETWORK_START_RSP_STATUS_SUCCESS )
       && nwk_ExtPANIDValid( pRsp->extendedPANID ) )
    {
      // Copy the new network parameters to NIB
      zll_SetNIB( ( ZSTACK_ROUTER_BUILD ? NWK_ROUTER : NWK_REJOINING ),
                  _NIB.nwkDevAddress, pRsp->extendedPANID,
                  pRsp->logicalChannel, pRsp->panId, pRsp->nwkUpdateId );

      // Apply the received network key
      zll_DecryptNwkKey( encKeySent, keyIndexSent, pRsp->transID, responseIDSent );

      // This is not a usual Trust Center protected network
      ZDSecMgrUpdateTCAddress( NULL );

      // Notify the application about this device
      osal_set_event( zllInitiator_TaskID, ZLL_NOTIFY_APP_EVT );

      // Wait at least ZLL_APLC_MIN_STARTUP_DELAY_TIME seconds to allow the
      // target to start the network correctly. Join the target afterwards.
      osal_start_timerEx( zllInitiator_TaskID, ZLL_START_NWK_EVT, ZLL_APLC_MIN_STARTUP_DELAY_TIME );
    }

    // return unprocessed events
    return ( events ^ ZLL_W4_NWK_START_RSP_EVT );
  }

  if ( events & ZLL_START_NWK_EVT )
  {
    // Let's join the network started by the target
    initiatorJoinNwk();

    // return unprocessed events
    return ( events ^ ZLL_START_NWK_EVT );
  }

  if ( events & ZLL_W4_NWK_JOIN_RSP_EVT )
  {
    zclLLNwkJoinRsp_t *pRsp = &(rxRsp.nwkJoinRsp);

    if ( pRsp->status == ZLL_NETWORK_JOIN_RSP_STATUS_SUCCESS )
    {
      // Target has joined our network

      // Wait at least ZLL_APLC_MIN_STARTUP_DELAY_TIME seconds to allow the
      // target to start operating on the network correctly. Notify the
      // application afterwards.
      osal_start_timerEx( zllInitiator_TaskID, ZLL_NOTIFY_APP_EVT,
                          ZLL_APLC_MIN_STARTUP_DELAY_TIME );
    }

    // We're done with touch-link procedure here
    initiatorSetNwkToInitState();

    zll_UpdateNV( ZLL_UPDATE_NV_RANGES );

    if ( ( POLL_RATE == 0 ) && ( selectedTarget.scanRsp.zLogicalType == ZG_DEVICETYPE_ENDDEVICE ) )
    {
      //allow to respond to ZLL commission utility commands after TL
      NLME_SetPollRate( ZLL_INITIATOR_TEMP_POST_TL_POLL_RATE );
      //polling should reset when TL life time expires
    }

    // return unprocessed events
    return ( events ^ ZLL_W4_NWK_JOIN_RSP_EVT );
  }

  if ( events & ZLL_ENDDEVICE_JOIN_EVT )
  {
    // If not factory new, perform a Leave on our old network
    if ( !zll_IsFactoryNew() && ( zll_SendLeaveReq() == ZSuccess ) )
    {
      // Wait for Leave confirmation before joining the new network
      zllLeaveInitiated = ZLL_LEAVE_TO_JOIN_NWK;
    }
    else
    {
      // Notify our task to join this network
      osal_set_event( zllInitiator_TaskID, ZLL_JOIN_NWK_EVT );
    }

    // return unprocessed events
    return ( events ^ ZLL_ENDDEVICE_JOIN_EVT );
  }

  if ( events & ZLL_JOIN_NWK_EVT )
  {
    if ( joinReq.nwkParams.nwkAddr != INVALID_NODE_ADDR ) // update only if valid, otherwise use our own values.
    {
      zclLLNwkParams_t *pParams = &(joinReq.nwkParams);

      // Copy the new network parameters to the NIB
      zll_SetNIB( ( ZSTACK_ROUTER_BUILD ? NWK_ROUTER : NWK_REJOINING ),
                  pParams->nwkAddr, pParams->extendedPANID,
                  pParams->logicalChannel, pParams->panId, joinReq.nwkUpdateId );

      // Apply the received network key
      zll_DecryptNwkKey( pParams->nwkKey, pParams->keyIndex, joinReq.transID, zllResponseID );

      zll_UpdateFreeRanges( pParams );
    }

    // We're asked to join a new network; let's do it!
    initiatorJoinNwk();

    // return unprocessed events
    return ( events ^ ZLL_JOIN_NWK_EVT );
  }

  if ( events & ZLL_DISABLE_RX_EVT )
  {
    // We're not asked to join a network
    initiatorSetNwkToInitState();

    scanReqChannels = ZLL_SCAN_PRIMARY_CHANNELS;
    numScanReqSent = 0;
    // Reset selected target
    initiatorClearSelectedTarget();

    // return unprocessed events
    return ( events ^ ZLL_DISABLE_RX_EVT );
  }

  if ( events & ZLL_W4_REJOIN_EVT )
  {
    // Stop joining cycle
    ZDApp_StopJoiningCycle();

    // return unprocessed events
    return ( events ^ ZLL_W4_REJOIN_EVT );
  }

  if ( events & ZLL_NOTIFY_APP_EVT )
  {
    if ( selectedTarget.lastRssi > ZLL_TL_WORST_RSSI )
    {
      epInfoRec_t rec;
      rec.nwkAddr = selectedTarget.newNwkAddr; // newly assigned network address
      rec.endpoint = selectedTarget.scanRsp.deviceInfo.endpoint;
      rec.profileID = selectedTarget.scanRsp.deviceInfo.profileID;
      rec.deviceID = selectedTarget.scanRsp.deviceInfo.deviceID;
      rec.version = selectedTarget.scanRsp.deviceInfo.version;
      // Notify the application
      if ( pfnNotifyAppCB )
      {
        (*pfnNotifyAppCB)( &rec );
      }
#if defined ( MT_APP_FUNC )
      MT_ZllNotifyTL( &rec );
#endif
    }
    // return unprocessed events
    return ( events ^ ZLL_NOTIFY_APP_EVT );
  }

  if ( events & ZLL_RESET_TO_FN_EVT )
  {
    zllInitiator_ResetToFactoryNew();

    // return unprocessed events
    return ( events ^ ZLL_RESET_TO_FN_EVT );
  }

  if ( events & ZLL_NWK_DISC_CNF_EVT )
  {
    if ( zllHAScanInitiated && ( zllHAScanInitiated != ZLL_SCAN_FOUND_NOTHING ) )
    {
      if ( ( zll_ClassicalCommissioningJoinDiscoveredNwk() == ZSuccess) &&
           ( pfnResetAppCB ) )
      {
        pfnResetAppCB();
      }
    }
    // return unprocessed events
    return ( events ^ ZLL_NWK_DISC_CNF_EVT );
  }

  if ( events & ZLL_TRANS_LIFETIME_EXPIRED_EVT )
  {
    zllTransID = 0;
    initiatorClearSelectedTarget();
    initiatorSetNwkToInitState();

    // return unprocessed events
    return ( events ^ ZLL_TRANS_LIFETIME_EXPIRED_EVT );
  }

  // If reach here, the events are unknown
  // Discard or make more handlers
  return 0;
}

/*********************************************************************
 * @fn      initiatorJoinNwk
 *
 * @brief   Initiate a network join request.
 *
 * @param   void
 *
 * @return  void
 */
static void initiatorJoinNwk( void )
{
  // Save free ranges
  zll_UpdateNV( ZLL_UPDATE_NV_RANGES );

  // In case we're here after a leave
  zllLeaveInitiated = FALSE;

  // Clear leave control logic
  ZDApp_LeaveCtrlReset();

  if ( POLL_RATE == 0 )
  {
    //allow to respond to ZLL commission utility commands after TL
    NLME_SetPollRate( ZLL_INITIATOR_TEMP_POST_TL_POLL_RATE );
    //polling should reset when TL life time expires
  }

#if ( ZSTACK_ROUTER_BUILD )
  zllInitiator_PermitJoin( 0 );
  ZDOInitDevice( 0 );
#else
  // Perform a network rejoin request
  initiatorReJoinNwk( MODE_REJOIN );
#endif
}

/*********************************************************************
 * @fn      initiatorClearSelectedTarget
 *
 * @brief   clear selected target variable.
 *
 * @param   none
 *
 * @return  none
 */
static void initiatorClearSelectedTarget( void )
{
  osal_memset( &selectedTarget, 0x00, sizeof(targetCandidate_t) );
  selectedTarget.lastRssi = ZLL_TL_WORST_RSSI;
}

/*********************************************************************
 * @fn      initiatorSelectNwkParams
 *
 * @brief   Select a unique PAN ID and Extended PAN ID when compared to
 *          the PAN IDs and Extended PAN IDs of the networks detected
 *          on the ZLL channels. The selected Extended PAN ID must be
 *          a random number (and not equal to our IEEE address).
 *
 * @param   void
 *
 * @return  void
 */
void initiatorSelectNwkParams( void )
{
  // Set our group ID range
  zll_PopGrpIDRange( zll_GetNumGrpIDs(), &zllGrpIDsBegin, &zllGrpIDsEnd );

  // Select a random Extended PAN ID
  zll_GenerateRandNum( _NIB.extendedPANID, Z_EXTADDR_LEN );

  // Select a random PAN ID
  _NIB.nwkPanId = osal_rand();

  // Select a random ZLL channel
  _NIB.nwkLogicalChannel = zll_GetRandPrimaryChannel();

  if ( devState != DEV_INIT ) // exclude in case of fresh FN ZED without HOLD_AUTO_START
  {
    // Let's assume we're the first initiator
    _NIB.nwkDevAddress = zll_PopNwkAddress();
  }

  // Configure MAC with our network parameters
  zll_SetMacNwkParams( _NIB.nwkDevAddress, _NIB.nwkPanId, _NIB.nwkLogicalChannel );
}

/*********************************************************************
 * @fn      initiatorSetNwkToInitState
 *
 * @brief   Set our network state to its original state.
 *
 * @param   void
 *
 * @return  void
 */
static void initiatorSetNwkToInitState()
{
  // Turn MAC receiver back to its old state
  ZMacSetReq( ZMacRxOnIdle, &savedRxOnIdle );

  // Tune back to our channel
  zll_SetChannel( _NIB.nwkLogicalChannel );

  // Set NWK task to run
  nwk_setStateIdle( FALSE );

  if ( savedPollRate != zgPollRate )
  {
    NLME_SetPollRate( savedPollRate );
    savedPollRate = POLL_RATE;
  }

  if ( savedQueuedPollRate != zgQueuedPollRate )
  {
    NLME_SetQueuedPollRate( savedQueuedPollRate );
    savedQueuedPollRate = QUEUED_POLL_RATE;
  }

  if ( savedResponsePollRate != zgResponsePollRate )
  {
    NLME_SetResponseRate( savedResponsePollRate );
    savedResponsePollRate = RESPONSE_POLL_RATE;
  }
}

/*********************************************************************
 * @fn      initiatorSendScanReq
 *
 * @brief   Send out an Scan Request command on one of the ZLL channels.
 *
 * @param   freshScan - TRUE to start fresh scan, FALSE to resume existing process.
 *
 * @return  void
 */
static void initiatorSendScanReq( bool freshScan )
{
  zclLLScanReq_t req;
  uint8 newChannel;
  uint8 secondaryChList[] = ZLL_SECONDARY_CHANNELS_SET;
  static uint8 channelIndex = 0;

  if ( freshScan )
  {
    channelIndex = 0;
  }

  // First figure out the channel
  if ( scanReqChannels == ZLL_SCAN_PRIMARY_CHANNELS )
  {
    if ( numScanReqSent < 5 )
    {
      // First five consecutive requests are sent on channel 11
      newChannel = ZLL_FIRST_CHANNEL;
    }
    else if ( numScanReqSent == 5 )
    {
      // Sixth request is sent on channel 15
      newChannel = ZLL_SECOND_CHANNEL;
    }
    else if ( numScanReqSent == 6 )
    {
      // Seventh request is sent on channel 20
      newChannel = ZLL_THIRD_CHANNEL;
    }
    else
    {
      // Last request is sent on channel 25
      newChannel = ZLL_FOURTH_CHANNEL;
    }
  }
  else
  {
    // scan secondary channel list
    if ( channelIndex < sizeof(secondaryChList) )
    {
       newChannel = secondaryChList[channelIndex++];
    }
    else
    {
      // set it to initial value for next discovery process
      channelIndex = 0;
      return;
    }
  }

  if ( zllTransID != 0 )
  {
    // Build the request
    req.transID = zllTransID;

    req.zInfo.zInfoByte = 0;
    req.zLogicalType = zgDeviceLogicalType;
    if ( ZDO_Config_Node_Descriptor.CapabilityFlags & CAPINFO_RCVR_ON_IDLE )
    {
      req.zRxOnWhenIdle = TRUE;
    }

    req.zllInfo.zllInfoByte = 0;
    req.zllFactoryNew = zll_IsFactoryNew();
    req.zllAddressAssignment = TRUE;
    req.zllLinkInitiator = TRUE;

    // First switch to the right channel
    zll_SetChannel( newChannel );

    // Broadcast the request
    zclLL_Send_ScanReq( ZLL_INTERNAL_ENDPOINT, &bcastAddr, &req, initiatorSeqNum++ );

    numScanReqSent++;

    // After each transmission, wait ZLL_APLC_SCAN_TIME_BASE_DURATION seconds
    // to receive any responses.
    osal_start_timerEx( zllInitiator_TaskID, ZLL_TL_SCAN_BASE_EVT, ZLL_APLC_SCAN_TIME_BASE_DURATION );
  }
  else
  {
    zllInitiator_AbortTL();
  }
}

/*********************************************************************
 * @fn      initiatorSendNwkStartReq
 *
 * @brief   Send out a Network Start Request command.
 *
 * @param   pRsp - received Scan Response
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorSendNwkStartReq( zclLLScanRsp_t *pRsp )
{
  zclLLNwkStartReq_t *pReq;
  ZStatus_t status;

  pReq = (zclLLNwkStartReq_t *)osal_mem_alloc( sizeof( zclLLNwkStartReq_t ) );
  if ( pReq != NULL )
  {
    uint16 i;
    zclLLNwkParams_t *pParams = &(pReq->nwkParams);

    osal_memset( pReq, 0, sizeof( zclLLNwkStartReq_t ) );

    // Build the request
    pReq->transID = selectedTarget.scanRsp.transID;

    // Find out key index (prefer highest)
    for ( i = 15; i > 0; i-- )
    {
      if ( ( (uint16)1 << i ) & pRsp->keyBitmask )
      {
        break;
      }
    }
    pParams->keyIndex = i;

    // Copy in the encrypted network key
    zll_EncryptNwkKey( pParams->nwkKey, i, pRsp->transID, pRsp->responseID );

    pParams->nwkAddr = zll_PopNwkAddress();
    if ( pParams->nwkAddr == 0 )
    {
      pParams->nwkAddr = osal_rand();
    }
    // update address for app notification
    selectedTarget.newNwkAddr = pParams->nwkAddr;

    // Set group ID range
    if ( pRsp->totalGrpIDs > 0 )
    {
      zll_PopGrpIDRange( pRsp->totalGrpIDs, &(pParams->grpIDsBegin), &(pParams->grpIDsEnd) );
    }

    if ( pRsp->zllAddressAssignment )
    {
      zll_SplitFreeRanges( &(pParams->freeNwkAddrBegin), &(pParams->freeNwkAddrEnd),
                           &(pParams->freeGrpIDBegin), &(pParams->freeGrpIDEnd) );
    }

#ifdef ZLL_INITIATOR_SET_NEW_NWK_PARAMS
    pParams->logicalChannel = _NIB.nwkLogicalChannel;
    pParams->panId = _NIB.nwkPanId;
    osal_memcpy( pParams->extendedPANID, _NIB.extendedPANID ,Z_EXTADDR_LEN);
#endif

    osal_cpyExtAddr( pReq->initiatorIeeeAddr, NLME_GetExtAddr() );
    pReq->initiatorNwkAddr = _NIB.nwkDevAddress;;

    status = zclLL_Send_NwkStartReq( ZLL_INTERNAL_ENDPOINT, &(selectedTarget.srcAddr), pReq, initiatorSeqNum++ );
    if ( status == ZSuccess )
    {
      // Keep a copy of the encryted network key sent to the target
      keyIndexSent = i;
      osal_memcpy( encKeySent, pParams->nwkKey, SEC_KEY_LEN );
      responseIDSent = pRsp->responseID;

      // After the transmission, wait ZLL_APLC_RX_WINDOW_DURATION seconds to
      // receive a response.
      osal_start_timerEx( zllInitiator_TaskID, ZLL_W4_NWK_START_RSP_EVT, ZLL_APLC_RX_WINDOW_DURATION );
    }

    osal_mem_free( pReq );
  }
  else
  {
    status = ZMemError;
  }

  return ( status );
}

/*********************************************************************
 * @fn      initiatorSendNwkJoinReq
 *
 * @brief   Send out a Network Join Router or End Device Request command.
 *
 * @param   pRsp - received Scan Response
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorSendNwkJoinReq( zclLLScanRsp_t *pRsp )
{
  zclLLNwkJoinReq_t *pReq;
  ZStatus_t status;

  pReq = (zclLLNwkJoinReq_t *)osal_mem_alloc( sizeof( zclLLNwkJoinReq_t ) );
  if ( pReq != NULL )
  {
    uint16 i;
    zclLLNwkParams_t *pParams = &(pReq->nwkParams);

    osal_memset( pReq, 0, sizeof( zclLLNwkJoinReq_t ) );

    // Build the request
    pReq->transID = selectedTarget.scanRsp.transID;

    // Find out key index (prefer highest)
    for ( i = 15; i > 0; i-- )
    {
      if ( ( (uint16)1 << i ) & pRsp->keyBitmask )
      {
        break;
      }
    }
    pParams->keyIndex = i;

    // Copy in the encrypted network key
    zll_EncryptNwkKey( pParams->nwkKey, i, pRsp->transID, pRsp->responseID );

    pParams->nwkAddr = zll_PopNwkAddress();
    if ( pParams->nwkAddr == 0 )
    {
      pParams->nwkAddr = osal_rand();
    }
    // update address for app notification
    selectedTarget.newNwkAddr = pParams->nwkAddr;

    // Set group ID range
    if ( pRsp->totalGrpIDs > 0 )
    {
      zll_PopGrpIDRange( pRsp->totalGrpIDs, &(pParams->grpIDsBegin), &(pParams->grpIDsEnd) );
    }

    if ( pRsp->zllAddressAssignment )
    {
      zll_SplitFreeRanges( &(pParams->freeNwkAddrBegin), &(pParams->freeNwkAddrEnd),
                           &(pParams->freeGrpIDBegin), &(pParams->freeGrpIDEnd) );
    }

    pParams->logicalChannel = _NIB.nwkLogicalChannel;
    pParams->panId = _NIB.nwkPanId;
    osal_cpyExtAddr( pParams->extendedPANID, _NIB.extendedPANID );
    pReq->nwkUpdateId = _NIB.nwkUpdateId;

    // Let PAN ID, Extended PAN ID and Logical Channel to be determined by the target
    if ( pRsp->zLogicalType == ZG_DEVICETYPE_ROUTER )
    {
      // It's a light
      status = zclLL_Send_NwkJoinRtrReq( ZLL_INTERNAL_ENDPOINT, &(selectedTarget.srcAddr), pReq, initiatorSeqNum++ );
    }
    else // another controller
    {
      status = zclLL_Send_NwkJoinEDReq( ZLL_INTERNAL_ENDPOINT, &(selectedTarget.srcAddr), pReq, initiatorSeqNum++ );
    }

    if ( status == ZSuccess )
    {
      // After the transmission, wait ZLL_APLC_RX_WINDOW_DURATION seconds to
      // receive a response.
      osal_start_timerEx( zllInitiator_TaskID, ZLL_W4_NWK_JOIN_RSP_EVT, ZLL_APLC_RX_WINDOW_DURATION );
    }

    osal_mem_free( pReq );
  }
  else
  {
    status = ZMemError;
  }

  return ( status );
}

/*********************************************************************
 * @fn      initiatorSendNwkUpdateReq
 *
 * @brief   Send out a Network Update Request command.
 *
 * @param   pRsp - received Scan Response
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorSendNwkUpdateReq( zclLLScanRsp_t *pRsp )
{
  zclLLNwkUpdateReq_t *pReq;
  ZStatus_t status;

  pReq = (zclLLNwkUpdateReq_t *)osal_mem_alloc( sizeof( zclLLNwkUpdateReq_t ) );
  if ( pReq!= NULL )
  {
    // Build the request
    pReq->transID = pRsp->transID;
    osal_cpyExtAddr( pReq->extendedPANID, _NIB.extendedPANID );
    pReq->nwkUpdateId = _NIB.nwkUpdateId;
    pReq->logicalChannel = _NIB.nwkLogicalChannel;
    pReq->PANID = _NIB.nwkPanId;
    pReq->nwkAddr = pRsp->nwkAddr;

    status = zclLL_Send_NwkUpdateReq( ZLL_INTERNAL_ENDPOINT, &(selectedTarget.srcAddr), pReq, initiatorSeqNum++ );

    osal_mem_free( pReq );
  }
  else
  {
    status = ZMemError;
  }

  return ( status );
}

//#define ZLL_ORPHAN_JOIN // Uncomment this line for Orphan Joining

/*********************************************************************
 * @fn      initiatorReJoinNwk
 *
 * @brief   Send out an Rejoin Request or Orphan Join Request command
 *          depending on the Start Mode.
 *
 * @param   startMode - MODE_REJOIN or MODE_RESUME
 *
 * @return  none
 */
static void initiatorReJoinNwk( devStartModes_t startMode )
{
  // Set NWK task to run
  nwk_setStateIdle( FALSE );

  // Configure MAC with our network parameters
  zll_SetMacNwkParams( _NIB.nwkDevAddress, _NIB.nwkPanId, _NIB.nwkLogicalChannel );

  // Use the new network paramters
  zgConfigPANID = _NIB.nwkPanId;
  zgDefaultChannelList = _NIB.channelList;
  osal_cpyExtAddr( ZDO_UseExtendedPANID, _NIB.extendedPANID );

#ifdef ZLL_ORPHAN_JOIN
  devStartMode = MODE_RESUME;
#else
  devStartMode = startMode;
#endif

  _tmpRejoinState = TRUE;

  // Start the network joining process
  osal_set_event( ZDAppTaskID, ZDO_NETWORK_INIT );
}

/*********************************************************************
 * @fn      initiatorScanReqCB
 *
 * @brief   This callback is called to process a Scan Request command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 * @param   seqNum - command sequence number
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorScanReqCB( afAddrType_t *srcAddr, zclLLScanReq_t *pReq, uint8 seqNum )
{
  int8 rssi;
  rssi = zll_GetMsgRssi();
  if( ( rssi > ZLL_TL_WORST_RSSI ) && ( pReq->zllLinkInitiator == TRUE ) )
  {
    // response to the originator, but switch to dst PAN 0xFFFF
    afAddrType_t dstAddr;
    osal_memcpy(&dstAddr, srcAddr, sizeof(afAddrType_t));
    dstAddr.panId = 0xFFFF;

    // If, during its scan, a non factory new initiator receives another scan
    // request inter-PAN command frame from a factory new target, it shall be ignored.
    if ( ( pReq->zllFactoryNew == TRUE ) && !zll_IsFactoryNew() &&
         osal_get_timeoutEx( zllInitiator_TaskID, ZLL_TL_SCAN_BASE_EVT ) )
    {
      return ( ZSuccess );
    }

    // Send a Scan Response back
    if ( zll_SendScanRsp( ZLL_INTERNAL_ENDPOINT, &dstAddr, pReq->transID, seqNum ) == ZSuccess )
    {
      // If we're a factory new initiator and are in the middle of a Device
      // Discovery, stop the procedure and wait for subsequent configuration
      // information from the non-factory new initiator that we just responded to.
      if ( zll_IsFactoryNew() && !pReq->zllFactoryNew )
      {
        osal_stop_timerEx( zllInitiator_TaskID, ZLL_TL_SCAN_BASE_EVT );

      }
    }
  }

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      initiatorDeviceInfoReqCB
 *
 * @brief   This callback is called to process a Device Information
 *          Request command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 * @param   seqNum - command sequence number
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorDeviceInfoReqCB( afAddrType_t *srcAddr, zclLLDeviceInfoReq_t *pReq, uint8 seqNum )
{
  if ( zll_IsValidTransID( pReq->transID ) == FALSE )
  {
    return ( ZFailure );
  }
  return ( zll_SendDeviceInfoRsp( ZLL_INTERNAL_ENDPOINT, srcAddr,
                                  pReq->startIndex, pReq->transID, seqNum ) );
}

/*********************************************************************
 * @fn      initiatorIdentifyReqCB
 *
 * @brief   This callback is called to process an Identify Request command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorIdentifyReqCB( afAddrType_t *srcAddr, zclLLIdentifyReq_t *pReq )
{
  if ( zll_IsValidTransID( pReq->transID ) == FALSE )
  {
    return ( ZFailure );
  }
  // The target should identify itself
  if ( pfnIdentifyCB != NULL )
  {
    zclIdentify_t cmd;

    cmd.srcAddr = srcAddr;

    // Values of the Identify Duration field:
    // - Exit identify mode: 0x0000
    // - Length of time to remain in identify mode: 0x0001-0xfffe
    // - Remain in identify mode for a default time known by the receiver: 0xffff
    if ( pReq->IdDuration == 0xffff )
    {
      cmd.identifyTime = ZLL_DEFAULT_IDENTIFY_TIME;
    }
    else
    {
      cmd.identifyTime = pReq->IdDuration;
    }

    (*pfnIdentifyCB)( &cmd );
  }

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      initiatorResetToFNReqCB
 *
 * @brief   This callback is called to process a Reset to Factory New
 *          Request command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorResetToFNReqCB( afAddrType_t *srcAddr, zclLLResetToFNReq_t *pReq )
{
  // If factory new, discard the request
  if ( ( zll_IsValidTransID( pReq->transID ) == FALSE ) || zll_IsFactoryNew() )
  {
    return ( ZFailure );
  }

  osal_set_event( zllInitiator_TaskID, ZLL_RESET_TO_FN_EVT );
  return ( ZSuccess );
}

/*********************************************************************
 * @fn      initiatorNwkJoinReqCB
 *
 * @brief   This callback is called to process Network Join End Device
 *          Request and Network Join End Device Request commands.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 * @param   seqNum - command sequence number
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorNwkJoinReqCB( afAddrType_t *srcAddr, zclLLNwkJoinReq_t *pReq, uint8 seqNum )
{
  zclLLNwkJoinRsp_t rsp;
  afAddrType_t dstAddr;

  if ( zll_IsValidTransID( pReq->transID ) == FALSE )
  {
    return ( ZFailure );
  }

  rsp.transID = pReq->transID;

  if ( nwk_ExtPANIDValid( pReq->nwkParams.extendedPANID ) )
    //NOTE: additional nwk params verification may be added here, e.g. ranges.
  {
    // Save the request for later
    joinReq = *pReq;

    // Notify our task to join the new network
    osal_set_event( zllInitiator_TaskID, ZLL_ENDDEVICE_JOIN_EVT );

    osal_stop_timerEx( zllInitiator_TaskID, ZLL_DISABLE_RX_EVT );
    osal_stop_timerEx( zllInitiator_TaskID, ZLL_CFG_TARGET_EVT );

    rsp.status = ZLL_NETWORK_JOIN_RSP_STATUS_SUCCESS;
  }
  else
  {
    rsp.status = ZLL_NETWORK_JOIN_RSP_STATUS_FAILURE;
  }

  dstAddr = *srcAddr;
  dstAddr.panId = 0xFFFF;

  // Send a response back
#if ( ZSTACK_ROUTER_BUILD )
  zclLL_Send_NwkJoinRtrRsp( ZLL_INTERNAL_ENDPOINT, &dstAddr, &rsp, seqNum );
#else
  zclLL_Send_NwkJoinEDRsp( ZLL_INTERNAL_ENDPOINT, &dstAddr, &rsp, seqNum );
#endif

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      initiatorScanReqCB
 *
 * @brief   This callback is called to process a Scan Response command.
 *
 * @param   srcAddr - sender's address
 * @param   pRsp - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorScanRspCB( afAddrType_t *srcAddr, zclLLScanRsp_t *pRsp )
{
  if ( osal_get_timeoutEx( zllInitiator_TaskID, ZLL_TL_SCAN_BASE_EVT )
       && ( zll_IsValidTransID( pRsp->transID ) )
       && ( pRsp->keyBitmask & zll_GetNwkKeyBitmask() ) )
  {
    uint8 selectThisTarget = FALSE;
    int8 rssi = zll_GetMsgRssi();
    if ( pfnSelectDiscDevCB != NULL )
    {
      selectThisTarget = pfnSelectDiscDevCB( pRsp, rssi );
    }
    // Default selection - according to RSSI
    else if ( rssi > ZLL_TL_WORST_RSSI )
    {
      if ( ( rssi + pRsp->rssiCorrection ) > selectedTarget.lastRssi )
      {
        // Better RSSI, select this target
        selectThisTarget = TRUE;
      }
    }

    if ( selectThisTarget )
    {
      selectedTarget.scanRsp = *pRsp;
      selectedTarget.lastRssi = rssi;
      selectedTarget.srcAddr = *srcAddr;
      selectedTarget.srcAddr.panId = 0xFFFF;

      // Remember channel we heard back this scan response on
      ZMacGetReq( ZMacChannel, &(selectedTarget.rxChannel));

      if ( pRsp->numSubDevices > 1 )
      {
        selectedTarget.scanRsp.deviceInfo.endpoint = DEV_INFO_INVALID_EP;

        zclLLDeviceInfoReq_t devInfoReq;
        devInfoReq.transID = pRsp->transID;
        devInfoReq.startIndex = 0;
        return zclLL_Send_DeviceInfoReq( ZLL_INTERNAL_ENDPOINT, srcAddr,
                                    &devInfoReq, initiatorSeqNum++ );
      }
    }
    return ( ZSuccess );
  }

  return ( ZFailure );
}

/*********************************************************************
 * @fn      initiatorDeviceInfoRspCB
 *
 * @brief   This callback is called to process a Device Information
 *          Response command.
 *          If sub-device is selected, selectedTarget data is updated.
 *
 * @param   srcAddr - sender's address
 * @param   pRsp - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorDeviceInfoRspCB( afAddrType_t *srcAddr, zclLLDeviceInfoRsp_t *pRsp )
{
  if ( zll_IsValidTransID( pRsp->transID )
       && ( srcAddr->addr.shortAddr == selectedTarget.srcAddr.addr.shortAddr ) )
  {
    uint8 selectedIdx = pRsp->cnt;
    zclLLScanRsp_t tmpInfo = selectedTarget.scanRsp;

    for (uint8 i = 0; i < pRsp->cnt; ++i )
    {
      if ( pfnSelectDiscDevCB != NULL )
      {
        tmpInfo.deviceInfo = pRsp->devInfoRec[i].deviceInfo;
        if ( pfnSelectDiscDevCB( &tmpInfo, selectedTarget.lastRssi ) )
        {
          selectedIdx = i;
          // no break here to allow cycling through all sub-devices
        }
      }
      else
      {
        if ( pRsp->devInfoRec[i].deviceInfo.profileID == ZLL_PROFILE_ID )
        {
          selectedIdx = i;
          break; // first match
        }
      }
    }
    if ( selectedIdx < pRsp->cnt )
    {
      // NOTE - the original scan response device info is overwritten with the
      // selected sub-device info, to complete the data required for the application.
      selectedTarget.scanRsp.deviceInfo = pRsp->devInfoRec[selectedIdx].deviceInfo;
    }
    else
    {
      // no sub-device of the currently selected device was selected.
      // clear selection
      initiatorClearSelectedTarget();
    }
    return ( ZSuccess );
  }
  return ( ZFailure );
}

/*********************************************************************
 * @fn      initiatorNwkStartRspCB
 *
 * @brief   This callback is called to process a Network Start Response command.
 *
 * @param   srcAddr - sender's address
 * @param   pRsp - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorNwkStartRspCB( afAddrType_t *srcAddr, zclLLNwkStartRsp_t *pRsp )
{
  if ( zll_IsValidTransID( pRsp->transID ) == FALSE )
  {
    return ( ZFailure );
  }
  // Make sure we didn't timeout waiting for this response
  if ( osal_get_timeoutEx( zllInitiator_TaskID, ZLL_W4_NWK_START_RSP_EVT ) )
  {
    // Save the Network Start Response for later
    rxRsp.nwkStartRsp = *pRsp;

    // No need to wait longer
    osal_start_timerEx( zllInitiator_TaskID, ZLL_W4_NWK_START_RSP_EVT, 0 );
  }

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      initiatorNwkJoinRtrRspCB
 *
 * @brief   This callback is called to process a Network Join Router
 *          Response command.
 *
 * @param   srcAddr - sender's address
 * @param   pRsp - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorNwkJoinRtrRspCB( afAddrType_t *srcAddr, zclLLNwkJoinRsp_t *pRsp )
{
  if ( zll_IsValidTransID( pRsp->transID ) == FALSE )
  {
    return ( ZFailure );
  }
  // Make sure we didn't timeout waiting for this response
  if ( osal_get_timeoutEx( zllInitiator_TaskID, ZLL_W4_NWK_JOIN_RSP_EVT ) )
  {
    // Save the Network Start Response for later
    rxRsp.nwkJoinRsp = *pRsp;

    // No need to wait longer
    osal_start_timerEx( zllInitiator_TaskID, ZLL_W4_NWK_JOIN_RSP_EVT, 0 );
  }

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      initiatorNwkJoinEDRspCB
 *
 * @brief   This callback is called to process a Network Join End Device
 *          Response command.
 *
 * @param   srcAddr - sender's address
 * @param   pRsp - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorNwkJoinEDRspCB( afAddrType_t *srcAddr, zclLLNwkJoinRsp_t *pRsp )
{
  if ( zll_IsValidTransID( pRsp->transID ) == FALSE )
  {
    return ( ZFailure );
  }
  // Make sure we didn't timeout waiting for this response
  if ( osal_get_timeoutEx( zllInitiator_TaskID, ZLL_W4_NWK_JOIN_RSP_EVT ) )
  {
    // Save the Network Start Response for later
    rxRsp.nwkJoinRsp = *pRsp;

    // No need to wait longer
    osal_start_timerEx( zllInitiator_TaskID, ZLL_W4_NWK_JOIN_RSP_EVT, 0 );

  }
  else
  {
    rxRsp.nwkJoinRsp.status = ZLL_NETWORK_JOIN_RSP_STATUS_FAILURE;
  }

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      initiatorNwkUpdateReqCB
 *
 * @brief   This callback is called to process a Network Update Request
 *          command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t initiatorNwkUpdateReqCB( afAddrType_t *srcAddr, zclLLNwkUpdateReq_t *pReq )
{
  if ( zll_IsValidTransID( pReq->transID ) == FALSE )
  {
    return ( ZFailure );
  }
  // Discard the request if the Extended PAN ID and PAN ID are not
  // identical with our corresponding stored values
  if ( ZLL_SAME_NWK( pReq->PANID, pReq->extendedPANID ) )
  {
    uint8 newUpdateId = zll_NewNwkUpdateId( pReq->nwkUpdateId, _NIB.nwkUpdateId);
    if ( _NIB.nwkUpdateId != newUpdateId )
    {
      // Update the network update id and logical channel
      zll_ProcessNwkUpdate( pReq->nwkUpdateId, pReq->logicalChannel );
    }
  }

  return ( ZSuccess );
}

/******************************************************************************
 * @fn      initiatorZdoLeaveCnfCB
 *
 * @brief   This callback is called to process a Leave Confirmation message.
 *
 *          Note: this callback function returns a pointer if it has handled
 *                the confirmation message and no further action should be
 *                taken with it. It returns NULL if it has not handled the
 *                confirmation message and normal processing should take place.
 *
 * @param   pParam - received message
 *
 * @return  Pointer if message processed. NULL, otherwise.
 */
static void *initiatorZdoLeaveCnfCB( void *pParam )
{
  (void)pParam;

  // Did we initiate the leave?
  if ( zllLeaveInitiated == FALSE )
  {
    return ( NULL );
  }

  if ( zllLeaveInitiated == ZLL_LEAVE_TO_JOIN_NWK )
  {
    // Notify our task to join the new network
    osal_set_event( zllInitiator_TaskID, ZLL_JOIN_NWK_EVT );
  }

  return ( (void *)&zllLeaveInitiated );
}

/*********************************************************************
 * @fn      zllInitiator_ChannelChange
 *
 * @brief   Change channel to supprot Frequency agility.
 *
 * @param   targetChannel - channel to
 *
 * @return  status
 */
ZStatus_t zllInitiator_ChannelChange( uint8 targetChannel )
{
    uint32 channelMask;
    zAddrType_t dstAddr = {0};
    if ( ( targetChannel < 11 ) || targetChannel > 26 )
    {
      if (ZLL_PRIMARY_CHANNEL (_NIB.nwkLogicalChannel))
      {
        switch (_NIB.nwkLogicalChannel)
        {
        case ZLL_FIRST_CHANNEL:
          targetChannel = ZLL_SECOND_CHANNEL;
          break;
        case ZLL_SECOND_CHANNEL:
          targetChannel = ZLL_THIRD_CHANNEL;
          break;
        case ZLL_THIRD_CHANNEL:
          targetChannel = ZLL_FOURTH_CHANNEL;
          break;
        case ZLL_FOURTH_CHANNEL:
          targetChannel = ZLL_FIRST_CHANNEL;
        }
      }
      else
      {
        targetChannel = _NIB.nwkLogicalChannel + 1;
        if ( _NIB.nwkLogicalChannel > 26 )
          targetChannel = 11;
      }
    }

    dstAddr.addrMode = AddrBroadcast;
    dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVRXON;
    channelMask = (uint32)1 << targetChannel;

    // Increment the nwkUpdateId parameter and set the updateID in the beacon
    NLME_SetUpdateID(_NIB.nwkUpdateId + 1);

    ZDP_MgmtNwkUpdateReq( &dstAddr, channelMask, 0xfe, 0, _NIB.nwkUpdateId, 0 );

    return ZSuccess;
}

/*********************************************************************
 * @fn      zllInitiator_ClassicalCommissioningInit
 *
 * @brief   Start Classical Commissioning.
 *
 * @param   none
 *
 * @return  status
 */
ZStatus_t zllInitiator_ClassicalCommissioningStart()
{
  ZStatus_t status;
  status = zll_ClassicalCommissioningInit();
  if ( ( POLL_RATE == 0 ) && ( status == ZSuccess ) )
  {
    //Allow completion of authentication process in the new network
    NLME_SetPollRate( ZLL_INITIATOR_TEMP_POST_TL_POLL_RATE );
    //Polling will be reset after authentication by ZDApp_DeviceAuthEvt(), but just in case
    osal_start_timerEx( zllInitiator_TaskID, ZLL_DISABLE_RX_EVT, 2*ZLL_APLC_RX_WINDOW_DURATION );
  }
  return status;
}

/*********************************************************************
 * @fn      zllSampleRemote_SendEPInfo
 *
 * @brief   Send Endpoint info command.
 *
 * @param   srcEP - source endpoint
 * @param   dstAddr - destination address
 * @param   seqNum - transaction sequnece number
 *
 * @return  ZStatus_t
 */
ZStatus_t zllInitiator_SendEPInfo( uint8 srcEP, afAddrType_t *dstAddr, uint8 seqNum)
{
    zclLLEndpointInfo_t zclLLEndpointInfoCmd;
    zclLLDeviceInfo_t  zclLLDeviceInfo;
      //send Epinfo cmd
    zll_GetSubDeviceInfo( 0, &zclLLDeviceInfo );
    zclLLEndpointInfoCmd.endpoint = zclLLDeviceInfo.endpoint;
    zclLLEndpointInfoCmd.profileID = zclLLDeviceInfo.profileID;
    zclLLEndpointInfoCmd.deviceID = zclLLDeviceInfo.deviceID;
    zclLLEndpointInfoCmd.version = zclLLDeviceInfo.version;

    osal_cpyExtAddr( zclLLEndpointInfoCmd.ieeeAddr, NLME_GetExtAddr() );
    zclLLEndpointInfoCmd.nwkAddr = NLME_GetShortAddr();

    dstAddr->panId = _NIB.nwkPanId;
    return zclLL_Send_EndpointInfo( srcEP, dstAddr, &zclLLEndpointInfoCmd,
                                          0, seqNum );
}

/*********************************************************************
 * @fn      zllInitiator_OrphanedRejoin
 *
 * @brief   Try to rejoin the network if the device is orphaned.
 *
 * @param   none
 *
 * @return  status - success if rejoin is initiated
 */
ZStatus_t zllInitiator_OrphanedRejoin()
{
  if ( ( _NIB.nwkState == NWK_DISC ) && ( _NIB.nwkCoordAddress != INVALID_NODE_ADDR ) && !zll_IsFactoryNew() )
  {
    initiatorReJoinNwk( MODE_REJOIN );
    return ZSuccess;
  }
  return ZFailure;
}

/*********************************************************************
 * @fn      zllInitiator_ResetToFNSelectedTarget
 *
 * @brief   Send Reset to Factory New Request command to the selected
 *          target of the current Touch-Link transaction.
 *          Note - this function should be called within no later than
 *          ZLL_APLC_INTER_PAN_TRANS_ID_LIFETIME ms from the Scan Request.
 *
 * @param   none
 *
 * @return  status - failure is returned due to invalid selected target or
 *          expired Touch-Link transaction.
 */
ZStatus_t zllInitiator_ResetToFNSelectedTarget()
{
  if ( ( zllTransID == 0 ) || ( selectedTarget.lastRssi == ZLL_TL_WORST_RSSI ) )
  {
    return ( ZFailure );
  }
  zclLLResetToFNReq_t req;
  req.transID = zllTransID;

  // Cancel further touch-link commissioning (if called during identify interval)
  osal_stop_timerEx( zllInitiator_TaskID, ZLL_CFG_TARGET_EVT );

  zll_SetChannel( selectedTarget.rxChannel );
  return zclLL_Send_ResetToFNReq( ZLL_INTERNAL_ENDPOINT, &(selectedTarget.srcAddr), &req, initiatorSeqNum++ );
}

#if defined ( MT_APP_FUNC )
/*********************************************************************
 * @fn      initiatorProcessRpcMsg
 *
 * @brief   Process MT_APP ZLL RPC message
 *
 * @param   srcEP - Sending Apps endpoint
 * @param   len - length of the received message
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
static ZStatus_t initiatorProcessRpcMsg( uint8 srcEP, uint8 len, uint8 *msg )
{
  uint16 clusterID;
  uint8 zllRpcCmdID;
  clusterID = BUILD_UINT16( msg[ZLL_MT_APP_RPC_OFFSET_CLUSTER_ID],
                            msg[ZLL_MT_APP_RPC_OFFSET_CLUSTER_ID + 1] );
  zllRpcCmdID = msg[ZLL_MT_APP_RPC_OFFSET_CMD_ID];

  if ( clusterID == ZLL_MT_APP_RPC_CLUSTER_ID )
  {
    switch ( zllRpcCmdID )
    {
      case ZLL_MT_APP_RPC_CMD_TOUCHLINK:
        zllInitiator_StartDevDisc();
        break;
      case ZLL_MT_APP_RPC_CMD_RESET_TO_FN:
        zllInitiator_ResetToFactoryNew();
        break;
      case ZLL_MT_APP_RPC_CMD_CH_CHANNEL:
        zllInitiator_ChannelChange( ( len > ( ZLL_MT_APP_RPC_OFFSET_CMD_ID - 1 ) ) ?
                                     msg[ZLL_MT_APP_RPC_OFFSET_CMD_ID + 1] : 0 );
        break;
      case ZLL_MT_APP_RPC_CMD_JOIN_HA:
        zllInitiator_ClassicalCommissioningStart();
        break;
      case ZLL_MT_APP_RPC_CMD_SEND_RESET_TO_FN:
        zllInitiator_ResetToFNSelectedTarget();
        break;
#if (ZSTACK_ROUTER_BUILD)
      case ZLL_MT_APP_RPC_CMD_PERMIT_JOIN:
        zllInitiator_PermitJoin( ( len > ( ZLL_MT_APP_RPC_OFFSET_CMD_ID - 1 ) ) ?
                               msg[ZLL_MT_APP_RPC_OFFSET_CMD_ID + 1] : 0 );
        break;
      case ZLL_MT_APP_RPC_CMD_START_DISTRIB_NWK:
        zllInitiator_BridgeStartNetwork();
        break;

#endif
    }
    return ( ZSuccess );
  }
  else
  {
    return ( ZFailure );
  }
}
#endif //defined ( MT_APP_FUNC )

#if (ZSTACK_ROUTER_BUILD)
/*********************************************************************
 * @fn      zllInitiator_PermitJoin
 *
 * @brief   Set the router permit join flag, to allow or deny classical
 *          commissioning by other ZigBee devices.
 *
 * @param   duration - enable up to aplcMaxPermitJoinDuration seconds,
 *                     0 to disable
 *
 * @return  status
 */
ZStatus_t zllInitiator_PermitJoin( uint8 duration )
{
  return zll_PermitJoin( duration );
}

/*********************************************************************
 * @fn      zllInitiator_BridgeStartNetwork
 *
 * @brief   Start a new ZLL network, without coordinator or Trust Center.
 *
 * @param   none
 *
 * @return  status
 */
ZStatus_t zllInitiator_BridgeStartNetwork( void )
{
  if ( !zll_IsFactoryNew() )
  {
    return ( ZFailure );
  }

    // Assuming initiatorSelectNwkParams() was called,
    // Copy the new network parameters to NV
  zll_SetNIB( NWK_ROUTER, _NIB.nwkDevAddress, _NIB.extendedPANID,
              _NIB.nwkLogicalChannel, _NIB.nwkPanId, _NIB.nwkUpdateId );

#ifdef ZLL_DEV_FIXED_NWK_KEY
  uint8 nwkKey[SEC_KEY_LEN] = ZLL_DEV_FIXED_NWK_KEY;
#else
  uint8 nwkKey[SEC_KEY_LEN];
  zll_GenerateRandNum( nwkKey, SEC_KEY_LEN );
#endif
  zll_UpdateNwkKey(nwkKey,0);

  // setting apsTrustCenterAddress to 0xffffffff
  ZDSecMgrUpdateTCAddress( NULL );

  // Use the new free ranges
  //zll_UpdateFreeRanges( pParams );

  // Save free ranges
  zll_UpdateNV( ZLL_UPDATE_NV_RANGES | ZLL_UPDATE_NV_NIB );

  // In case we're here after a leave
  zllLeaveInitiated = FALSE;

  // Clear leave control logic
  ZDApp_LeaveCtrlReset();

  // Start operating on the network, based on NV values
  ZDOInitDevice( 0 );

  return ( ZSuccess );
}
#endif //(ZSTACK_ROUTER_BUILD)

/*********************************************************************
*********************************************************************/

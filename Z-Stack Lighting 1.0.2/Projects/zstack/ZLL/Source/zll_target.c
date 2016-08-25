/**************************************************************************************************
  Filename:       zll_target.c
  Revised:        $Date: 2013-11-26 15:12:49 -0800 (Tue, 26 Nov 2013) $
  Revision:       $Revision: 36298 $

  Description:    Zigbee Cluster Library - Light Link Target.


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
#include "OSAL_Nv.h"
#include "AF.h"
#include "ZDApp.h"
#include "nwk_util.h"
#include "AddrMgr.h"
#include "ZDSecMgr.h"

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

#include "zll_target.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define ZLL_NWK_START_EVT                                  ZLL_AUX_1_EVT
#define ZLL_ROUTER_JOIN_EVT                                ZLL_NWK_JOIN_EVT

/*********************************************************************
 * TYPEDEFS
 */


typedef union
{
  zclLLNwkStartReq_t nwkStartReq;
  zclLLNwkJoinReq_t nwkJoinReq;
} zclLLReq_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8 zllTarget_TaskID;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */


// Info related to the received request
static zclLLReq_t rxReq; // network start or join request
static uint8 rxSeqNum;
static afAddrType_t dstAddr;

// Application callback
static zclGCB_Identify_t pfnIdentifyCB = NULL;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

// This callback is called to process a Scan Request command
static ZStatus_t targetScanReqCB( afAddrType_t *srcAddr, zclLLScanReq_t *pReq, uint8 seqNum );

// This callback is called to process a Device Information Request command
static ZStatus_t targetDeviceInfoReqCB( afAddrType_t *srcAddr, zclLLDeviceInfoReq_t *pReq, uint8 seqNum );

// This callback is called to process an Identify Request command
static ZStatus_t targetIdentifyReqCB( afAddrType_t *srcAddr, zclLLIdentifyReq_t *pReq );

// This callback is called to process a Reset to Factory New Request command
static ZStatus_t targetResetToFNReqCB( afAddrType_t *srcAddr, zclLLResetToFNReq_t *pReq );

// This callback is called to process a Network Start Request command
static ZStatus_t targetNwkStartReqCB( afAddrType_t *srcAddr, zclLLNwkStartReq_t *pReq, uint8 seqNum );

// This callback is called to process a Network Join Router Request command
static ZStatus_t targetNwkJoinRtrReqCB( afAddrType_t *srcAddr, zclLLNwkJoinReq_t *pReq, uint8 seqNum );

// This callback is called to process a Network Update Request command
static ZStatus_t targetNwkUpdateReqCB( afAddrType_t *srcAddr, zclLLNwkUpdateReq_t *pReq );

// This callback is called to process a Leave Confirmation message
static void *targetZdoLeaveCnfCB( void *pParam );

static void targetProcessStateChange( devStates_t devState );

static void targetStartRtr( zclLLNwkParams_t *pParams, uint32 transID );

static void targetSendNwkStartRsp( afAddrType_t *dstAddr, uint32 transID, uint8 status,
                                   zclLLNwkParams_t *pNwkParams, uint8 nwkUpdateId, uint8 seqNum );
static void targetSelectNwkParams( void );
static ZStatus_t targetVerifyNwkParams( uint16 PANID, uint8 *pExtendedPANID );

#if defined (MT_APP_FUNC)
static void targetProcessRpcMsg( uint8 srcEP, uint8 len, uint8 *msg );
#endif

/*********************************************************************
 * ZLL Target Callback Table
 */
// Target Command Callbacks table
static zclLL_InterPANCallbacks_t zllTarget_CmdCBs =
{
  // Received Server Commands
  targetScanReqCB,       // Scan Request command
  targetDeviceInfoReqCB, // Device Information Request command
  targetIdentifyReqCB,   // Identify Request command
  targetResetToFNReqCB,  // Reset to Factory New Request command
  targetNwkStartReqCB,   // Network Start Request command
  targetNwkJoinRtrReqCB, // Network Join Router Request command
  NULL,                  // Network Join End Device Request command
  targetNwkUpdateReqCB,  // Network Update Request command

  // Received Client Commands
  NULL,                  // Scan Response command
  NULL,                  // Device Information Response command
  NULL,                  // Network Start Response command
  NULL,                  // Network Join Router Response command
  NULL                   // Network Join End Device Response command
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      zllTarget_InitDevice
 *
 * @brief   Start the ZLL Target device in the network if it's not
 *          factory new. Otherwise, determine the network parameters
 *          and wait for a touchlink command.
 *
 * @param   none
 *
 * @return  status
 */
ZStatus_t zllTarget_InitDevice( void )
{
  if ( !zll_IsFactoryNew() )
  {
    // Resume ZigBee functionality based on the info stored in NV
    ZDOInitDevice( 0 );
  }
  else
  {
    uint8 x = TRUE;

    // Enable our receiver
    ZMacSetReq( ZMacRxOnIdle, &x );

    // Determine unique PAN Id and Extended PAN Id
    targetSelectNwkParams();

#ifndef HOLD_AUTO_START
    zll_ClassicalCommissioningInit();
#endif

    // Wait for a touchlink command
  }

  zllTarget_PermitJoin(0);

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      targetProcessStateChange
 *
 * @brief   Process ZDO device state change
 *
 * @param   devState - The device's network state
 *
 * @return  none
 */
static void targetProcessStateChange( devStates_t devState )
{
  if ( ( devState == DEV_ROUTER ) || ( devState == DEV_END_DEVICE ) )
  {
    ZDP_DeviceAnnce( NLME_GetShortAddr(), NLME_GetExtAddr(),
                     ZDO_Config_Node_Descriptor.CapabilityFlags, 0 );
  }
}

/*********************************************************************
 * @fn      zllTarget_RegisterIdentifyCB
 *
 * @brief   Register an Application's Idnetify callback function.
 *
 * @param   pfnIdentify - application callback
 *
 * @return  none
 */
void zllTarget_RegisterIdentifyCB( zclGCB_Identify_t pfnIdentify )
{
  pfnIdentifyCB = pfnIdentify;
}

/*********************************************************************
 * @fn      zllTarget_Init
 *
 * @brief   Initialization function for the ZLL Target task.
 *
 * @param   task_id - ZLL Target task id
 *
 * @return  none
 */
void zllTarget_Init( uint8 task_id )
{
  // Save our own Task ID
  zllTarget_TaskID = task_id;

  zll_SetZllTaskId( zllTarget_TaskID );

  // Initialize ZLL common variables
  zll_InitVariables( FALSE );

  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( zllTarget_TaskID );

  // Register for ZLL Target callbacks (for Inter-PAN commands)
  zclLL_RegisterInterPANCmdCallbacks( &zllTarget_CmdCBs );

  // Register for Initiator to receive Leave Confirm
  ZDO_RegisterForZdoCB( ZDO_LEAVE_CNF_CBID, targetZdoLeaveCnfCB );

  // Register to process ZDO messages
  ZDO_RegisterForZDOMsg( zllTarget_TaskID, Mgmt_Permit_Join_req );
  ZDO_RegisterForZDOMsg( zllTarget_TaskID, Device_annce );
}

/*********************************************************************
 * @fn      zllTarget_RegisterApp
 *
 * @brief   Register an Application's EndPoint with the ZLL profile.
 *
 * @param   simpleDesc -  application simple description
 * @param   pDeviceInfo - ZLL application device info
 *
 * @return  ZSuccess - Registered
 *          ZInvalidParameter - invalid or duplicate endpoint
 *          ZMemError - not enough memory to add
 */
ZStatus_t zllTarget_RegisterApp( SimpleDescriptionFormat_t *simpleDesc, zclLLDeviceInfo_t *pDeviceInfo )
{
  return zll_RegisterApp( simpleDesc, pDeviceInfo );
}

/*********************************************************************
 * @fn      zllTarget_PermitJoin
 *
 * @brief   Set the router permit join flag, to allow or deny classical
 *          commissioning by other ZigBee devices.
 *
 * @param   duration - enable up to aplcMaxPermitJoinDuration seconds,
 *                     0 to disable
 *
 * @return  status
 */
ZStatus_t zllTarget_PermitJoin( uint8 duration )
{
  return zll_PermitJoin( duration );
}

/*********************************************************************
 * @fn      zllTarget_event_loop
 *
 * @brief   Event Loop Processor for ZLL Target.
 *
 * @param   task_id - task id
 * @param   events - event bitmap
 *
 * @return  unprocessed events
 */
uint16 zllTarget_event_loop( uint8 task_id, uint16 events )
{
  if ( events & SYS_EVENT_MSG )
  {
    osal_event_hdr_t *pMsg;

    if ( (pMsg = (osal_event_hdr_t *)osal_msg_receive( task_id )) != NULL )
    {
      switch (pMsg->event )
      {
        case ZDO_CB_MSG:
          // ZDO sends the message that we registered for
          zll_RouterProcessZDOMsg( (zdoIncomingMsg_t *)pMsg );
          break;

        case ZDO_STATE_CHANGE:
          targetProcessStateChange( (devStates_t)pMsg->status );
          break;

#if defined (MT_APP_FUNC)
        case MT_SYS_APP_MSG:
          // Message received from MT
         targetProcessRpcMsg( ((mtSysAppMsg_t *)pMsg)->endpoint,
                              ((mtSysAppMsg_t *)pMsg)->appDataLen,
                              ((mtSysAppMsg_t *)pMsg)->appData );
          break;
#endif

        default:
          break;
      }

      // Release the OSAL message
      VOID osal_msg_deallocate( (uint8 *)pMsg );
    }

    // return unprocessed events
    return ( events ^ SYS_EVENT_MSG );
  }

  if ( events & ZLL_NWK_START_EVT )
  {
    zclLLNwkStartReq_t *pReq = &(rxReq.nwkStartReq);

    // If the PAN Id, Extended PAN Id or Logical Channel are zero then
    // determine each of these parameters
    if ( !nwk_ExtPANIDValid( pReq->nwkParams.extendedPANID ) )
    {
      zll_GenerateRandNum( pReq->nwkParams.extendedPANID, Z_EXTADDR_LEN );
    }

    if ( pReq->nwkParams.panId == 0 )
    {
      pReq->nwkParams.panId = osal_rand();
    }

    if ( pReq->nwkParams.logicalChannel == 0 )
    {
      pReq->nwkParams.logicalChannel = zll_GetRandPrimaryChannel();
    }

    if ( pReq->nwkParams.nwkAddr == 0 )
    {
      pReq->nwkParams.nwkAddr = osal_rand();
    }

    // Perform Network Discovery to verify our new network parameters uniqeness
    zll_PerformNetworkDisc( (uint32)1 << _NIB.nwkLogicalChannel );

    // return unprocessed events
    return ( events ^ ZLL_NWK_START_EVT );
  }

  if ( events & ZLL_NWK_DISC_CNF_EVT )
  {
    if ( ( zllHAScanInitiated ) && (zllHAScanInitiated != ZLL_SCAN_FOUND_NOTHING) )
    {
      if ( zll_ClassicalCommissioningJoinDiscoveredNwk() != ZSuccess )
      {
        // Select our Network Parameters
        targetSelectNwkParams();

        // Wait for a touchlink command
      }
    }
    else if ( zllHAScanInitiated != ZLL_SCAN_FOUND_NOTHING )
    {
      zclLLNwkStartReq_t *pReq = &(rxReq.nwkStartReq);
      zclLLNwkParams_t *pParams = &(pReq->nwkParams);
      uint8 status;

      // Verify the received Network Parameters
      if ( targetVerifyNwkParams( pParams->panId, pParams->extendedPANID ) == ZSuccess )
      {
        status = ZLL_NETWORK_START_RSP_STATUS_SUCCESS;
      }
      else
      {
        status = ZLL_NETWORK_START_RSP_STATUS_FAILURE;
      }

      zll_FreeNwkParamList();

      // Send a response back
      targetSendNwkStartRsp( &dstAddr, pReq->transID, status, pParams, _NIB.nwkUpdateId, rxSeqNum );
      if ( status == ZLL_NETWORK_START_RSP_STATUS_SUCCESS )
      {
        // If not factory new, perform a Leave on our old network
        if ( ( zll_IsFactoryNew() == FALSE ) && ( zll_SendLeaveReq() == ZSuccess ) )
        {
          // Wait for Leave confirmation before joining the new network
          zllLeaveInitiated = ZLL_LEAVE_TO_START_NWK;
        }
        else
        {
          // Notify our task to start the network
          osal_set_event( zllTarget_TaskID, ZLL_START_NWK_EVT );
        }
      }
      else
      {
        //NOTE - chosen network params were found in scan.
      }
    }
    else //zllHAScanInitiated == ZLL_SCAN_FOUND_NOTHING
    {
      zllHAScanInitiated = FALSE;
#ifndef HOLD_AUTO_START
      _NIB.nwkState = NWK_ROUTER;
#endif
    }

    // return unprocessed events
    return ( events ^ ZLL_NWK_DISC_CNF_EVT );
  }

  if ( events & ZLL_ROUTER_JOIN_EVT )
  {

    // If not factory new, perform a Leave on our old network
    if ( ( zll_IsFactoryNew() == FALSE ) && ( zll_SendLeaveReq() == ZSuccess ) )
    {
      // Wait for Leave confirmation before joining the new network
      zllLeaveInitiated = ZLL_LEAVE_TO_JOIN_NWK;
    }
    else
    {
      // Notify our task to join this network
      osal_set_event( zllTarget_TaskID, ZLL_JOIN_NWK_EVT );
    }

    // return unprocessed events
    return ( events ^ ZLL_ROUTER_JOIN_EVT );
  }

  if ( events & ZLL_START_NWK_EVT )
  {
    zclLLNwkStartReq_t *pReq = &(rxReq.nwkStartReq);

    // Start operating on the new network
    targetStartRtr( &(pReq->nwkParams), pReq->transID );

    // Perform a ZigBee Direct Join in order to allow direct communication
    // via the ZigBee network between the Initiator and the Target (i.e.,
    // create an entry in the neighbor table with the IEEE address and the
    // network address of the Initiator).
    NLME_DirectJoinRequestWithAddr( pReq->initiatorIeeeAddr, pReq->initiatorNwkAddr,
                                    CAPINFO_DEVICETYPE_RFD );

    // return unprocessed events
    return ( events ^ ZLL_START_NWK_EVT );
  }

  if ( events & ZLL_JOIN_NWK_EVT )
  {
    zclLLNwkJoinReq_t *pReq = &(rxReq.nwkJoinReq);

    // Start operating on the new network
    targetStartRtr( &(pReq->nwkParams), pReq->transID );

    // return unprocessed events
    return ( events ^ ZLL_JOIN_NWK_EVT );
  }

  if ( events & ZLL_RESET_TO_FN_EVT )
  {
    zllTarget_ResetToFactoryNew();

    // return unprocessed events
    return ( events ^ ZLL_RESET_TO_FN_EVT );
  }

  if ( events & ZLL_TRANS_LIFETIME_EXPIRED_EVT )
  {
    zllTransID = 0;
    // return unprocessed events
    return ( events ^ ZLL_TRANS_LIFETIME_EXPIRED_EVT );
  }

  // If reach here, the events are unknown
  // Discard or make more handlers
  return 0;
}

/*********************************************************************
 * @fn      targetStartRtr
 *
 * @brief   Start operating on the new network.
 *
 * @param   pParams - pointer to received network parameters
 * @param   transID - transaction id
 *
 * @return  none
 */
static void targetStartRtr( zclLLNwkParams_t *pParams, uint32 transID )
{
  // Copy the new network parameters to
  zll_SetNIB( NWK_ROUTER, pParams->nwkAddr, pParams->extendedPANID,
              pParams->logicalChannel, pParams->panId, _NIB.nwkUpdateId );

  // Apply the received network key
  zll_DecryptNwkKey( pParams->nwkKey, pParams->keyIndex, transID, zllResponseID );

  // setting apsTrustCenterAddress to 0xffffffff
  ZDSecMgrUpdateTCAddress( NULL );

  zllTarget_PermitJoin(0);

  // Use the new free ranges
  zll_UpdateFreeRanges( pParams );

  // Save free ranges
  zll_UpdateNV( ZLL_UPDATE_NV_RANGES );

  // In case we're here after a leave
  zllLeaveInitiated = FALSE;

  // Clear leave control logic
  ZDApp_LeaveCtrlReset();

  // Start operating on the new network
  ZDOInitDevice( 0 );
}


/*********************************************************************
 * @fn      targetSelectNwkParams
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
static void targetSelectNwkParams( void )
{
  uint8 status = ZFailure;

  while ( status == ZFailure )
  {
    // Select a random Extended PAN ID
    zll_GenerateRandNum( _NIB.extendedPANID, Z_EXTADDR_LEN );

    // Select a random PAN ID
    _NIB.nwkPanId = osal_rand();

    // Make sure they're unique
    status = targetVerifyNwkParams( _NIB.nwkPanId, _NIB.extendedPANID );
  }

  // Select randomly one of the ZLL channels as our logical channel
  _NIB.nwkLogicalChannel = zll_GetRandPrimaryChannel();

  _NIB.nwkDevAddress = osal_rand();

  // Configure MAC with our network parameters
  zll_SetMacNwkParams( _NIB.nwkDevAddress, _NIB.nwkPanId, _NIB.nwkLogicalChannel );
}

/*********************************************************************
 * @fn      targetVerifyNwkParams
 *
 * @brief   Verify that the PAN ID and Extended PAN ID are unique.
 *
 * @param   PANID - PAN Identifier
 * @param   pExtendedPANID - extended PAN Identifier
 *
 * @return  status
 */
static ZStatus_t targetVerifyNwkParams( uint16 PANID, uint8 *pExtendedPANID )
{
  zllDiscoveredNwkParam_t *pParam = pDiscoveredNwkParamList;

  // Add for our network parameters in the Network Parameter List
  while ( pParam != NULL )
  {
    if ( ( pParam->PANID == PANID ) &&
         ( osal_ExtAddrEqual( pParam->extendedPANID, pExtendedPANID ) ) )
    {
      return ( ZFailure );
    }

    pParam = pParam->nextParam;
  }

  return ( ZSuccess );
}


/*********************************************************************
 * @fn      targetScanReqCB
 *
 * @brief   This callback is called to process a Scan Request command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 * @param   seqNum - command sequence number
 *
 * @return  ZStatus_t
 */
static ZStatus_t targetScanReqCB( afAddrType_t *srcAddr, zclLLScanReq_t *pReq, uint8 seqNum )
{
  ZStatus_t ret = ZSuccess;
  int8 rssi;
  rssi = zll_GetMsgRssi();
  if( rssi > ZLL_TL_WORST_RSSI )
  {
    if ( pDiscoveredNwkParamList == NULL )
    {
      dstAddr = *srcAddr;
      dstAddr.panId = 0xFFFF;

      ret = zll_SendScanRsp( ZLL_INTERNAL_ENDPOINT, &dstAddr, pReq->transID, seqNum );
      if ( ret == ZSuccess )
      {
        zllTransID = pReq->transID;
      }
    }
  }

  return ( ret );
}

/*********************************************************************
 * @fn      targetDeviceInfoReqCB
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
static ZStatus_t targetDeviceInfoReqCB( afAddrType_t *srcAddr, zclLLDeviceInfoReq_t *pReq, uint8 seqNum )
{
  if ( zll_IsValidTransID( pReq->transID ) == FALSE )
  {
    return ( ZFailure );
  }
  return ( zll_SendDeviceInfoRsp( ZLL_INTERNAL_ENDPOINT, srcAddr,
                                  pReq->startIndex, pReq->transID, seqNum ) );
}

/*********************************************************************
 * @fn      targetIdentifyReqCB
 *
 * @brief   This callback is called to process an Identify Request command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t targetIdentifyReqCB( afAddrType_t *srcAddr, zclLLIdentifyReq_t *pReq )
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
 * @fn      targetResetToFNReqCB
 *
 * @brief   This callback is called to process a Reset to Factory New
 *          Request command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t targetResetToFNReqCB( afAddrType_t *srcAddr, zclLLResetToFNReq_t *pReq )
{
  // If factory new, discard the request
  if ( ( zll_IsValidTransID( pReq->transID ) == FALSE ) || ( zll_IsFactoryNew() ) )
  {
    return ( ZFailure );
  }

  osal_set_event( zllTarget_TaskID, ZLL_RESET_TO_FN_EVT );

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      targetNwkStartReqCB
 *
 * @brief   This callback is called to process a Network Start Request command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 * @param   seqNum - command sequence number
 *
 * @return  ZStatus_t
 */
static ZStatus_t targetNwkStartReqCB( afAddrType_t *srcAddr, zclLLNwkStartReq_t *pReq, uint8 seqNum )
{
  if ( zll_IsValidTransID( pReq->transID ) == FALSE )
  {
    return ( ZFailure );
  }
  dstAddr = *srcAddr;
  dstAddr.panId = 0xFFFF;

  if ( pDiscoveredNwkParamList == NULL )
  {
    // Save the request for later
    rxReq.nwkStartReq = *pReq;
    rxSeqNum = seqNum;

    osal_set_event( zllTarget_TaskID, ZLL_NWK_START_EVT );
  }
  else
  {
    targetSendNwkStartRsp( &dstAddr, pReq->transID, ZLL_NETWORK_START_RSP_STATUS_FAILURE,
                           NULL, 0, seqNum );
  }

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      targetNwkJoinRtrReqCB
 *
 * @brief   This callback is called to process a Network Join Router
 *          Request command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 * @param   seqNum - command sequence number
 *
 * @return  ZStatus_t
 */
static ZStatus_t targetNwkJoinRtrReqCB( afAddrType_t *srcAddr, zclLLNwkJoinReq_t *pReq, uint8 seqNum )
{
  zclLLNwkJoinRsp_t rsp;
  if ( zll_IsValidTransID( pReq->transID ) == FALSE )
  {
    return ( ZFailure );
  }

  dstAddr = *srcAddr;
  dstAddr.panId = 0xFFFF;

  if ( pDiscoveredNwkParamList == NULL )
  {
    // Save the request for later
    rxReq.nwkJoinReq = *pReq;

    osal_set_event( zllTarget_TaskID, ZLL_ROUTER_JOIN_EVT );

    rsp.status = ZLL_NETWORK_JOIN_RSP_STATUS_SUCCESS;
  }
  else
  {
    rsp.status = ZLL_NETWORK_JOIN_RSP_STATUS_FAILURE;
  }

  rsp.transID = pReq->transID;

  // Send a response back
  zclLL_Send_NwkJoinRtrRsp( ZLL_INTERNAL_ENDPOINT, &dstAddr, &rsp, seqNum );

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      targetNwkUpdateReqCB
 *
 * @brief   This callback is called to process a Network Update Request
 *          command.
 *
 * @param   srcAddr - sender's address
 * @param   pReq - parsed command
 *
 * @return  ZStatus_t
 */
static ZStatus_t targetNwkUpdateReqCB( afAddrType_t *srcAddr, zclLLNwkUpdateReq_t *pReq )
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
      zll_ProcessNwkUpdate( newUpdateId, pReq->logicalChannel );
    }
  }

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      targetSendNwkStartRsp
 *
 * @brief   Send out a Network Start Response command.
 *
 * @param   dstAddr - destination's address
 * @param   transID - touch link transaction identifier
 * @param   status - Network Start Response command status field
 * @param   pNwkParams - network parameters
 * @param   nwkUpdateId - network update identifier
 * @param   seqNum
 *
 * @return  none
 */
static void targetSendNwkStartRsp( afAddrType_t *dstAddr, uint32 transID, uint8 status,
                                   zclLLNwkParams_t *pNwkParams, uint8 nwkUpdateId, uint8 seqNum )
{
  zclLLNwkStartRsp_t *pRsp;

  // Send out a response
  pRsp = (zclLLNwkStartRsp_t *)osal_mem_alloc( sizeof( zclLLNwkStartRsp_t ) );
  if ( pRsp )
  {
    pRsp->transID = transID;
    pRsp->status = status;

    if ( pNwkParams != NULL )
    {
      osal_cpyExtAddr( pRsp->extendedPANID, pNwkParams->extendedPANID );
      pRsp->logicalChannel = pNwkParams->logicalChannel;
      pRsp->panId = pNwkParams->panId;
    }
    else
    {
      osal_memset( pRsp->extendedPANID, 0, Z_EXTADDR_LEN );
      pRsp->logicalChannel = 0;
      pRsp->panId = 0;
    }

    pRsp->nwkUpdateId = nwkUpdateId;

    zclLL_Send_NwkStartRsp( ZLL_INTERNAL_ENDPOINT, dstAddr, pRsp, seqNum );

    osal_mem_free( pRsp );
  }
}

/******************************************************************************
 * @fn      targetZdoLeaveCnfCB
 *
 * @brief   This callback is called to process a Leave Confirmation message.
 *
 *          Note: this callback function returns a pointer if it has handled
 *                the confirmation message and no further action should be
 *                taken with it. It returns NULL if it has not handled the
 *                confirmation message and normal processing should take place.
 *
 * @param       pParam - received message
 *
 * @return      Pointer if message processed. NULL, otherwise.
 */
static void *targetZdoLeaveCnfCB( void *pParam )
{
  // Clear all groups and scenes for all endpoints.
  if ( ((NLME_LeaveCnf_t*)pParam)->rejoin == FALSE )
  {
    uint8 i,n_groups;
    uint16 groupList[APS_MAX_GROUPS];
    epList_t *epItem;

    for ( epItem = epList; epItem != NULL; epItem = epItem->nextDesc )
    {
      if ( n_groups = aps_FindAllGroupsForEndpoint( epItem->epDesc->endPoint, groupList ) )
      {
        for ( i = 0; i < n_groups; i++ )
        {
          zclGeneral_RemoveAllScenes( epItem->epDesc->endPoint, groupList[i] );
        }
        aps_RemoveAllGroup( epItem->epDesc->endPoint );
      }
    }
  }

  // Did we initiate the leave?
  if ( zllLeaveInitiated == FALSE )
  {
    return ( NULL );
  }

  if ( zllLeaveInitiated == ZLL_LEAVE_TO_START_NWK )
  {
    // Notify our task to start the network
    osal_set_event( zllTarget_TaskID, ZLL_START_NWK_EVT );
  }
  else // ZLL_LEAVE_TO_JOIN_NWK
  {
    AssocReset();
    nwkNeighborInitTable();
    AddrMgrSetDefaultNV();
    // Immediately store empty tables in NV
    osal_set_event( ZDAppTaskID, ZDO_NWK_UPDATE_NV );
    // Notify our task to join the new network
    osal_start_timerEx( zllTarget_TaskID, ZLL_JOIN_NWK_EVT, 100 );
  }

  return ( (void *)&zllLeaveInitiated );
}


/*********************************************************************
 * @fn      zllTarget_ResetToFactoryNew
 *
 * @brief   Call to Reset the Target to factory new
 *
 * @param   none
 *
 * @return  none
 */
void zllTarget_ResetToFactoryNew()
{
  zll_ResetToFactoryNew( FALSE );
}

/*********************************************************************
 * @fn      zllTarget_ClassicalCommissioningStart
 *
 * @brief   Start Classical Commissioning.
 *
 * @param   none
 *
 * @return  status
 */
ZStatus_t zllTarget_ClassicalCommissioningStart()
{
  return ( zll_ClassicalCommissioningInit() );
}

#if defined (MT_APP_FUNC)
/*********************************************************************
 * @fn      targetProcessRpcMsg
 *
 * @brief   Process Test messages - currently just test app messages
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
static void targetProcessRpcMsg( uint8 srcEP, uint8 len, uint8 *msg )
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
      case ZLL_MT_APP_RPC_CMD_RESET_TO_FN:
        zllTarget_ResetToFactoryNew();
        break;
      case ZLL_MT_APP_RPC_CMD_JOIN_HA:
        zllTarget_ClassicalCommissioningStart();
        break;
      case ZLL_MT_APP_RPC_CMD_PERMIT_JOIN:
        zllTarget_PermitJoin( ( len > ( ZLL_MT_APP_RPC_OFFSET_CMD_ID - 1 ) ) ?
                               msg[ZLL_MT_APP_RPC_OFFSET_CMD_ID + 1] : 0 );
        break;
    }
  }
}
#endif


/*********************************************************************
*********************************************************************/

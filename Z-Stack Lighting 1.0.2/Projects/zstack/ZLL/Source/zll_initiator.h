/**************************************************************************************************
  Filename:       zll_initiator.h
  Revised:        $Date: 2013-07-15 15:29:01 -0700 (Mon, 15 Jul 2013) $
  Revision:       $Revision: 34720 $

  Description:    This file contains the ZCL Light Link (ZLL) Initiator definitions.


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

#ifndef ZLL_INITIATOR_H
#define ZLL_INITIATOR_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "zcl_general.h"
#include "zcl_ll.h"

/*********************************************************************
 * CONSTANTS
 */
// time interval (in msec) between target selection and configuration,
// to allow target visual identification by the user.
#define ZLL_INITIATOR_IDENTIFY_INTERVAL          500

// If defined, the initiator will determine new ZLL network parameters.
// Otherwise, it will be determind by the target starting the network.
//#define ZLL_INITIATOR_SET_NEW_NWK_PARAMS

/*********************************************************************
 * TYPEDEFS
 */
// This callback is called to notify the application when a target device is
// successfully touch-linked.
typedef ZStatus_t (*zll_NotifyAppTLCB_t)( epInfoRec_t *pData );

// This callback is called to decide whether to select a device, which responded to scan, as a target.
// Note newScanRsp value should be copied if used beyond the call scope.
typedef uint8 (*zll_SelectDiscDevCB_t)( const zclLLScanRsp_t *newScanRsp, int8 newRssi );

/*********************************************************************
 * VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

/*-------------------------------------------------------------------
 * TASK API - These functions must only be called by OSAL.
 */

/*
 * Initialization for the ZLL Initiator task
 */
void zllInitiator_Init( uint8 task_id );

/*
 *  Event Process for the ZLL Initiator task
 */
uint16 zllInitiator_event_loop( uint8 task_id, uint16 events );

/*-------------------------------------------------------------------
-------------------------------------------------------------------*/

/*
 * Start the ZLL Initiator device in the network
 */
ZStatus_t zllInitiator_InitDevice( void );

/*
 * Register application task to receive unprocessed messages
 */
ZStatus_t zllInitiator_RegisterForMsg( uint8 taskId );

/*
 * Register an Application's Reset callback function
 */
void zllInitiator_RegisterResetAppCB( zclGCB_BasicReset_t pfnResetApp );

/*
 * Register an Application's Identify callback function
 */
void zllInitiator_RegisterIdentifyCB( zclGCB_Identify_t pfnIdentify );

/*
 * Register an Application's Touch-Link Notify callback function
 */
void zllInitiator_RegisterNotifyTLCB( zll_NotifyAppTLCB_t pfnNotifyApp );

/*
 * Register an Application's EndPoint with the ZLL profile
 */
ZStatus_t zllInitiator_RegisterApp( SimpleDescriptionFormat_t *simpleDesc, zclLLDeviceInfo_t *pDeviceInfo );

/*
 * Register an Application's Selection callback function
 */
void zllInitiator_RegisterSelectDiscDevCB( zll_SelectDiscDevCB_t pfnSelectDiscDev );

/*
 * Start Touch-Link device discovery
 */
ZStatus_t zllInitiator_StartDevDisc( void );

/*
 * Abort Touch-Link
 */
ZStatus_t zllInitiator_AbortTL( void );

/*
 * Change Channel for Frequency Agility
 */
ZStatus_t zllInitiator_ChannelChange( uint8 targetChannel );

/*
 * Reset device to Factory New
 */
void zllInitiator_ResetToFactoryNew( void );

/*
 * Start Classical Commissioning
 */
ZStatus_t zllInitiator_ClassicalCommissioningStart( void );

/*
 * Send EP Info
 */
ZStatus_t zllInitiator_SendEPInfo( uint8 srcEP, afAddrType_t *dstAddr, uint8 seqNum);

/*
 * Try to Rejoin network if the device is orphaned
 */
ZStatus_t zllInitiator_OrphanedRejoin( void );

/*
 * Send Reset to FN to the selected target of the current TL transaction
 */
ZStatus_t zllInitiator_ResetToFNSelectedTarget( void );

#if (ZSTACK_ROUTER_BUILD)
/*
 * Set the router permit join flag
 */
ZStatus_t zllInitiator_PermitJoin( uint8 duration );

/*
 * Start a new ZLL network
 */
ZStatus_t zllInitiator_BridgeStartNetwork( void );
#endif

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZLL_INITIATOR_H */

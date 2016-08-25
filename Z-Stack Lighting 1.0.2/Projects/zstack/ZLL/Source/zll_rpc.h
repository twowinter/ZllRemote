/**************************************************************************************************
  Filename:       zll_rpc.h
  Revised:        $Date: 2013-11-22 16:17:23 -0800 (Fri, 22 Nov 2013) $
  Revision:       $Revision: 36220 $

  Description:    This file contains the Zigbee Cluster Library: Light Link
                  MT Remote procedure call definitions.


  Copyright 2012 Texas Instruments Incorporated. All rights reserved.

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

#ifndef ZLL_RPC_H
#define ZLL_RPC_H

#ifdef __cplusplus
extern "C"
{
#endif

#if ( defined (MT_TASK) && !defined (MT_APP_FUNC) )
#warning MT_TASK and MT_APP_FUNC should be defined globally to use ZLL RPC
#endif

/*********************************************************************
 * INCLUDES
 */
#include "zcl_ll.h"

/*********************************************************************
 * CONSTANTS
 */

#define ZLL_MT_APP_RPC_CLUSTER_ID             0xFFFF

#define ZLL_MT_APP_RPC_CMD_TOUCHLINK          0x01
#define ZLL_MT_APP_RPC_CMD_RESET_TO_FN        0x02
#define ZLL_MT_APP_RPC_CMD_CH_CHANNEL         0x03
#define ZLL_MT_APP_RPC_CMD_JOIN_HA            0x04
#define ZLL_MT_APP_RPC_CMD_PERMIT_JOIN        0x05
#define ZLL_MT_APP_RPC_CMD_SEND_RESET_TO_FN   0x06
#define ZLL_MT_APP_RPC_CMD_START_DISTRIB_NWK  0x07

#define ZLL_MT_APP_RPC_OFFSET_CLUSTER_ID      3
#define ZLL_MT_APP_RPC_OFFSET_CMD_ID          9

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Process and send Indication for a successful ZLL Touch-Link over MT_APP
 */
void MT_ZllNotifyTL( epInfoRec_t *pRec );

/*
 * Process and send received ZCL commands over MT_APP.
 */
void MT_ZllSendZCLCmd( uint8 appEP, uint16 shortAddr, uint8 endpoint, uint16 clusterID,
                            zclFrameHdr_t *zclHdr, uint8 payloadLen, uint8 *pPayload );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZLL_RPC_H */
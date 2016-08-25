/**************************************************************************************************
  Filename:       zll_rpc.c
  Revised:        $Date: 2012-10-29 16:57:10 -0700 (Mon, 29 Oct 2012) $
  Revision:       $Revision: 31957 $

  Description:    This file contains the Zigbee Cluster Library: Light Link
                  MT Remote procedure calls.


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

/*********************************************************************
 * INCLUDES
 */
 #include "zll_rpc.h"
 #include "MT.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

 /***************************************************************************************************
 * @fn      MT_ZllNotifyTL
 *
 * @brief   Process and send Indication for a successful ZLL Touch-Link over MT_APP.
 *
 * @param   pRec - Target's information record
 *
 * @return  none
 ***************************************************************************************************/
void MT_ZllNotifyTL( epInfoRec_t *pRec )
{
  byte *pBuf;
  pBuf = osal_mem_alloc( ZLL_CMDLENOPTIONAL_GET_EP_LIST_RSP ); // endpoint information record entry
  if ( pBuf )
  {
    pBuf[0] = LO_UINT16( pRec->nwkAddr );
    pBuf[1] = HI_UINT16( pRec->nwkAddr );

    pBuf[2] = pRec->endpoint;

    pBuf[3] = LO_UINT16( pRec->profileID );
    pBuf[4] = HI_UINT16( pRec->profileID );

    pBuf[5] = LO_UINT16( pRec->deviceID );
    pBuf[6] = HI_UINT16( pRec->deviceID );

    pBuf[7] = pRec->version;

    /* Send out Reset Response message */
    MT_BuildAndSendZToolResponse( ((uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_APP),
                                   MT_APP_ZLL_TL_IND,
                                   ZLL_CMDLENOPTIONAL_GET_EP_LIST_RSP, pBuf );

    osal_mem_free( pBuf );
  }
}

/*********************************************************************
 * @fn      MT_ZllSendZCLCmd
 *
 * @brief   Process and send received ZCL commands over MT_APP.
 *          Will be sent in Z-Tool format as APP_MSG_RESPONSE
 *
 * @param   appEP - The user application endpoint (original message's destination endpoint)
 * @param   shortAddr - source network address
 * @param   endpoint - source endpoint
 * @param   clusterID
 * @param   zclHdr - pointer to the received ZCL header
 * @param   payloadLen - length of payload
 * @param   pPayload - pointer to the payload buffer
 *
 * @return  none
 */
void MT_ZllSendZCLCmd( uint8 appEP, uint16 shortAddr, uint8 endpoint, uint16 clusterID,
                            zclFrameHdr_t *pZclHdr, uint8 payloadLen, uint8 *pPayload )
{
  uint8 *msg;
  uint8 *pBuf;
  uint8 len;
  uint8 zclHdrLen;

  zclHdrLen = ( pZclHdr->fc.manuSpecific ? 5 : 3 );
  // prepend srcAddr, endpoint, cluster ID, and data length
  len =   sizeof ( uint8 )      // this endpoint
        + sizeof ( uint16 )     // srcAddr (low byte first)
        + sizeof ( uint8 )      // source endpoint
        + sizeof ( uint16 )     // cluster ID (low byte first)
        + sizeof ( uint8 )      // data length
        + zclHdrLen             // ZCL header
        + payloadLen;

  msg = osal_mem_alloc( len );
  if ( msg != NULL )
  {
    pBuf = msg;
    *pBuf++ = appEP;
    *pBuf++ = LO_UINT16( shortAddr );
    *pBuf++ = HI_UINT16( shortAddr );
    *pBuf++ = endpoint;
    *pBuf++ = LO_UINT16( clusterID );
    *pBuf++ = HI_UINT16( clusterID );
    *pBuf++ = zclHdrLen + payloadLen;

    // zclBuildHdr( pZclHdr, pBuf );
    *pBuf++ = ( pZclHdr->fc.type
                | ( pZclHdr->fc.manuSpecific << 2 )
                | ( pZclHdr->fc.direction << 3 )
                | ( pZclHdr->fc.disableDefaultRsp << 4 ) );
    if ( pZclHdr->fc.manuSpecific )
    {
      *pBuf++ = LO_UINT16( pZclHdr->manuCode );
      *pBuf++ = HI_UINT16( pZclHdr->manuCode );
    }
    *pBuf++ = pZclHdr->transSeqNum;
    *pBuf++ = pZclHdr->commandID;

    if ( payloadLen )
    {
      osal_memcpy( pBuf, pPayload, payloadLen );
    }

    MT_BuildAndSendZToolResponse( ((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_APP), MT_APP_RSP,
                                  len, msg );

    osal_mem_free( msg );
  }
}

/*********************************************************************
*********************************************************************/

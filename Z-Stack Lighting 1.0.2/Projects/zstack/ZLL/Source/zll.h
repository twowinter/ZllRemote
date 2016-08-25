/**************************************************************************************************
  Filename:       zll.h
  Revised:        $Date: 2013-08-30 16:09:11 -0700 (Fri, 30 Aug 2013) $
  Revision:       $Revision: 35197 $

  Description:    This file contains the Zigbee Cluster Library: Light Link
                  Profile definitions.


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

#ifndef ZLL_H
#define ZLL_H

#ifdef __cplusplus
extern "C"
{
#endif

#if ( SECURE != TRUE )
#error SECURE must be globally defined to TRUE.
#endif
/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// ZLL Profile Constants
#define ZLL_APLC_INTER_PAN_TRANS_ID_LIFETIME               8000 // 8s
#define ZLL_APLC_MIN_STARTUP_DELAY_TIME                    2000 // 2s
#define ZLL_APLC_RX_WINDOW_DURATION                        5000 // 5s
#define ZLL_APLC_SCAN_TIME_BASE_DURATION                   250  // 0.25s
#define ZLL_APLC_MAX_PERMIT_JOIN_DURATION                  60   // 60s

/** Received signal strength threshold **/

// Manufacturer specific threshold (greater than -128),
// do not respond to Touch-link scan request if reached
#ifndef ZLL_TL_WORST_RSSI
#define ZLL_TL_WORST_RSSI -40 // dBm
#endif

// Pre-programmed RSSI correction offset (0x00-0x20)
#ifndef ZLL_RSSI_CORRECTION
#define ZLL_RSSI_CORRECTION                                0x00
#endif

/** Pre-Installed Keys **/

//#define ZLL_MASTER_KEY                   { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??,\
                                             0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? }
//#define ZLL_MASTER_LINK_KEY              { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??,\
                                             0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? }

#define ZLL_CERTIFICATION_ENC_KEY          { 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,\
                                             0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf }
#define ZLL_CERTIFICATION_LINK_KEY         { 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,\
                                             0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf }

#define ZLL_DEFAULT_AES_KEY                { 0x50, 0x68, 0x4c, 0x69, 0xea, 0x9c, 0xd1, 0x38,\
                                             0x43, 0x4c, 0x53, 0x4e, 0x8f, 0x8d, 0xba, 0xb4 }
//#define ZLL_DEV_FIXED_NWK_KEY            { 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,\
                                             0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc }

#define ZLL_KEY_INDEX_DEV         0
#define ZLL_KEY_INDEX_MASTER      4
#define ZLL_KEY_INDEX_CERT        15

// For production:
//#define ZLL_ENC_KEY  ZLL_MASTER_KEY
//#define ZLL_LINK_KEY  ZLL_MASTER_LINK_KEY
//#define ZLL_KEY_INDEX ZLL_KEY_INDEX_MASTER

// For certification only:
#define ZLL_ENC_KEY  ZLL_CERTIFICATION_ENC_KEY
#define ZLL_LINK_KEY  ZLL_CERTIFICATION_LINK_KEY
#define ZLL_KEY_INDEX ZLL_KEY_INDEX_CERT

// Dev mode: set offset for primary channels.
//#define Ch_Plus_3

#ifdef Ch_Plus_1
#warning ZLL primary channel offset 1
// ZLL Channels + 1
#define ZLL_FIRST_CHANNEL                                  12 // 0x0C
#define ZLL_SECOND_CHANNEL                                 16 // 0x10
#define ZLL_THIRD_CHANNEL                                  21 // 0x15
#define ZLL_FOURTH_CHANNEL                                 26 // 0x19


#define ZLL_PRIMARY_CHANNEL_LIST                           ( 0x04000000 | \
                                                             0x00200000 | \
                                                             0x00010000 | \
                                                             0x00001000 )
#define ZLL_SECONDARY_CHANNEL_LIST                         0x03DEE800  // ZLL Secondary Channels +1
#define ZLL_SECONDARY_CHANNELS_SET                         {11, 13, 14, 15, 17, 18, 19, 20, 22, 23, 24, 25}

#elif (defined Ch_Plus_2)
#warning ZLL primary channel offset 2
// ZLL Channels + 2
#define ZLL_FIRST_CHANNEL                                  13 // 0x0D
#define ZLL_SECOND_CHANNEL                                 17 // 0x11
#define ZLL_THIRD_CHANNEL                                  22 // 0x16
#define ZLL_FOURTH_CHANNEL                                 19 // 0x13


#define ZLL_PRIMARY_CHANNEL_LIST                           ( 0x00400000 | \
                                                             0x00080000 | \
                                                             0x00020000 | \
                                                             0x00002000 )
#define ZLL_SECONDARY_CHANNEL_LIST                         0x0785E800  // ZLL Secondary Channels +2
#define ZLL_SECONDARY_CHANNELS_SET                         {11, 12, 14, 15, 16, 18, 20, 21, 23, 24, 25, 26}

#elif (defined Ch_Plus_3)
#warning ZLL primary channel offset 3
// ZLL Channels + 3
#define ZLL_FIRST_CHANNEL                                  14 // 0x0E
#define ZLL_SECOND_CHANNEL                                 18 // 0x12
#define ZLL_THIRD_CHANNEL                                  23 // 0x17
#define ZLL_FOURTH_CHANNEL                                 24 // 0x18


#define ZLL_PRIMARY_CHANNEL_LIST                           ( 0x01000000 | \
                                                             0x00800000 | \
                                                             0x00040000 | \
                                                             0x00004000 )
#define ZLL_SECONDARY_CHANNEL_LIST                         0x067BB800  // ZLL Secondary Channels +3
#define ZLL_SECONDARY_CHANNELS_SET                         {11, 12, 13, 15, 16, 17, 19, 20, 21, 22, 25, 26}

#else
// ZLL Channels (standard)
#define ZLL_FIRST_CHANNEL                                  11 // 0x0B
#define ZLL_SECOND_CHANNEL                                 15 // 0x0F
#define ZLL_THIRD_CHANNEL                                  20 // 0x14
#define ZLL_FOURTH_CHANNEL                                 25 // 0x19

#define ZLL_PRIMARY_CHANNEL_LIST                           ( 0x02000000 | \
                                                             0x00100000 | \
                                                             0x00008000 | \
                                                             0x00000800 )
#define ZLL_SECONDARY_CHANNEL_LIST                         0x05EF7000  // ZLL Secondary Channels
#define ZLL_SECONDARY_CHANNELS_SET                         {12, 13, 14, 16, 17, 18, 19, 21, 22, 23, 24, 26}
#endif

// Dev mode: use only first primary channel
//#define ZLL_DEV_SELECT_FIRST_CHANNEL

// Task Events
#define ZLL_NWK_DISC_CNF_EVT                               0x0001
#define ZLL_NWK_JOIN_EVT                                   0x0002
#define ZLL_RESET_TO_FN_EVT                                0x0004
#define ZLL_START_NWK_EVT                                  0x0008
#define ZLL_JOIN_NWK_EVT                                   0x0010
#define ZLL_TRANS_LIFETIME_EXPIRED_EVT                     0x0020
#define ZLL_AUX_1_EVT                                      0x0040
#define ZLL_AUX_2_EVT                                      0x0080
#define ZLL_AUX_3_EVT                                      0x0100
#define ZLL_AUX_4_EVT                                      0x0200
#define ZLL_AUX_5_EVT                                      0x0400
#define ZLL_AUX_6_EVT                                      0x0800
#define ZLL_AUX_7_EVT                                      0x1000
#define ZLL_AUX_8_EVT                                      0x2000
#define ZLL_AUX_9_EVT                                      0x4000 //last

#define ZLL_DEFAULT_IDENTIFY_TIME                          3

// Reason for Leave command initiation
#define ZLL_LEAVE_TO_JOIN_NWK                              1
#define ZLL_LEAVE_TO_START_NWK                             2

//Scan modes
#define ZLL_SCAN_PRIMARY_CHANNELS                          1
#define ZLL_SCAN_SECONDARY_CHANNELS                        2
#define ZLL_SCAN_FOUND_NOTHING                             3

#define ZLL_ADDR_MIN                                       0x0001
#define ZLL_ADDR_MAX                                       0xFFF7
#define ZLL_ADDR_THRESHOLD                                 10

#define ZLL_GRP_ID_MIN                                     0x0001
#define ZLL_GRP_ID_MAX                                     0xFEFF
#define ZLL_GRP_ID_THRESHOLD                               10

#define ZLL_UPDATE_NV_NIB                                  0x01
#define ZLL_UPDATE_NV_RANGES                               0x02

// For internal EP's simple descriptor
#define ZLL_INTERNAL_ENDPOINT                              13
#define ZLL_INTERNAL_DEVICE_ID                             0xE15E
#define ZLL_INTERNAL_FLAGS                                 0

/*********************************************************************
 * MACROS
 */
#define ZLL_PRIMARY_CHANNEL( ch )              ( (ch) == ZLL_FIRST_CHANNEL  || \
                                                 (ch) == ZLL_SECOND_CHANNEL || \
                                                 (ch) == ZLL_THIRD_CHANNEL  || \
                                                 (ch) == ZLL_FOURTH_CHANNEL )

#define ZLL_SAME_NWK( panId, ePanId )          ( ( _NIB.nwkPanId == (panId) ) && \
                                                 osal_ExtAddrEqual( _NIB.extendedPANID, (ePanId) ) )

/*********************************************************************
 * TYPEDEFS
 */
typedef struct nwkParam
{
  struct nwkParam *nextParam;
  uint16 PANID;
  uint8 logicalChannel;
  uint8 extendedPANID[Z_EXTADDR_LEN];
  uint16 chosenRouter;
  uint8 chosenRouterLinkQuality;
  uint8 chosenRouterDepth;
  uint8 routerCapacity;
  uint8 deviceCapacity;
} zllDiscoveredNwkParam_t;

/*********************************************************************
 * VARIABLES
 */
extern uint32 zllResponseID;
extern uint32 zllTransID;

extern uint16 zllGrpIDsBegin;
extern uint16 zllGrpIDsEnd;

extern uint8 zllLeaveInitiated;
extern uint8 zllHAScanInitiated;

extern zllDiscoveredNwkParam_t *pDiscoveredNwkParamList;

/*********************************************************************
 * FUNCTIONS
 */

/*
 *  Register Simple Description to the AF
 */
ZStatus_t zll_RegisterSimpleDesc( SimpleDescriptionFormat_t *simpleDesc );

/*
 *  Initialize the ZLL global and local variables
 */
void zll_InitVariables( bool initiator );

/*
 * Register an Application with the ZLL
 */
ZStatus_t zll_RegisterApp( SimpleDescriptionFormat_t *simpleDesc, zclLLDeviceInfo_t *pDeviceInfo);

/*
 *  Check to see if the device is factory new
 */
bool zll_IsFactoryNew( void );

/*
 * Get the total number of sub-devices (endpoints) registered
 */
uint8 zll_GetNumSubDevices( uint8 startIndex );

/*
 * Get the total number of group IDs required by this device
 */
uint8 zll_GetNumGrpIDs( void );

/*
 * Get the sub-device information
 */
void zll_GetSubDeviceInfo( uint8 index, zclLLDeviceInfo_t *pInfo );

/*
 * Copy new Network Parameters to the NIB
 */
void zll_SetNIB( nwk_states_t nwkState, uint16 nwkAddr, uint8 *pExtendedPANID,
                        uint8 logicalChannel, uint16 panId, uint8 nwkUpdateId );

/*
 * Updates NV with NIB and free ranges items
 */
void zll_UpdateNV( uint8 enables );

/*
 * Set our channel
 */
void zll_SetChannel( uint8 newChannel );

/*
 * Encrypt the network key to be sent to the Target
 */
void zll_EncryptNwkKey( uint8 *pNwkKey, uint8 keyIndex, uint32 transID, uint32 responseID );

/*
 * Decrypt the network key received from the Initiator
 */
void zll_DecryptNwkKey( uint8 *pNwkKey, uint8 keyIndex, uint32 transID, uint32 responseID );

/*
 * Fill buffer with random bytes
 */
void zll_GenerateRandNum( uint8 *pNum, uint8 numSize );

/*
 * Get randomly chosen ZLL primary channel
 */
uint8 zll_GetRandPrimaryChannel( void );

/*
 * Get the supported network key bitmask
 */
uint16 zll_GetNwkKeyBitmask( void );

/*
 * Update our local network update id and logical channel
 */
void zll_ProcessNwkUpdate( uint8 nwkUpdateId, uint8 logicalChannel );

/*
 * Configure MAC with our Network Parameters
 */
void zll_SetMacNwkParams( uint16 nwkAddr, uint16 panId, uint8 channel );

/*
 * Send out a Scan Response command
 */
ZStatus_t zll_SendScanRsp( uint8 srcEP, afAddrType_t *dstAddr, uint32 transID, uint8 seqNum );

/*
 * Send out a Device Information Response command
 */
uint8 zll_SendDeviceInfoRsp( uint8 srcEP, afAddrType_t *dstAddr, uint8 startIndex,
                                    uint32 transID, uint8 seqNum );

/*
 * Send out a Leave Request command
 */
ZStatus_t zll_SendLeaveReq( void );

/*
 * Reset to factory new
 */
void zll_ResetToFactoryNew( bool initiator );

/*
 * Pop an avaialble short address out of the free network addresses range
 */
uint16 zll_PopNwkAddress( void );

/*
 * Update the ZLL free range global variables
 */
void zll_UpdateFreeRanges( zclLLNwkParams_t *pParams );

/*
 * Checks to see if the free ranges can be split
 */
bool zll_IsValidSplitFreeRanges( uint8 totalGrpIDs );

/*
 * Split our own free network address and group ID ranges
 */
void zll_SplitFreeRanges( uint16 *pAddrBegin, uint16 *pAddrEnd,
                               uint16 *pGrpIdBegin, uint16 *pGrpIdEnd );

/*
 * Pop the requested number of group IDs out of the free group IDs range.
 */
void zll_PopGrpIDRange( uint8 numGrpIDs, uint16 *pGrpIDsBegin, uint16 *pGrpIDsEnd );

/*
 * Get the RSSI of the message just received through a ZCL callback
 */
int8 zll_GetMsgRssi( void );

/*
 * Determine the new network update id.
 */
uint8 zll_NewNwkUpdateId( uint8 ID1, uint8 ID2 );

/*
 * Update the network key.
 */
void zll_UpdateNwkKey( uint8 *pNwkKey, uint8 keyIndex );

/*
 * Initiate Classical ZigBee commissioning of ZLL device
 */
ZStatus_t zll_ClassicalCommissioningInit( void );

/*
 *  Join best network discovered by classical commissioning
 */
ZStatus_t zll_ClassicalCommissioningJoinDiscoveredNwk( void );

/*
 * Register Target/Initiator taskID for commissioning events
 */
void zll_SetZllTaskId( uint8 taskID );

/*
 * Perform a Network Discovery scan
 */
void zll_PerformNetworkDisc( uint32 scanChannelList );

/*
 * Free any network discovery data
 */
void zll_FreeNwkParamList( void );

/*
 * Transaction ID Filter for Touch-Link received commands
 */
bool zll_IsValidTransID( uint32 transID );

/*
 * Process incoming ZDO messages (for routers)
 */
void zll_RouterProcessZDOMsg( zdoIncomingMsg_t *inMsg );

/*
 * Set the router permit join flag
 */
ZStatus_t zll_PermitJoin( uint8 duration );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZLL_H */

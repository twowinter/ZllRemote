/**************************************************************************************************
  Filename:       zll.c
  Revised:        $Date: 2013-12-06 15:53:38 -0800 (Fri, 06 Dec 2013) $
  Revision:       $Revision: 36460 $

  Description:    Zigbee Cluster Library - Light Link Profile.


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
#include "OSAL_Nv.h"
#include "hal_aes.h"
#include "nwk_util.h"
#include "ZDSecMgr.h"
#include "ZDObject.h"

#if defined( INTER_PAN )
  #include "stub_aps.h"
#endif

#include "zcl_ll.h"
#include "zll.h"

/*********************************************************************
 * MACROS
 */
#define ZLL_NEW_MIN( min, max )                  ( ( (uint32)(max) + (uint32)(min) + 1 ) / 2 )

/*********************************************************************
 * CONSTANTS
 */

#define ZLL_NUM_DEVICE_INFO_ENTRIES              5


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Used for Network Discovery
zllDiscoveredNwkParam_t *pDiscoveredNwkParamList = NULL;

// Current Touch Link Transaction ID
uint32 zllTransID;

// Scan Response ID
uint32 zllResponseID;

// Our group ID range
uint16 zllGrpIDsBegin;
uint16 zllGrpIDsEnd;

// Flag for leave
uint8 zllLeaveInitiated;

// Flags for Classical Commissioning
uint8 zllHAScanInitiated;
bool zllJoinedHANetwork;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint32 zllLastAcceptedTransID;

// ZLL Profile attributes - Our free network address and group ID ranges
static uint16 zllFreeNwkAddrBegin;
static uint16 zllFreeNwkAddrEnd;
static uint16 zllFreeGrpIdBegin;
static uint16 zllFreeGrpIdEnd;

// Device Information Table
static zclLLDeviceInfo_t *zllSubDevicesTbl[ZLL_NUM_DEVICE_INFO_ENTRIES];
static bool zllIsInitiator;
static uint8 zllTaskId;

// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
#define ZLL_EP_MAX_INCLUSTERS       1
static const cId_t zll_EP_InClusterList[ZLL_EP_MAX_INCLUSTERS] =
{
  ZCL_CLUSTER_ID_LIGHT_LINK
};

#define ZLL_EP_MAX_OUTCLUSTERS       1
static const cId_t zll_EP_OutClusterList[ZLL_EP_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_LIGHT_LINK
};

static SimpleDescriptionFormat_t zll_EP_SimpleDesc =
{
  ZLL_INTERNAL_ENDPOINT,         //  int Endpoint;
  ZLL_PROFILE_ID,                //  uint16 AppProfId[2];
  ZLL_INTERNAL_DEVICE_ID,        //  uint16 AppDeviceId[2];
  ZLL_DEVICE_VERSION,            //  int   AppDevVer:4;
  ZLL_INTERNAL_FLAGS,            //  int   AppFlags:4;
  ZLL_EP_MAX_INCLUSTERS,         //  byte  AppNumInClusters;
  (cId_t *)zll_EP_InClusterList, //  byte *pAppInClusterList;
  ZLL_EP_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
  (cId_t *)zll_EP_OutClusterList //  byte *pAppInClusterList;
};

#if defined( INTER_PAN )
// Define endpoint structure to register with STUB APS for INTER-PAN support
static endPointDesc_t zll_EP =
{
  ZLL_INTERNAL_ENDPOINT,
  &zllTaskId,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};
#endif


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zll_BuildAesKey( uint8 *pAesKey, uint32 transID, uint32 responseID, uint8 keyIndex );
static void zll_InitFreeRanges( bool initiator );
void zll_ItemInit( uint16 id, uint16 len, void *pBuf );
static void *zll_BeaconIndCB ( void *param );
static void *zll_NwkDiscoveryCnfCB ( void *param );
static void zll_InitNV( void );
static void zll_SetTCLK( void );
static ZStatus_t zll_ClassicalCommissioningNetworkDisc( void );

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      zll_RegisterSimpleDesc
 *
 * @brief   Register the Simple descriptor with the AF.
 *
 * @param   simpleDesc - a pointer to a valid SimpleDescriptionFormat_t, must not be NULL.
 *
 * @return  status
 */
ZStatus_t zll_RegisterSimpleDesc( SimpleDescriptionFormat_t *simpleDesc )
{
  endPointDesc_t *epDesc;

  // Register the application's endpoint descriptor
  //  - This memory is allocated and never freed.
  epDesc = osal_mem_alloc( sizeof ( endPointDesc_t ) );
  if ( epDesc )
  {
    // Fill out the endpoint description.
    epDesc->endPoint = simpleDesc->EndPoint;
    epDesc->task_id = &zcl_TaskID;   // all messages get sent to ZCL first
    epDesc->simpleDesc = simpleDesc;
    epDesc->latencyReq = noLatencyReqs;

    // Register the endpoint description with the AF
    return afRegister( epDesc );
  }
  return ZMemError;
}

/*********************************************************************
 * @fn      zll_InitVariables
 *
 * @brief   Initialize the ZLL global and local variables.
 *
 * @param   initiator - if caller is Initiator
 *
 * @return  none
 */
void zll_InitVariables( bool initiator )
{
  zllTransID = 0;
  zllJoinedHANetwork = FALSE;
  zllIsInitiator = initiator;

  if ( zll_IsFactoryNew() )
  {
    _NIB.nwkDevAddress = INVALID_NODE_ADDR;
  }
  else
  {
    if ( !APSME_IsDistributedSecurity() )
    {
      zllJoinedHANetwork = TRUE;
    }
  }

  // verify groups communication is initiated by broadcasts rather than multicasts
  _NIB.nwkUseMultiCast = FALSE;
  // detect and remove stored deprecated end device children after power up
  zgRouterOffAssocCleanup = TRUE;
  osal_nv_write(ZCD_NV_ROUTER_OFF_ASSOC_CLEANUP, 0, sizeof(zgRouterOffAssocCleanup), &zgRouterOffAssocCleanup);

  zll_InitFreeRanges( initiator );

  zll_InitNV();

  zllLeaveInitiated = FALSE;
  zllHAScanInitiated = FALSE;

  // Initialize device info table
  osal_memset( zllSubDevicesTbl, 0, sizeof( zllSubDevicesTbl ) );

  //set ZLL default Link Key
  zll_SetTCLK();

  // set broadcast address mask to support broadcast filtering
  NLME_SetBroadcastFilter( ZDO_Config_Node_Descriptor.CapabilityFlags );
}

/*********************************************************************
 * @fn      zll_InitFreeRanges
 *
 * @brief   Initialize the ZLL free range global variables.
 *
 * @param   initiator - if caller is link initiator
 *
 * @return  none
 */
static void zll_InitFreeRanges( bool initiator )
{
  // Initialize our free network address and group ID ranges
  if ( initiator )
  {
    zllFreeNwkAddrBegin = ZLL_ADDR_MIN;
    zllFreeNwkAddrEnd = ZLL_ADDR_MAX;

    zllFreeGrpIdBegin = ZLL_GRP_ID_MIN;
    zllFreeGrpIdEnd = ZLL_GRP_ID_MAX;
  }
  else
  {
    zllFreeNwkAddrBegin = zllFreeNwkAddrEnd = 0;
    zllFreeGrpIdBegin = zllFreeGrpIdEnd = 0;
  }

  // Initialize our local group ID range
  zllGrpIDsBegin = zllGrpIDsEnd = 0;
}

/*********************************************************************
 * @fn      zll_RegisterApp
 *
 * @brief   Register an Application's EndPoint with the ZLL.
 *
 * @param   simpleDesc - application simple description
 * @param   numGrpIDs - number of unique group IDs required by application
 *
 * @return  ZSuccess - Registered
 *          ZInvalidParameter - invalid or duplicate endpoint
 *          ZMemError - not enough memory to add
 */
ZStatus_t zll_RegisterApp( SimpleDescriptionFormat_t *simpleDesc, zclLLDeviceInfo_t *pDeviceInfo )
{
  zclLLDeviceInfo_t **pSubDevice = NULL;
  ZStatus_t status;

  // Make sure the endpoint is valid
  if ( ( simpleDesc->EndPoint != pDeviceInfo->endpoint ) || ( pDeviceInfo->profileID != ZLL_PROFILE_ID ) )
  {
    return ( ZInvalidParameter ); // invalid endpoint
  }

  // find the first empty entry in the device info table
  for ( uint8 i = 0; i < ZLL_NUM_DEVICE_INFO_ENTRIES; i++ )
  {
    if ( zllSubDevicesTbl[i] == NULL )
    {
       pSubDevice = zllSubDevicesTbl+i;
       break;
    }
  }
  if ( pSubDevice == NULL )
  {
    // Device info table is full
    return ( ZMemError );
  }

  status = zll_RegisterSimpleDesc( simpleDesc );
  if ( status != ZSuccess )
  {
    return status;
  }

  // Add the device info to the table
  *pSubDevice = pDeviceInfo;

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      zll_IsFactoryNew
 *
 * @brief   Check to see if the device is factory new.
 *
 * @param   none
 *
 * @return  TRUE if factory new. FALSE, otherwise.
 */
bool zll_IsFactoryNew( void )
{
  uint16 nwkAddr = INVALID_NODE_ADDR;

  osal_nv_read( ZCD_NV_NIB, osal_offsetof( nwkIB_t, nwkDevAddress ),
                sizeof( uint16), &nwkAddr );

  // Does the NIB have anything more than default?
  return ( nwkAddr == INVALID_NODE_ADDR ? TRUE : FALSE );
}

/*********************************************************************
 * @fn      zll_UpdateFreeRanges
 *
 * @brief   Update the ZLL free range global variables.
 *
 * @param   pParams - new parameters
 *
 * @return  none
 */
void zll_UpdateFreeRanges( zclLLNwkParams_t *pParams )
{
  // Set our free network address and group ID ranges
  zllFreeNwkAddrBegin = pParams->freeNwkAddrBegin;
  zllFreeNwkAddrEnd = pParams->freeNwkAddrEnd;
  zllFreeGrpIdBegin = pParams->freeGrpIDBegin;
  zllFreeGrpIdEnd = pParams->freeGrpIDEnd;

  // Set our group ID range
  zllGrpIDsBegin = pParams->grpIDsBegin;
  zllGrpIDsEnd = pParams->grpIDsEnd;
}

/*********************************************************************
 * @fn      zll_IsValidSplitFreeRanges
 *
 * @brief   Checks to see if the resulting two ranges are not smaller
 *          than the threshold after division of a network address or
 *          group ID range. The Initiator splits its own free range
 *          in half and assigns the top half to the new device.
 *
 *          Note: A range (Nmin...Nmax) is split as follows:
 *
 *                N'min = (Nmax + Nmin + 1)/2
 *                N'max = Nmax
 *                Nmax = N'min - 1
 *
 * @param   totalGrpIDs - total number of group IDs needed
 *
 * @return  TRUE if split possible. FALSE, otherwise.
 */
bool zll_IsValidSplitFreeRanges( uint8 totalGrpIDs )
{
  if ( ( zllFreeNwkAddrBegin != 0 ) && ( zllFreeGrpIdBegin != 0 ) )
  {
      return ( ( ( ( zllFreeNwkAddrEnd - zllFreeNwkAddrBegin ) / 2 ) >= ZLL_ADDR_THRESHOLD ) &&
               ( ( ( zllFreeGrpIdEnd - ( zllFreeGrpIdBegin + totalGrpIDs - 1 ) ) / 2 ) >= ZLL_GRP_ID_THRESHOLD ) );

  }

  return ( FALSE );
}

/*********************************************************************
 * @fn      zll_SplitFreeRanges
 *
 * @brief   Split our own free network address and group ID ranges
 *          in half and assign the top half to the new device.
 *
 *          Note: A range (Nmin...Nmax) is split as follows:
 *
 *                N'min = (Nmax + Nmin + 1)/2
 *                N'max = Nmax
 *                Nmax = N'min - 1
 *
 * output parameters
 *
 * @param   pAddrBegin - new address range begin
 * @param   pAddrEnd - new address range end
 * @param   pGrpIdBegin - new group id range begin
 * @param   pGrpIdEnd - new group id range end
 *
 * @return  none
 */
void zll_SplitFreeRanges( uint16 *pAddrBegin, uint16 *pAddrEnd,
                        uint16 *pGrpIdBegin, uint16 *pGrpIdEnd )
{
  if ( ( zllFreeNwkAddrBegin != 0 ) && ( zllFreeGrpIdBegin != 0 ) )
  {
    *pAddrBegin = ZLL_NEW_MIN( zllFreeNwkAddrBegin, zllFreeNwkAddrEnd );
    *pGrpIdBegin = ZLL_NEW_MIN( zllFreeGrpIdBegin, zllFreeGrpIdEnd );

    *pAddrEnd = zllFreeNwkAddrEnd;
    *pGrpIdEnd = zllFreeGrpIdEnd;

    // Update our max
    zllFreeNwkAddrEnd = *pAddrBegin - 1;
    zllFreeGrpIdEnd = *pGrpIdBegin - 1;
  }
  else
  {
    *pAddrBegin = *pAddrEnd = 0;
    *pGrpIdBegin = *pGrpIdEnd = 0;
  }
}

/*********************************************************************
 * @fn      zll_PopGrpIDRange
 *
 * @brief   Pop the requested number of group IDs out of the free group IDs range.
 *
 * input parameters
 *
 * @param   numGrpIDs - number of group IDs needed
 *
 * output parameters
 *
 * @param   pGrpIdBegin - new group id range begin, or 0 if unavaialable
 * @param   pGrpIdEnd - new group id range end, , or 0 if unavaialable
 *
 * @return  none
 */
void zll_PopGrpIDRange( uint8 numGrpIDs, uint16 *pGrpIDsBegin, uint16 *pGrpIDsEnd )
{
  if ( ( zllFreeGrpIdBegin != 0 )
       && ( zllFreeGrpIdBegin <= zllFreeGrpIdEnd )
       && ( ( zllFreeGrpIdEnd - zllFreeGrpIdBegin ) >= numGrpIDs ) )
  {
    *pGrpIDsBegin = zllFreeGrpIdBegin;

    // Update min free group id
    zllFreeGrpIdBegin += numGrpIDs;

    *pGrpIDsEnd = zllFreeGrpIdBegin - 1;
  }
  else
  {
    *pGrpIDsBegin = *pGrpIDsEnd = 0;
  }
}

/*********************************************************************
 * @fn      zll_PopNwkAddress
 *
 * @brief   Pop an avaialble short address out of the free network addresses range.
 *
 * @param   none
 *
 * @return  free address if available. 0, otherwise.
 */
uint16 zll_PopNwkAddress( void )
{
  if ( ( zllFreeNwkAddrBegin == 0 ) || ( zllFreeNwkAddrBegin > zllFreeNwkAddrEnd ) )
  {
    return ( 0 );
  }

  return ( zllFreeNwkAddrBegin++ );
}

/*********************************************************************
 * @fn      zll_GetNumSubDevices
 *
 * @brief   Get the total number of sub-devices (endpoints) registered.
 *
 * @param   startIndex - starting index
 *
 * @return  number of sub-devices
 */
uint8 zll_GetNumSubDevices( uint8 startIndex )
{
  uint8 numEPs = 0;

  for ( uint8 i = startIndex; i < ZLL_NUM_DEVICE_INFO_ENTRIES; i++ )
  {
    if ( zllSubDevicesTbl[i] != NULL )
    numEPs++;
  }

  return ( numEPs );
}

/*********************************************************************
 * @fn      zll_GetNumGrpIDs
 *
 * @brief   Get the total number of group IDs required by this device.
 *
 * @param   none
 *
 * @return  number of group IDs
 */
uint8 zll_GetNumGrpIDs( void )
{
  uint8 numGrpIDs = 0;

  for ( uint8 i = 0; i < ZLL_NUM_DEVICE_INFO_ENTRIES; i++ )
  {
    if ( zllSubDevicesTbl[i] != NULL )
    {
      numGrpIDs += zllSubDevicesTbl[i]->grpIdCnt;
    }
  }

  return ( numGrpIDs );
}

/*********************************************************************
 * @fn      zll_GetSubDeviceInfo
 *
 * @brief   Get the sub-device information.
 *
 * input parameter
 *
 * @param   index - index of sub-device
 *
 * output parameter
 *
 * @param   pInfo - sub-device info (to be returned)
 *
 * @return  none
 */
void zll_GetSubDeviceInfo( uint8 index, zclLLDeviceInfo_t *pInfo )
{
  if ( pInfo == NULL )
  {
    return;
  }
  if ( ( index < ZLL_NUM_DEVICE_INFO_ENTRIES ) &&
       ( zllSubDevicesTbl[index] != NULL ) )
  {
    endPointDesc_t *epDesc = afFindEndPointDesc( zllSubDevicesTbl[index]->endpoint );
    if ( epDesc != NULL )
    {
      // Copy sub-device info
      *pInfo = *(zllSubDevicesTbl[index]);
    }
  }
  else
  {
    osal_memset( pInfo, 0, sizeof( zclLLDeviceInfo_t ) );
  }
}


/*********************************************************************
 * @fn      zll_EncryptNwkKey
 *
 * @brief   Encrypt the current network key to be sent to a Target.
 *          In case of Factory New device generate new key.
 *
 * output parameter
 *
 * @param   pNwkKey - pointer to encrypted network key
 *
 * input parameters
 *
 * @param   keyIndex - key index
 * @param   transID - transaction id
 * @param   responseID - response id
 *
 * @return  none
 */
void zll_EncryptNwkKey( uint8 *pNwkKey, uint8 keyIndex, uint32 transID, uint32 responseID )
{
  uint8 aesKeyKey[SEC_KEY_LEN] = ZLL_DEFAULT_AES_KEY;
  uint8 masterKey[SEC_KEY_LEN] = ZLL_ENC_KEY;
#ifdef ZLL_DEV_FIXED_NWK_KEY
uint8 nwkKey[SEC_KEY_LEN] = ZLL_DEV_FIXED_NWK_KEY;
#else
  uint8 nwkKey[SEC_KEY_LEN];
  if ( zll_IsFactoryNew() )
  {
    zll_GenerateRandNum( nwkKey, SEC_KEY_LEN );
  }
  else
  {
    nwkActiveKeyItems keyItems;
    SSP_ReadNwkActiveKey( &keyItems );
    osal_memcpy( nwkKey, keyItems.active.key , SEC_KEY_LEN);
  }
#endif
  // Build the AES key
  zll_BuildAesKey( aesKeyKey, transID, responseID, keyIndex );

  if ( ( keyIndex == ZLL_KEY_INDEX_MASTER ) || ( keyIndex == ZLL_KEY_INDEX_CERT ) )
  {
    // Encypt with the master key
    sspAesEncrypt( masterKey, aesKeyKey );
  }
  // Encrypt the network key with the AES key
  sspAesEncrypt( aesKeyKey, nwkKey );

  // Copy in the encrypted network key
  osal_memcpy( pNwkKey, nwkKey, SEC_KEY_LEN );
}

/*********************************************************************
 * @fn      zll_DecryptNwkKey
 *
 * @brief   Decrypt the received network key and update.
 *
 * @param   pNwkKey - pointer to the encrypted network key
 * @param   keyIndex - key index
 * @param   transID - transaction id
 * @param   responseID - response id
 *
 * @return  none
 */
void zll_DecryptNwkKey( uint8 *pNwkKey, uint8 keyIndex, uint32 transID, uint32 responseID )
{
  uint8 aesKeyKey[SEC_KEY_LEN] = ZLL_DEFAULT_AES_KEY;

  uint8 nwkKey[SEC_KEY_LEN];

  uint8 masterKey[SEC_KEY_LEN] = ZLL_ENC_KEY;

  // Copy in the encrypted network key
  osal_memcpy( nwkKey, pNwkKey, SEC_KEY_LEN );

  zll_BuildAesKey( aesKeyKey, transID, responseID, keyIndex );

  if ( ( keyIndex == ZLL_KEY_INDEX_MASTER ) || ( keyIndex == ZLL_KEY_INDEX_CERT ) )
  {
    //encypt with the master key
    sspAesEncrypt( masterKey, aesKeyKey );
  }
  // Decrypt the network key with the AES key
  sspAesDecrypt( aesKeyKey, nwkKey );

  zll_UpdateNwkKey( nwkKey, keyIndex );
}

/*********************************************************************
 * @fn      zll_BuildAesKey
 *
 * @brief   Build an AES key using Transaction ID and Response ID.
 *
 * @param   pAesKey - pointer to AES to be built
 * @param   transID - transaction id
 * @param   responseID - response id
 *
 * @return  none
 */
static void zll_BuildAesKey( uint8 *pAesKey, uint32 transID, uint32 responseID, uint8 keyIndex )
{

  if ( ( keyIndex == ZLL_KEY_INDEX_MASTER ) || ( keyIndex == ZLL_KEY_INDEX_CERT ) )
  {
    // Copy transaction identifier to 1st byte
    pAesKey[0] = BREAK_UINT32( transID, 3 );
    pAesKey[1] = BREAK_UINT32( transID, 2 );
    pAesKey[2] = BREAK_UINT32( transID, 1 );
    pAesKey[3] = BREAK_UINT32( transID, 0 );

    // Copy response identifier 3rd bute
    pAesKey[8] = BREAK_UINT32( responseID, 3 );
    pAesKey[9] = BREAK_UINT32( responseID, 2 );
    pAesKey[10] = BREAK_UINT32( responseID, 1 );
    pAesKey[11] = BREAK_UINT32( responseID, 0 );
  }

  // Copy in the transaction identifier
  pAesKey[4] = BREAK_UINT32( transID, 3 );
  pAesKey[5] = BREAK_UINT32( transID, 2 );
  pAesKey[6] = BREAK_UINT32( transID, 1 );
  pAesKey[7] = BREAK_UINT32( transID, 0 );

  // Copy in the response identifier
  pAesKey[12] = BREAK_UINT32( responseID, 3 );
  pAesKey[13] = BREAK_UINT32( responseID, 2 );
  pAesKey[14] = BREAK_UINT32( responseID, 1 );
  pAesKey[15] = BREAK_UINT32( responseID, 0 );
}

/*********************************************************************
 * @fn      zll_UpdateNwkKey
 *
 * @brief   Update the network key.
 *
 * @param   pNwkParams - pointer to new network key
 * @param   keyIndex - key index
 *
 * @return  none
 */
void zll_UpdateNwkKey( uint8 *pNwkKey, uint8 keyIndex )
{
  uint32 nwkFrameCounterTmp;
  (void)keyIndex;

  // To prevent Framecounter out of sync issues, store the lastkey
  nwkFrameCounterTmp = nwkFrameCounter;  // (Global in SSP).

  // Update the network key
  SSP_UpdateNwkKey( pNwkKey, 0 );

  SSP_SwitchNwkKey( 0 );

  nwkFrameCounter  = nwkFrameCounterTmp; // restore

  // Save off the security
  ZDApp_SaveNwkKey();
}

/*********************************************************************
 * @fn      zll_GetNwkKeyBitmask
 *
 * @brief   Get the supported network key bitmask.
 *
 * @param   none
 *
 * @return  network key bitmask
 */
uint16 zll_GetNwkKeyBitmask( void )
{
  return ( (uint16)1 << ZLL_KEY_INDEX );
}

/*********************************************************************
 * @fn      zll_GenerateRandNum
 *
 * @brief   Fill buffer with random bytes.
 *
 * input parameter
 *
 * @param   numSize - size of buffer in bytes
 *
 * output parameter
 *
 * @param   pNum - pointer to buffer to be filled with random values
 *
 * @return  none
 */
void zll_GenerateRandNum( uint8 *pNum, uint8 numSize )
{
  if ( pNum && numSize )
  {
    uint8 lastByte = ( numSize - 1 );
    for ( uint8 i = 0; i < lastByte; i += 2 )
    {
      uint16 rand = osal_rand();
      pNum[i]   = LO_UINT16( rand );
      pNum[i+1] = HI_UINT16( rand );
    }

    // In case the number is odd
    if ( numSize % 2 )
    {
      pNum[lastByte] = LO_UINT16( osal_rand() );
    }
  }
}

/*********************************************************************
 * @fn      zll_GetRandPrimaryChannel
 *
 * @brief   Get randomly chosen ZLL primary channel.
 *
 * @return  channel
 */
uint8 zll_GetRandPrimaryChannel()
{
  uint8 channel = osal_rand() & 0x1F;
  if ( channel <= ZLL_FIRST_CHANNEL )
  {
    channel = ZLL_FIRST_CHANNEL;
  }
  else if ( channel <= ZLL_SECOND_CHANNEL )
  {
    channel = ZLL_SECOND_CHANNEL;
  }
  else if ( channel <= ZLL_THIRD_CHANNEL )
  {
    channel = ZLL_THIRD_CHANNEL;
  }
  else
  {
    channel = ZLL_FOURTH_CHANNEL;
  }
#ifdef ZLL_DEV_SELECT_FIRST_CHANNEL
#warning The device will always select the first primary channel
  channel = ZLL_FIRST_CHANNEL;
#endif
  return channel;
}

/*********************************************************************
 * @fn      zll_SetNIB
 *
 * @brief   Copy new Network Parameters to the NIB.
 *
 * @param   nwkState - network state
 * @param   nwkAddr - short address
 * @param   pExtendedPANID - pointer to extended PAN ID
 * @param   logicalChannel - channel
 * @param   panId - PAN identifier
 * @param   nwkUpdateId - nwtwork update identifier
 *
 * @return      void
 */
void zll_SetNIB( nwk_states_t nwkState, uint16 nwkAddr, uint8 *pExtendedPANID,
                 uint8 logicalChannel, uint16 panId, uint8 nwkUpdateId )
{
  // Copy the new network parameters to NIB
  _NIB.nwkState = nwkState;
  _NIB.nwkDevAddress = nwkAddr;
  _NIB.nwkLogicalChannel = logicalChannel;
  _NIB.nwkCoordAddress = INVALID_NODE_ADDR;
  _NIB.channelList = (uint32)1 << logicalChannel;
  _NIB.nwkPanId = panId;
  _NIB.nodeDepth = 1;
  _NIB.MaxRouters = (uint8)gNWK_MAX_DEVICE_LIST;
  _NIB.MaxChildren = (uint8)gNWK_MAX_DEVICE_LIST;
  _NIB.allocatedRouterAddresses = 1;
  _NIB.allocatedEndDeviceAddresses = 1;

  if ( _NIB.nwkUpdateId != nwkUpdateId )
  {
    NLME_SetUpdateID( nwkUpdateId );
  }

  osal_cpyExtAddr( _NIB.extendedPANID, pExtendedPANID );

  // Save the NIB
  if ( nwkState == NWK_ROUTER )
  {
    zll_UpdateNV( ZLL_UPDATE_NV_NIB );
  }
  // else will be updated when ED joins its parent
}

/*********************************************************************
 * @fn      zll_ProcessNwkUpdate
 *
 * @brief   Update our local network update id and logical channel.
 *
 * @param   nwkUpdateId - new network update id
 * @param   logicalChannel - new logical channel
 *
 * @return  void
 */
void zll_ProcessNwkUpdate( uint8 nwkUpdateId, uint8 logicalChannel )
{
  // Update the network update id
  NLME_SetUpdateID( nwkUpdateId );

  // Switch channel
  if ( _NIB.nwkLogicalChannel != logicalChannel )
  {
    _NIB.nwkLogicalChannel = logicalChannel;
    zll_SetChannel( logicalChannel );
  }

  // Update channel list
  _NIB.channelList = (uint32)1 << logicalChannel;

  // Our Channel has been changed -- notify to save info into NV
  ZDApp_NwkStateUpdateCB();
  //zll_UpdateNV();

  // Reset the total transmit count and the transmit failure counters
  _NIB.nwkTotalTransmissions = 0;
  nwkTransmissionFailures( TRUE );
}

/*********************************************************************
 * @fn      zll_UpdateNV
 *
 * @brief   Updates NV with NIB and free ranges items
 *
 * @param   enables - specifies what to update
 *
 * @return  none
 */
void zll_UpdateNV( uint8 enables )
{
#if defined ( NV_RESTORE )

 #if defined ( NV_TURN_OFF_RADIO )
  // Turn off the radio's receiver during an NV update
  uint8 RxOnIdle;
  uint8 x = FALSE;
  ZMacGetReq( ZMacRxOnIdle, &RxOnIdle );
  ZMacSetReq( ZMacRxOnIdle, &x );
 #endif

  if ( enables & ZLL_UPDATE_NV_NIB )
  {
    // Update NIB in NV
    osal_nv_write( ZCD_NV_NIB, 0, sizeof( nwkIB_t ), &_NIB );

    // Reset the NV startup option to resume from NV by clearing
    // the "New" join option.
    zgWriteStartupOptions( ZG_STARTUP_CLEAR, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );
  }

  if ( enables & ZLL_UPDATE_NV_RANGES )
  {
    // Store our free network address and group ID ranges
    osal_nv_write( ZCD_NV_MIN_FREE_NWK_ADDR, 0, sizeof( zllFreeNwkAddrBegin ), &zllFreeNwkAddrBegin );
    osal_nv_write( ZCD_NV_MAX_FREE_NWK_ADDR, 0, sizeof( zllFreeNwkAddrEnd ), &zllFreeNwkAddrEnd );
    osal_nv_write( ZCD_NV_MIN_FREE_GRP_ID, 0, sizeof( zllFreeGrpIdBegin ), &zllFreeGrpIdBegin );
    osal_nv_write( ZCD_NV_MAX_FREE_GRP_ID, 0, sizeof( zllFreeGrpIdEnd ), &zllFreeGrpIdEnd );

    // Store our group ID range
    osal_nv_write( ZCD_NV_MIN_GRP_IDS, 0, sizeof( zllGrpIDsBegin ), &zllGrpIDsBegin );
    osal_nv_write( ZCD_NV_MAX_GRP_IDS, 0, sizeof( zllGrpIDsEnd ), &zllGrpIDsEnd );
  }

 #if defined ( NV_TURN_OFF_RADIO )
  ZMacSetReq( ZMacRxOnIdle, &RxOnIdle );
 #endif

#endif // NV_RESTORE
}

/*********************************************************************
 * @fn          zll_InitNV
 *
 * @brief       Initialize free range RAM variables from NV. If NV items
 *              don't exist, then the NV is initialize with what is in
 *              RAM variables.
 *
 * @param       none
 *
 * @return      none
 */
static void zll_InitNV( void )
{
  // Initialize our free network address and group ID ranges
  zll_ItemInit( ZCD_NV_MIN_FREE_NWK_ADDR, sizeof( zllFreeNwkAddrBegin ), &zllFreeNwkAddrBegin );
  zll_ItemInit( ZCD_NV_MAX_FREE_NWK_ADDR, sizeof( zllFreeNwkAddrEnd ), &zllFreeNwkAddrEnd );
  zll_ItemInit( ZCD_NV_MIN_FREE_GRP_ID, sizeof( zllFreeGrpIdBegin ), &zllFreeGrpIdBegin );
  zll_ItemInit( ZCD_NV_MAX_FREE_GRP_ID, sizeof( zllFreeGrpIdEnd ), &zllFreeGrpIdEnd );

  // Initialize our group ID range
  zll_ItemInit( ZCD_NV_MIN_GRP_IDS, sizeof( zllGrpIDsBegin ), &zllGrpIDsBegin );
  zll_ItemInit( ZCD_NV_MAX_GRP_IDS, sizeof( zllGrpIDsEnd ), &zllGrpIDsEnd );
}

/*********************************************************************
 * @fn      zll_ItemInit
 *
 * @brief   Initialize an NV item. If the item doesn't exist in NV memory,
 *          write the default (value passed in) into NV memory. But if
 *          it exists, set the item to the value stored in NV memory.
 *
 * @param   id - item id
 * @param   len - item len
 * @param   buf - pointer to the item
 *
 * @return  none
 */
void zll_ItemInit( uint16 id, uint16 len, void *pBuf )
{
#if defined ( NV_RESTORE )
  // If the item doesn't exist in NV memory, create and initialize
  // it with the value passed in.
  if ( osal_nv_item_init( id, len, pBuf ) == ZSuccess )
  {
    // The item already exists in NV memory, read it from NV memory
    osal_nv_read( id, 0, len, pBuf );
  }
#endif // NV_RESTORE
}

/*********************************************************************
 * @fn      zll_SetMacNwkParams
 *
 * @brief   Configure MAC with our Network Parameters.
 *
 * @param   nwkAddr - network address
 * @param   panId - PAN identifier
 * @param   channel
 *
 * @return  void
 */
void zll_SetMacNwkParams( uint16 nwkAddr, uint16 panId, uint8 channel )
{
  // Set our short address
  ZMacSetReq( ZMacShortAddress, (byte*)&nwkAddr );

  // Set our PAN ID
  ZMacSetReq( ZMacPanId, (byte*)&panId );

  // Tune to the selected logical channel
  zll_SetChannel( channel );
}

/*********************************************************************
 * @fn      zll_SetChannel
 *
 * @brief   Set our channel.
 *
 * @param   channel - new channel to change to
 *
 * @return  void
 */
void zll_SetChannel( uint8 channel )
{
  uint8 curChannel;

  // Try to change channel
  ZMacGetReq( ZMacChannel, &curChannel );

  if ( curChannel != channel )
  {
    // Set the new channel
    ZMacSetReq( ZMacChannel, &channel );

    // NOTE - When operating on channel 26, the transmission power may be
    //        reduced in order to comply with FCC regulations.
  }
}

/*********************************************************************
 * @fn      zll_SendScanRsp
 *
 * @brief   Send out a Scan Response command.
 *
 * @param   srcEP - sender's endpoint
 * @param   dstAddr - pointer to destination address struct
 * @param   transID - received transaction id
 * @param   seqNum - received sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zll_SendScanRsp( uint8 srcEP, afAddrType_t *dstAddr, uint32 transID, uint8 seqNum )
{
  ZStatus_t status = ZSuccess;

  // Make sure we respond only once during a Device Discovery
  if ( zllLastAcceptedTransID != transID )
  {
    zclLLScanRsp_t *pRsp;

    pRsp = (zclLLScanRsp_t *)osal_mem_alloc( sizeof( zclLLScanRsp_t ) );
    if ( pRsp )
    {
      osal_memset( pRsp, 0, sizeof( zclLLScanRsp_t ) );

      // Save transaction id
      zllLastAcceptedTransID = transID;
      osal_start_timerEx( zllTaskId, ZLL_TRANS_LIFETIME_EXPIRED_EVT,
                          ZLL_APLC_INTER_PAN_TRANS_ID_LIFETIME );

      pRsp->transID = transID;
      pRsp->rssiCorrection = ZLL_RSSI_CORRECTION;
      pRsp->zLogicalType = zgDeviceLogicalType;
      pRsp->zllAddressAssignment = zll_IsValidSplitFreeRanges(0);
      pRsp->zllLinkInitiator = zllIsInitiator;

      if ( ZDO_Config_Node_Descriptor.CapabilityFlags & CAPINFO_RCVR_ON_IDLE )
      {
        pRsp->zRxOnWhenIdle = TRUE;
      }

      pRsp->zllLinklinkPriority = FALSE;
      pRsp->keyBitmask = zll_GetNwkKeyBitmask();

      // Generate a new Response ID
      zllResponseID = ( ((uint32)osal_rand()) << 16 ) + osal_rand();
      pRsp->responseID = zllResponseID;

      pRsp->zllFactoryNew = zll_IsFactoryNew();
      if ( pRsp->zllFactoryNew )
      {
        pRsp->nwkAddr = 0xFFFF;
        pRsp->nwkUpdateId = 0;
      }
      else
      {
        pRsp->nwkAddr = _NIB.nwkDevAddress;
        pRsp->nwkUpdateId = _NIB.nwkUpdateId;
      }
      pRsp->PANID = _NIB.nwkPanId;
      pRsp->logicalChannel = _NIB.nwkLogicalChannel;
      osal_cpyExtAddr( pRsp->extendedPANID, _NIB.extendedPANID );

      pRsp->numSubDevices = zll_GetNumSubDevices( 0 );
      if ( pRsp->numSubDevices == 1 )
      {
        zll_GetSubDeviceInfo( 0, &(pRsp->deviceInfo) );
      }

      pRsp->totalGrpIDs = zll_GetNumGrpIDs();

      // Send a response back
      status = zclLL_Send_ScanRsp( srcEP, dstAddr, pRsp, seqNum );

      osal_mem_free( pRsp );
    }
    else
    {
      status = ZMemError;
    }
  }

  return ( status );
}

/*********************************************************************
 * @fn      zll_SendDeviceInfoRsp
 *
 * @brief   Send out a Device Information Response command.
 *
 * @param   srcEP - sender's endpoint
 * @param   dstAddr - destination address
 * @param   startIndex - start index
 * @param   transID - received transaction id
 * @param   seqNum - received sequence number
 *
 * @return  ZStatus_t
 */
uint8 zll_SendDeviceInfoRsp( uint8 srcEP, afAddrType_t *dstAddr, uint8 startIndex,
                             uint32 transID, uint8 seqNum )
{
  zclLLDeviceInfoRsp_t *pRsp;
  uint8 cnt;
  uint8 rspLen;
  uint8 status = ZSuccess;

  cnt = zll_GetNumSubDevices( startIndex );
  if ( cnt > ZLL_DEVICE_INFO_RSP_REC_COUNT_MAX )
  {
    cnt = ZLL_DEVICE_INFO_RSP_REC_COUNT_MAX; // should be between 0x00-0x05
  }

  rspLen = sizeof( zclLLDeviceInfoRsp_t ) + ( cnt * sizeof( devInfoRec_t ) );

  pRsp = (zclLLDeviceInfoRsp_t *)osal_mem_alloc( rspLen );
  if ( pRsp )
  {
    pRsp->transID = transID;

    pRsp->numSubDevices = zll_GetNumSubDevices( 0 );
    pRsp->startIndex = startIndex;
    pRsp->cnt = cnt;

    for ( uint8 i = 0; i < cnt; i++ )
    {
      devInfoRec_t *pRec = &(pRsp->devInfoRec[i]);

      osal_cpyExtAddr( pRec->ieeeAddr, NLME_GetExtAddr() );

      zll_GetSubDeviceInfo( startIndex + i, &(pRec->deviceInfo) );

      pRec->sort = 0;
    }

    // Send a response back
    status = zclLL_Send_DeviceInfoRsp( srcEP, dstAddr, pRsp, seqNum );

    osal_mem_free( pRsp );
  }
  else
  {
    status = ZMemError;
  }

  return ( status );
}

/*********************************************************************
 * @fn      zll_SendLeaveReq
 *
 * @brief   Send out a Leave Request command.
 *
 * @param   void
 *
 * @return  ZStatus_t
 */
ZStatus_t zll_SendLeaveReq( void )
{
  NLME_LeaveReq_t leaveReq;

  // Set every field to 0
  osal_memset( &leaveReq, 0, sizeof( NLME_LeaveReq_t ) );

  // Send out our leave
  return ( NLME_LeaveReq( &leaveReq ) );
}

/*********************************************************************
 * @fn      zll_ResetToFactoryNew
 *
 * @brief   Reset to factory new.
 *
 * @param   initiator - initialize initiator
 *
 * @return  none
 */
void zll_ResetToFactoryNew( bool initiator )
{
  // Set the NV startup option to force a "new" join.
  zgWriteStartupOptions( ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );

  // Leave the network, and reset afterwards
  if ( zll_SendLeaveReq() != ZSuccess )
  {
    // Couldn't send out leave; prepare to reset anyway
    ZDApp_LeaveReset( FALSE );
  }

  // Reset our free ranges
  zll_InitFreeRanges( initiator );

  _NIB.nwkDevAddress = INVALID_NODE_ADDR;
  _NIB.nwkCoordAddress = INVALID_NODE_ADDR;
  _NIB.nwkPanId = 0xFFFF;
  osal_memset(_NIB.extendedPANID, 0, Z_EXTADDR_LEN);
  _NIB.nwkUpdateId = 0;
  ZDSecMgrUpdateTCAddress( NULL );

  // Save free ranges in NV
  zll_UpdateNV( ZLL_UPDATE_NV_RANGES | ZLL_UPDATE_NV_NIB );
}

/*********************************************************************
 * @fn      zll_GetMsgRssi
 *
 * @brief   Get the RSSI of the message just received through a ZCL callback.
 *
 * @param   none
 *
 * @return  RSSI if AF message was received, ZLL_TL_WORST_RSSI otherwise.
 */
int8 zll_GetMsgRssi( void )
{
  afIncomingMSGPacket_t *pAF = zcl_getRawAFMsg();

  if ( pAF != NULL )
  {
    return ( pAF->rssi );
  }

  return ( ZLL_TL_WORST_RSSI );
}

/*********************************************************************
 * @fn      zll_NewNwkUpdateId
 *
 * @brief   Determine the new network update id. The nwkUpdateId attribute
 *          can take the value of 0x00 - 0xff and may wrap around so care
 *          must be taken when comparing for newness.
 *
 * @param   ID1 - first nwk update id
 * @param   ID2 - second nwk update id
 *
 * @return  new nwk update ID
 */
uint8 zll_NewNwkUpdateId( uint8 ID1, uint8 ID2 )
{
  if ( ( (ID1 >= ID2) && ((ID1 - ID2) > 200) )
      || ( (ID1 < ID2) && ((ID2 - ID1) > 200) ) )
  {
    return ( MIN( ID1, ID2 ) );
  }

  return ( MAX( ID1, ID2 ) );
}

/*********************************************************************
 * @fn      zll_SetZllTaskId
 *
 * @brief   Register Target/Initiator taskID for commissioning events
 *
 * @param   taskID
 *
 * @return  none
 */
void zll_SetZllTaskId( uint8 taskID )
{
  zllTaskId = taskID;

  // register internal EP for ZLL messages
  zll_RegisterSimpleDesc( &zll_EP_SimpleDesc );

#if defined( INTER_PAN )
  // Register with Stub APS
  StubAPS_RegisterApp( &zll_EP );
#endif // INTER_PAN
}

/*********************************************************************
 * @fn      zll_ClassicalCommissioningInit
 *
 * @brief   Initiate Classical ZigBee commissioning of ZLL device
 *
 * @param   none
 *
 * @return  status
 */
ZStatus_t zll_ClassicalCommissioningInit()
{
  zllHAScanInitiated = ZLL_SCAN_PRIMARY_CHANNELS;
  NLME_NwkDiscTerm();
  osal_stop_timerEx( ZDAppTaskID, ZDO_NETWORK_INIT );
  _NIB.nwkState = NWK_INIT;
  return ( zll_ClassicalCommissioningNetworkDisc() );
}

/*********************************************************************
 * @fn      zll_ClassicalCommissioningNetworkDisc
 *
 * @brief   Initiate Network Discovery on Primary or Seconday channels
 *          to allow choosing network to join.
 *
 * @param   none
 *
 * @return  status
 */
static ZStatus_t zll_ClassicalCommissioningNetworkDisc( void )
{
  if ( pDiscoveredNwkParamList == NULL )
  {
    if ( zllHAScanInitiated == ZLL_SCAN_PRIMARY_CHANNELS )
    {
      // Use the primary ZLL channel list
      zll_PerformNetworkDisc(ZLL_PRIMARY_CHANNEL_LIST);
      return ( ZSuccess );
    }
    else if ( zllHAScanInitiated == ZLL_SCAN_SECONDARY_CHANNELS )
    {
      // Use the secondary ZLL channel list
      zll_PerformNetworkDisc(ZLL_SECONDARY_CHANNEL_LIST);
      return ( ZSuccess );
    }
  }

  return ( ZFailure );
}

/*********************************************************************
 * @fn      zll_ClassicalCommissioningJoinDiscoveredNwk
 *
 * @brief   Join best network discovered by classical commissioning
 *
 * @param   void
 *
 * @return  ZStatus
 */
ZStatus_t zll_ClassicalCommissioningJoinDiscoveredNwk( void )
{
  zllHAScanInitiated = FALSE;
  // it will try to scan primary and secondary channel list
  if ( pDiscoveredNwkParamList == NULL )
  {
    return ( ZFailure );
  }
  zllDiscoveredNwkParam_t *pParam = pDiscoveredNwkParamList;
  zllDiscoveredNwkParam_t *pChosenNwk = NULL;

  // Add the network parameter to the Network Parameter List
  while ( pParam != NULL )
  {
    if ( pChosenNwk == NULL )
    {
      // set it to the first element of the list
      pChosenNwk = pParam;
    }
    else
    {
      if ( ((pParam->chosenRouterLinkQuality > pChosenNwk->chosenRouterLinkQuality) &&
            (pParam->chosenRouterDepth < MAX_NODE_DEPTH)) ||
          ((pParam->chosenRouterLinkQuality == pChosenNwk->chosenRouterLinkQuality) &&
           (pParam->chosenRouterDepth < pChosenNwk->chosenRouterDepth)) )
      {
        pChosenNwk = pParam;  // found the better network to join
        break;
      }
    }

    pParam = pParam->nextParam;
  }

  if ( pChosenNwk != NULL )
  {
    // join that network
    if ( NLME_JoinRequest( pChosenNwk->extendedPANID, pChosenNwk->PANID,
                          pChosenNwk->logicalChannel,
                          ZDO_Config_Node_Descriptor.CapabilityFlags,
                          pChosenNwk->chosenRouter, pChosenNwk->chosenRouterDepth ) != ZSuccess )
    {
      // do something if it fails to join
      zll_FreeNwkParamList();
      return ( ZFailure );
    }
  }

  zll_FreeNwkParamList();
  zllJoinedHANetwork = TRUE;
  zllFreeNwkAddrBegin = zllFreeNwkAddrEnd = zllFreeGrpIdBegin = zllFreeGrpIdEnd = 0;
  devState = DEV_HOLD;
  ZDApp_ResetNwkKey();
  _NIB.nwkKeyLoaded = FALSE;
  zll_UpdateNV( ZLL_UPDATE_NV_NIB | ZLL_UPDATE_NV_RANGES );
  ZDApp_LeaveCtrlReset();
  return ( ZSuccess );
}


/*********************************************************************
 * @fn      zll_PerformNetworkDisc
 *
 * @brief   Perform a Network Discovery scan.
 *          Scan results will be stored locally to analyze.
 *
 * @param   scanChannelList - channels to perform discovery scan
 *
 * @return  void
 */
void zll_PerformNetworkDisc( uint32 scanChannelList )
{
  NLME_ScanFields_t scan;

  scan.channels = scanChannelList;
  scan.duration = BEACON_ORDER_240_MSEC;
  scan.scanType = ZMAC_ACTIVE_SCAN;
  scan.scanApp  = NLME_DISC_SCAN;

  if ( NLME_NwkDiscReq2( &scan ) == ZSuccess )
  {
    // Register ZDO callback to handle the network discovery confirm and
    // beacon notification confirm
    ZDO_RegisterForZdoCB( ZDO_NWK_DISCOVERY_CNF_CBID, zll_NwkDiscoveryCnfCB );
    ZDO_RegisterForZdoCB( ZDO_BEACON_NOTIFY_IND_CBID, zll_BeaconIndCB );
  }
  else
  {
    NLME_NwkDiscTerm();
  }
}


/*********************************************************************
 * @fn      zll_BeaconIndCB
 *
 * @brief   Process the incoming beacon indication.
 *
 * @param   param -  pointer to a parameter and a structure of parameters
 *
 * @return  void
 */
static void *zll_BeaconIndCB ( void *param )
{
  NLME_beaconInd_t *pBeacon = param;
  zllDiscoveredNwkParam_t *pParam = pDiscoveredNwkParamList;
  zllDiscoveredNwkParam_t *pLastParam;
  uint8 found = FALSE;

  if ( pBeacon->permitJoining == TRUE )
  {
    // Add the network parameter to the Network Parameter List
    while ( pParam != NULL )
    {
      if ( ( pParam->PANID == pBeacon->panID ) &&
          ( pParam->logicalChannel == pBeacon->logicalChannel ) )
      {
        found = TRUE;
        break;
      }

      pLastParam = pParam;
      pParam = pParam->nextParam;
    }

    // If no existing parameter found, make a new one and add to the list
    if ( found == FALSE )
    {
      pParam = osal_mem_alloc( sizeof( zllDiscoveredNwkParam_t ) );
      if ( pParam == NULL )
      {
        // Memory alloc failed, discard this beacon
        return ( NULL );
      }

      // Clear the network descriptor
      osal_memset( pParam, 0, sizeof( zllDiscoveredNwkParam_t )  );

      // Initialize the descriptor
      pParam->chosenRouter = INVALID_NODE_ADDR;
      pParam->chosenRouterDepth = 0xFF;

      // Save new entry into the descriptor list
      if ( pDiscoveredNwkParamList == NULL )
      {
        // First element in the list
        pDiscoveredNwkParamList = pParam;
      }
      else
      {
        // Last element in the list
        pLastParam->nextParam = pParam;
      }
    }

    // Update the descriptor with the incoming beacon
    pParam->logicalChannel = pBeacon->logicalChannel;
    pParam->PANID          = pBeacon->panID;

    // Save the extended PAN ID from the beacon payload only if 1.1 version network
    if ( pBeacon->protocolVersion != ZB_PROT_V1_0 )
    {
      osal_cpyExtAddr( pParam->extendedPANID, pBeacon->extendedPanID );
    }
    else
    {
      osal_memset( pParam->extendedPANID, 0xFF, Z_EXTADDR_LEN );
    }

    // check if this device is a better choice to join...
    // ...dont bother checking assocPermit flag is doing a rejoin
    if ( ( pBeacon->LQI > gMIN_TREE_LINK_COST ) &&
        (  pBeacon->permitJoining == TRUE   ) )
    {
      uint8 selected = FALSE;
      uint8 capacity = FALSE;

      if ( _NIB.nwkAddrAlloc == NWK_ADDRESSING_STOCHASTIC )
      {
        if ( ((pBeacon->LQI   > pParam->chosenRouterLinkQuality) &&
              (pBeacon->depth < MAX_NODE_DEPTH)) ||
            ((pBeacon->LQI   == pParam->chosenRouterLinkQuality) &&
             (pBeacon->depth < pParam->chosenRouterDepth)) )
        {
          selected = TRUE;
        }
      }
      else
      {
        if ( pBeacon->depth < pParam->chosenRouterDepth )
        {
          selected = TRUE;
        }
      }

      capacity = pBeacon->routerCapacity;

      if ( (capacity) && (selected) )
      {
        // this is the new chosen router for joining...
        pParam->chosenRouter            = pBeacon->sourceAddr;
        pParam->chosenRouterLinkQuality = pBeacon->LQI;
        pParam->chosenRouterDepth       = pBeacon->depth;
      }

      if ( pBeacon->deviceCapacity )
        pParam->deviceCapacity = 1;

      if ( pBeacon->routerCapacity )
        pParam->routerCapacity = 1;
    }
  }
  return ( NULL );
}

/*********************************************************************
 * @fn      zll_NwkDiscoveryCnfCB
 *
 * @brief   Send an event to inform the target the completion of
 *          network discovery scan
 *
 * @param   param - pointer to a parameter and a structure of parameters
 *
 * @return  void
 */
static void *zll_NwkDiscoveryCnfCB ( void *param )
{
  // Scan completed. De-register the callbacks with ZDO
  ZDO_DeregisterForZdoCB( ZDO_NWK_DISCOVERY_CNF_CBID );
  ZDO_DeregisterForZdoCB( ZDO_BEACON_NOTIFY_IND_CBID );

  NLME_NwkDiscTerm();

  if ( pDiscoveredNwkParamList != NULL )
  {
    // proceed to join the network, otherwise
    // Notify our task
    osal_set_event( zllTaskId, ZLL_NWK_DISC_CNF_EVT );
  }
  else
  {
    if ( zllHAScanInitiated == ZLL_SCAN_PRIMARY_CHANNELS )
    {
      // try discovery in secondary ZLL channel list
      zllHAScanInitiated = ZLL_SCAN_SECONDARY_CHANNELS;

      zll_ClassicalCommissioningNetworkDisc();
    }
    else
    {
      if ( zllHAScanInitiated != FALSE )
      {
        zllHAScanInitiated = ZLL_SCAN_FOUND_NOTHING;
      }
      zllJoinedHANetwork = FALSE;

      // no suitable network in secondary channel list, then just wait for touchlink
      // Notify our task
      osal_set_event( zllTaskId, ZLL_NWK_DISC_CNF_EVT );
    }
  }

  return ( NULL );
}

/****************************************************************************
 * @fn      zll_FreeNwkParamList
 *
 * @brief   This function frees any network discovery data.
 *
 * @param   none
 *
 * @return  none
 */
void zll_FreeNwkParamList( void )
{
  zllDiscoveredNwkParam_t *pParam = pDiscoveredNwkParamList;
  zllDiscoveredNwkParam_t *pNextParam;

  // deallocate the pDiscoveredNwkParamList memory
  while ( pParam != NULL )
  {
    pNextParam = pParam->nextParam;

    osal_mem_free( pParam );

    pParam = pNextParam;
  }

  pDiscoveredNwkParamList = NULL;
}

/****************************************************************************
 * @fn      zll_SetTCLK
 *
 * @brief   This function sets the default ZLL link key in the TC Link Key table,
 *          in the last entry.
 *
 * @param   none
 *
 * @return  none
 */
static void zll_SetTCLK()
{
  uint8 defaultZllTCLinkKey[SEC_KEY_LEN] = ZLL_LINK_KEY;
  APSME_TCLinkKey_t zllTCLKentry = {0};
  osal_memset( zllTCLKentry.extAddr, 0xFF, Z_EXTADDR_LEN );
  osal_memcpy( zllTCLKentry.key, defaultZllTCLinkKey, SEC_KEY_LEN);
#if ( ZDSECMGR_TC_DEVICE_MAX > 1 )
  osal_nv_write( ZCD_NV_TCLK_TABLE_START + (ZDSECMGR_TC_DEVICE_MAX-1), 0,
                 sizeof(zllTCLKentry), &zllTCLKentry);
#else
#error ZDSECMGR_TC_DEVICE_MAX should be defined to be 2 or more
#endif
}

/****************************************************************************
 * @fn      zll_IsValidTransID
 *
 * @brief   Transaction ID Filter for Touch-Link received commands.
 *
 * @param   transID - received transaction ID
 *
 * @return  FALSE if not matching current or transaction expired
 */
bool zll_IsValidTransID( uint32 transID )
{
  if ( ( zllTransID == 0 ) || ( ( zllTransID != transID ) && ( zllLastAcceptedTransID != transID ) ) )
  {
    return ( FALSE );
  }
  return ( TRUE );
}

/*********************************************************************
 * @fn      zll_RouterProcessZDOMsg
 *
 * @brief   Process incoming ZDO messages (for routers)
 *
 * @param   inMsg - message to process
 *
 * @return  none
 */
void zll_RouterProcessZDOMsg( zdoIncomingMsg_t *inMsg )
{
  ZDO_DeviceAnnce_t devAnnce;

  switch ( inMsg->clusterID )
  {
    case Device_annce:
      {
        // all devices should send link status, including the one sending it
        ZDO_ParseDeviceAnnce( inMsg, &devAnnce );

        linkInfo_t *linkInfo;

        // check if entry exists
        linkInfo = nwkNeighborGetLinkInfo( devAnnce.nwkAddr, _NIB.nwkPanId );

        // if not, look for a vacant entry to add this node...
        if ( linkInfo == NULL )
        {
          nwkNeighborAdd( devAnnce.nwkAddr, _NIB.nwkPanId, 1 );
          // if we have end device childs, send link status
          if ( AssocCount(CHILD_RFD, CHILD_RFD_RX_IDLE) > 0 )
          {
            linkInfo = nwkNeighborGetLinkInfo( devAnnce.nwkAddr, _NIB.nwkPanId );
            if ( (linkInfo != NULL) && (linkInfo->txCost == 0) )
            {
              linkInfo->txCost = MAX_LINK_COST;
            }
            NLME_UpdateLinkStatus();
          }
        }
        else
        {
          // only update the TxCost, so the Link Status can be sent properly
          if (linkInfo->txCost == 0)
          {
            linkInfo->txCost = MAX_LINK_COST;
          }
        }
      }
      break;

    case Mgmt_Permit_Join_req:
      {
        uint8 duration = inMsg->asdu[ZDP_MGMT_PERMIT_JOIN_REQ_DURATION];
        ZStatus_t stat = NLME_PermitJoiningRequest( duration );
        // Send a response if unicast
        if ( !inMsg->wasBroadcast )
        {
          ZDP_MgmtPermitJoinRsp( inMsg->TransSeq, &(inMsg->srcAddr), stat, false );
        }
      }
      break;

    default:
      break;
  }
}

/*********************************************************************
 * @fn      zll_PermitJoin
 *
 * @brief   Set the router permit join flag, to allow or deny classical
 *          commissioning by other ZigBee devices.
 *
 * @param   duration - enable up to aplcMaxPermitJoinDuration seconds,
 *                     0 to disable
 *
 * @return  status
 */
ZStatus_t zll_PermitJoin( uint8 duration )
{
  if ( duration > ZLL_APLC_MAX_PERMIT_JOIN_DURATION )
  {
    duration = ZLL_APLC_MAX_PERMIT_JOIN_DURATION;
  }
  return NLME_PermitJoiningRequest( duration );
}


/*********************************************************************
*********************************************************************/

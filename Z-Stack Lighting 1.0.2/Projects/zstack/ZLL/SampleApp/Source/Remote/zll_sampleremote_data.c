/**************************************************************************************************
  Filename:       zll_sampleremote_data.c
  Revised:        $Date: 2013-12-18 19:18:39 -0800 (Wed, 18 Dec 2013) $
  Revision:       $Revision: 36695 $


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
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_LL.h"

#include "zll_sampleremote.h"

/*********************************************************************
 * CONSTANTS
 */

#define SAMPLEREMOTE_DEVICE_VERSION    1
#define SAMPLEREMOTE_FLAGS             0

#define SAMPLEREMOTE_HWVERSION         1
#define SAMPLEREMOTE_ZCLVERSION        1

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Basic Cluster
const uint8 zllSampleRemote_HWVersion = SAMPLEREMOTE_HWVERSION;
const uint8 zllSampleRemote_ZCLVersion = SAMPLEREMOTE_ZCLVERSION;
const uint8 zllSampleRemote_ManufacturerName[] = { 16, 'T','e','x','a','s','I','n','s','t','r','u','m','e','n','t','s' };
const uint8 zllSampleRemote_ModelId[] = { 15, 'T','I',' ','S','a','m','p','l','e','R','e','m','o','t','e' };
const uint8 zllSampleRemote_DateCode[] = { 8, '2','0','1','3','1','2','0','6' };
const uint8 zllSampleRemote_PowerSource = POWER_SOURCE_BATTERY;
const uint8 zllSampleRemote_SWBuildID[] = { 5, '1','.','0','.','2' };
const uint8 zllSampleRemote_AppVersion = 1;
const uint8 zllSampleRemote_StackVersion = 2;

// Identify Cluster
uint16 zllSampleRemote_IdentifyTime = 0;

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t zllSampleRemote_Attrs[SAMPLEREMOTE_MAX_ATTRIBUTES] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_BASIC_HW_VERSION,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)&zllSampleRemote_HWVersion  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zllSampleRemote_ZCLVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_APPL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zllSampleRemote_AppVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_STACK_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zllSampleRemote_StackVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_SW_BUILD_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zllSampleRemote_SWBuildID
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zllSampleRemote_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zllSampleRemote_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zllSampleRemote_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ,
      (void *)&zllSampleRemote_PowerSource
    }
  },

  // *** Identify Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // Attribute record
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zllSampleRemote_IdentifyTime
    }
  },
};

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
const cId_t zllSampleRemote_InClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_LIGHT_LINK
};

const cId_t zllSampleRemote_OutClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_GROUPS,
  ZCL_CLUSTER_ID_GEN_SCENES,
  ZCL_CLUSTER_ID_GEN_ON_OFF,
#ifdef ZCL_LEVEL_CTRL
  ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL,
#endif
#ifdef ZCL_COLOR_CTRL
  ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
#endif
  ZCL_CLUSTER_ID_LIGHT_LINK
};

SimpleDescriptionFormat_t zllSampleRemote_SimpleDesc =
{
  SAMPLEREMOTE_ENDPOINT,               //  int Endpoint;
  ZCL_HA_PROFILE_ID,                   //  uint16 AppProfId[2];
  HA_DEVICEID,                         //  uint16 AppDeviceId[2];
  ZLL_DEVICE_VERSION,                  //  int   AppDevVer:4;
  SAMPLEREMOTE_FLAGS,                  //  int   AppFlags:4;
  ( sizeof(zllSampleRemote_InClusterList) / sizeof(cId_t) ),  //  byte  AppNumInClusters;
  (cId_t *)zllSampleRemote_InClusterList,                     //  byte *pAppInClusterList;
  ( sizeof(zllSampleRemote_OutClusterList) / sizeof(cId_t) ), //  byte  AppNumInClusters;
  (cId_t *)zllSampleRemote_OutClusterList                     //  byte *pAppInClusterList;
};

zclLLDeviceInfo_t zllSampleRemote_DeviceInfo =
{
  SAMPLEREMOTE_ENDPOINT,  //uint8 endpoint;
  ZLL_PROFILE_ID,         //uint16 profileID;
  ZLL_DEVICEID,           //uint16 deviceID;
  ZLL_DEVICE_VERSION,     //uint8 version;
  SAMPLEREMOTE_NUM_GRPS   //uint8 grpIdCnt;
};

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/****************************************************************************
****************************************************************************/



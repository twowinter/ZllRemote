/**************************************************************************************************
  Filename:       zll_samplelight_data.c
  Revised:        $Date: 2013-12-12 08:31:21 -0800 (Thu, 12 Dec 2013) $
  Revision:       $Revision: 36570 $


  Description:    Zigbee Cluster Library - Light Link (ZLL) Light Sample
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
#include "zcl_ll.h"

#include "zll_samplelight.h"

/*********************************************************************
 * CONSTANTS
 */

#define SAMPLELIGHT_DEVICE_VERSION     1
#define SAMPLELIGHT_FLAGS              0

#define SAMPLELIGHT_ZCL_VERSION        1
#define SAMPLELIGHT_HW_VERSION         1

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
const uint8 zllSampleLight_ZCLVersion = SAMPLELIGHT_ZCL_VERSION;
const uint8 zllSampleLight_HWVersion = SAMPLELIGHT_HW_VERSION;
const uint8 zllSampleLight_ManufacturerName[] = { 16, 'T','e','x','a','s','I','n','s','t','r','u','m','e','n','t','s' };
const uint8 zllSampleLight_ModelId[] = { 14, 'T','I',' ','S','a','m','p','l','e','L','i','g','h','t' };
const uint8 zllSampleLight_DateCode[] = { 8, '2','0','1','3','1','2','0','6' };
const uint8 zllSampleLight_PowerSource = POWER_SOURCE_MAINS_1_PHASE;
const uint8 zllSampleLight_SWBuildID[] = { 5, '1','.','0','.','2' };
const uint8 zllSampleLight_AppVersion = 1;
const uint8 zllSampleLight_StackVersion = 2;

// Identify Cluster
uint16 zllSampleLight_IdentifyTime = 0;

// On/Off Cluster
uint8  zllSampleLight_OnOff = LIGHT_OFF;
uint8  zllSampleLight_GlobalSceneCtrl = TRUE;
uint16 zllSampleLight_OnTime = 0x0000;
uint16 zllSampleLight_OffWaitTime = 0x0000;
zclGeneral_Scene_t zllSampleLight_GlobalScene =
{
  0,                                     // The group ID for which this scene applies
  0,                                     // Scene ID
  0,                                     // Time to take to transition to this scene
  0,                                     // Together with transTime, this allows transition time to be specified in 1/10s
  "GlobalScene",                         // Scene name
  ZCL_GEN_SCENE_EXT_LEN,                 // Length of extension fields
  0,                                     // Extension fields
};

// Level control Cluster (server) -----------------------------------------------------
#ifdef ZCL_LEVEL_CTRL
uint8 zclLevel_CurrentLevel = 0xFE;
uint16 zclLevel_LevelRemainingTime = 0;
#endif //ZCL_LEVEL_CTRL

// Scene Cluster (server) -----------------------------------------------------
uint8 zllSampleLight_CurrentScene = 0x0;
uint16 zllSampleLight_CurrentGroup = 0x0;
uint8 zllSampleLight_SceneValid = 0x0;
const uint8 zllSampleLight_SceneNameSupport = 0;

// Group Cluster (server) -----------------------------------------------------
const uint8 zllSampleLight_GroupNameSupport = 0;

// Color control Cluster (server) -----------------------------------------------------
#ifdef ZCL_COLOR_CTRL
uint16 zclColor_CurrentX = 0x616b;
uint16 zclColor_CurrentY = 0x607d;
uint16 zclColor_EnhancedCurrentHue = 0;
uint8  zclColor_CurrentHue = 0;
uint8  zclColor_CurrentSaturation = 0x0;

uint8  zclColor_ColorMode = COLOR_MODE_CURRENT_X_Y;
uint8  zclColor_EnhancedColorMode = ENHANCED_COLOR_MODE_CURRENT_HUE_SATURATION;
uint16 zclColor_ColorRemainingTime = 0;
uint8  zclColor_ColorLoopActive = 0;
uint8  zclColor_ColorLoopDirection = 0;
uint16 zclColor_ColorLoopTime = 0x0019;
uint16 zclColor_ColorLoopStartEnhancedHue = 0x2300;
uint16 zclColor_ColorLoopStoredEnhancedHue = 0;
uint16 zclColor_ColorCapabilities = ( COLOR_CAPABILITIES_ATTR_BIT_HUE_SATURATION |
                                      COLOR_CAPABILITIES_ATTR_BIT_ENHANCED_HUE |
                                      COLOR_CAPABILITIES_ATTR_BIT_COLOR_LOOP |
                                      COLOR_CAPABILITIES_ATTR_BIT_X_Y_ATTRIBUTES );
#ifdef ZLL_HW_LED_LAMP
const uint8  zclColor_NumOfPrimaries = 3;
//RED: LR W5AP, 625nm
const uint16 zclColor_Primary1X = 0xB35B;
const uint16 zclColor_Primary1Y = 0x4C9F;
const uint8 zclColor_Primary1Intensity = 0x9F;
//GREEN: LT W5AP, 528nm
const uint16 zclColor_Primary2X = 0x2382;
const uint16 zclColor_Primary2Y = 0xD095;
const uint8 zclColor_Primary2Intensity = 0xF0;
//BLUE: LD W5AP, 455nm
const uint16 zclColor_Primary3X = 0x26A7;
const uint16 zclColor_Primary3Y = 0x05D2;
const uint8 zclColor_Primary3Intensity = 0xFE;
#else
const uint8  zclColor_NumOfPrimaries = 0;
#endif //ZLL_HW_LED_LAMP
#endif //ZCL_COLOR_CTRL

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t zllSampleLight_Attrs[SAMPLELIGHT_NUM_ATTRIBUTES] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zllSampleLight_ZCLVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_APPL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zllSampleLight_AppVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_STACK_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zllSampleLight_StackVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_SW_BUILD_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zllSampleLight_SWBuildID
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_BASIC_HW_VERSION,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)&zllSampleLight_HWVersion   // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zllSampleLight_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zllSampleLight_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zllSampleLight_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ,
      (void *)&zllSampleLight_PowerSource
    }
  },

  // *** Identify Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // Attribute record
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      NULL // Use application's callback to Read/Write this attribute
    }
  },
  // *** Scene Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_SCENES,
    { // Attribute record
      ATTRID_SCENES_COUNT,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ),
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_SCENES,
    { // Attribute record
      ATTRID_SCENES_CURRENT_SCENE,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ),
      (void *)&zllSampleLight_CurrentScene
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_SCENES,
    { // Attribute record
      ATTRID_SCENES_CURRENT_GROUP,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ),
      (void *)&zllSampleLight_CurrentGroup
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_SCENES,
    { // Attribute record
      ATTRID_SCENES_SCENE_VALID,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ),
      (void *)&zllSampleLight_SceneValid
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_SCENES,
    { // Attribute record
      ATTRID_SCENES_NAME_SUPPORT,
      ZCL_DATATYPE_BITMAP8,
      (ACCESS_CONTROL_READ),
      (void *)&zllSampleLight_SceneNameSupport
    }
  },
  // *** Groups Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_GROUPS,
    { // Attribute record
      ATTRID_GROUP_NAME_SUPPORT,
      ZCL_DATATYPE_BITMAP8,
      (ACCESS_CONTROL_READ),
      (void *)&zllSampleLight_GroupNameSupport
    }
  },

  // *** On/Off Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF,
      ZCL_DATATYPE_BOOLEAN,
      ACCESS_CONTROL_READ,
      (void *)&zllSampleLight_OnOff
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF_GLOBAL_SCENE_CTRL,
      ZCL_DATATYPE_BOOLEAN,
      ACCESS_CONTROL_READ,
      (void *)&zllSampleLight_GlobalSceneCtrl
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF_ON_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zllSampleLight_OnTime
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF_OFF_WAIT_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zllSampleLight_OffWaitTime
    }
  },
  // *** Level Control Cluster Attribute ***
#ifdef ZCL_LEVEL_CTRL
  {
    ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL,
    { // Attribute record
      ATTRID_LEVEL_CURRENT_LEVEL,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclLevel_CurrentLevel
    }
  },

  {
    ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL,
    { // Attribute record
      ATTRID_LEVEL_REMAINING_TIME,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclLevel_LevelRemainingTime
    }
  },
#endif //ZCL_LEVEL_CTRL
  // *** Color Control Cluster Attributes ***
#ifdef ZCL_COLOR_CTRL
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_CurrentSaturation
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_CurrentHue
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_REMAINING_TIME,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorRemainingTime
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_X,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ),
      (void *)&zclColor_CurrentX
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_Y,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ),
      (void *)&zclColor_CurrentY
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_MODE,
      ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ),
      (void *)&zclColor_ColorMode
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_NUM_PRIMARIES,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ),
      (void *)&zclColor_NumOfPrimaries
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_ENHANCED_COLOR_MODE,
      ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ),
      (void *)&zclColor_EnhancedColorMode
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_ENHANCED_CURRENT_HUE,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_EnhancedCurrentHue
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_LOOP_ACTIVE,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorLoopActive
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_LOOP_DIRECTION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorLoopDirection
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_LOOP_TIME,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorLoopTime
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_LOOP_START_ENHANCED_HUE,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorLoopStartEnhancedHue
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_LOOP_STORED_ENHANCED_HUE,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorLoopStoredEnhancedHue
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_CAPABILITIES,
      ZCL_DATATYPE_BITMAP16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorCapabilities
    }
  },
#ifdef ZLL_HW_LED_LAMP
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_1_X,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary1X
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_1_Y,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary1Y
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_1_INTENSITY,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary1Intensity
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_2_X,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary2X
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_2_Y,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary2Y
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_2_INTENSITY,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary2Intensity
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_3_X,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary3X
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_3_Y,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary3Y
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_3_INTENSITY,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary3Intensity
    }
  },
#endif //ZLL_HW_LED_LAMP
#endif //ZCL_COLOR_CTRL
};

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.

const cId_t zllSampleLight_InClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
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
};

SimpleDescriptionFormat_t zllSampleLight_SimpleDesc =
{
  SAMPLELIGHT_ENDPOINT,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                     //  uint16 AppProfId[2];
  HA_DEVICEID,                           //  uint16 AppDeviceId[2];
  ZLL_DEVICE_VERSION,                    //  int   AppDevVer:4;
  SAMPLELIGHT_FLAGS,                     //  int   AppFlags:4;
  ( sizeof(zllSampleLight_InClusterList) / sizeof(cId_t) ),  //  byte  AppNumInClusters;
  (cId_t *)zllSampleLight_InClusterList,                     //  byte *pAppInClusterList;
  0,                                     //  byte  AppNumOutClusters;
  NULL                                   //  byte *pAppOutClusterList;
};

#ifdef ZLL_1_0_HUB_COMPATIBILITY
SimpleDescriptionFormat_t zllSampleLight_SimpleDesc2 =
{
  SAMPLELIGHT_ENDPOINT2, //  int Endpoint;
  ZCL_HA_PROFILE_ID,     //  uint16 AppProfId[2];
  ZLL_DEVICEID,          //  uint16 AppDeviceId[2];
  ZLL_DEVICE_VERSION,    //  int   AppDevVer:4;
  SAMPLELIGHT_FLAGS,     //  int   AppFlags:4;
  0,                     //  0 AppNumInClusters to evade Match Descriptor Req
  NULL,                  //  byte *pAppInClusterList;
  0,                     //  0 AppNumOutClusters to evade Match Descriptor Req
  NULL                   //  byte *pAppOutClusterList;
};
#endif

zclLLDeviceInfo_t zllSampleLight_DeviceInfo =
{
  SAMPLELIGHT_ENDPOINT,  //uint8 endpoint;
  ZLL_PROFILE_ID,        //uint16 profileID;
  ZLL_DEVICEID,          //uint16 deviceID;
  ZLL_DEVICE_VERSION,    //uint8 version;
  SAMPLELIGHT_NUM_GRPS   //uint8 grpIdCnt;
};

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/****************************************************************************
****************************************************************************/



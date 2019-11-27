/***************************************************************************//**
* \file ble_custom_hi.h
* \version 1.0
*
* \brief
* Header file for BLE custom host interface service.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/
#ifndef _BLE_CUSTOM_HI_H_
#define _BLE_CUSTOM_HI_H_

#include "ble_common.h"

/* The C binding of definitions if building with the C++ compiler */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************
* Macro definitions
***************************************/
/* Indexes of a two-byte CCCD array */
#define CCCD_INDEX_0            (uint8_t) (0x00u)
#define CCCD_INDEX_1            (uint8_t) (0x01u)

/* Bit mask for the notification bit in CCCD (Client Characteristic Configuration 
   Descriptor), which is written by the client device */
#define CCCD_NOTIFY_ENABLED     (uint8_t) (0x01u)
#define CCCD_NOTIFY_DISABLED    (uint8_t) (0x00u)

/* Redefinition of long CHAR and CCCD handles and indexes for better readability */
#define CUSTOM_CMD_CHAR_HANDLE (CY_BLE_CUSTOM_HOST_INTERFACE_COMMAND_CHAR_HANDLE)
#define CUSTOM_RES_CHAR_HANDLE (CY_BLE_CUSTOM_HOST_INTERFACE_RESPONSE_CHAR_HANDLE)
#define CUSTOM_RES_CCCD_HANDLE (CY_BLE_CUSTOM_HOST_INTERFACE_RESPONSE_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE)

/* The callback function prototype to handle custom command */
typedef void (* ble_custom_write_callback_t)(uint32_t len, void *cmd);

/**
 * @brief BLE custom command buffer size.
 */
#define BLE_CUSTOM_CMD_BUFFER_SIZE      (CY_BLE_GATT_DB_MAX_VALUE_LEN)

/**
 * @brief BLE custom response buffer size.
 */
#define BLE_CUSTOM_RES_BUFFER_SIZE      (CY_BLE_GATT_DB_MAX_VALUE_LEN)


/***************************************
* Data Types
***************************************/
/* BLE custom host interface Configuration structure */
typedef struct
{
    ble_custom_write_callback_t cmd_callback_func;
} ble_custom_hi_config_t;

/**
 * @brief custom command buffer structure for ble command received.
 */
typedef struct
{
    volatile uint8_t receive_flag;
    uint8_t reserve[3];
    uint16_t len;
    uint8_t  buf[BLE_CUSTOM_CMD_BUFFER_SIZE];
} custom_command_buf_t;


/***************************************
* Function Prototypes
***************************************/
cy_en_ble_api_result_t ble_custom_hi_init(ble_custom_hi_config_t *config);
void ble_custom_hi_service_evt_callback(uint32_t event, void* eventParam);
cy_en_ble_api_result_t ble_custom_hi_response_fast(uint16_t len, void *res);
cy_en_ble_api_result_t ble_custom_hi_response(uint16_t len, void *res);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BLE_CUSTOM_HI_H_ */

/* [] END OF FILE */

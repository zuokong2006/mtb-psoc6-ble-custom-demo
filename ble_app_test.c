/***************************************************************************//**
* \file ipc_app_test.c
* \version 1.0
*
* \brief
* Source file for BLE application testing.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "ble_app.h"

/* BLE custom service command received buffer */
static custom_command_buf_t ble_custom_cmd_buf;

/*******************************************************************************
* Function Name: ble_custom_command_callback
****************************************************************************//**
*
* \brief The callback function handler for BLE custom service command receivced.
*
* \param len  Received len
*
* \param cmd  point to the data buffer
*
* \return None
*
*******************************************************************************/
static void ble_custom_command_callback(uint32_t len, void *cmd)
{
    if(true == ble_custom_cmd_buf.receive_flag) {
        return;
    }
    ble_custom_cmd_buf.len = len;
    memcpy(ble_custom_cmd_buf.buf, (uint8_t *)cmd, len);
    ble_custom_cmd_buf.receive_flag = true;
}

/*******************************************************************************
* Function Name: ble_app_test
****************************************************************************//**
*
* BLE application test.
*
* \param none.
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
cy_en_ble_api_result_t ble_app_test(void)
{
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;
    ble_custom_hi_config_t custom_hi_config;
    
    ble_custom_cmd_buf.receive_flag = false;
    /* Initializes the custom host interface */
    custom_hi_config.cmd_callback_func = ble_custom_command_callback;
    ble_custom_hi_init(&custom_hi_config);
    /* Initializes the BLE application */
    if(CY_BLE_SUCCESS != (apiResult = ble_app_init())) {
        return apiResult;
    }
    
    for(;;)
    {
        /* Echo the received data to host */
        if(ble_custom_cmd_buf.receive_flag) {
            ble_custom_cmd_buf.receive_flag = false;
            ble_custom_hi_response_fast(ble_custom_cmd_buf.len, ble_custom_cmd_buf.buf);
        }
        /* BLE application task. */
        ble_app_task();
    }
}

/* [] END OF FILE */

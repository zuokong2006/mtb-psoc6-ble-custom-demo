/***************************************************************************//**
* \file ble_app.h
* \version 1.0
*
* \brief
* Header file for BLE Application.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/
#ifndef _BLE_APP_H_
#define _BLE_APP_H_

#include "ble_common.h"
#include "ble_custom_hi.h"

/* The C binding of definitions if building with the C++ compiler */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************
* Macro definitions
***************************************/

/***************************************
* Data Types
***************************************/

/***************************************
* Public Function Prototypes
***************************************/
cy_en_ble_api_result_t ble_app_init(void);
cy_en_ble_api_result_t ble_app_task(void);
cy_en_ble_api_result_t ble_app_stop(void);
cy_en_ble_api_result_t ble_app_connection_param_update_request(uint16_t interval_min, \
                        uint16_t interval_max, uint16_t slave_latency, uint16_t timeout_multiplier);
uint16_t ble_app_negotiate_mtu(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BLE_APP_H_ */

/* [] END OF FILE */

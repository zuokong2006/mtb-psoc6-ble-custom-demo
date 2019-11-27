/***************************************************************************//**
* \file ble_bond.h
* \version 1.0
*
* \brief
* Header file for BLE bond list helper APIs.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/
#ifndef _BLE_BOND_H_
#define _BLE_BOND_H_

#include "ble_common.h"

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
cy_en_ble_api_result_t ble_display_bond_list(void);
cy_en_ble_api_result_t ble_remove_devices_from_bond_list(void);
uint32_t ble_get_count_of_bonded_devices(void);
bool ble_is_device_in_bond_list(uint32_t bdHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BLE_BOND_H_ */

/* [] END OF FILE */

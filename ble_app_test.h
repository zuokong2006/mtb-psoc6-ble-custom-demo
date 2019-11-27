/***************************************************************************//**
* \file ble_app_test.h
* \version 1.0
*
* \brief
* Header file for ble application testing.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/
#ifndef _BLE_APP_TEST_H_
#define _BLE_APP_TEST_H_

#include "ble_app.h"

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
cy_en_ble_api_result_t ble_app_test(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BLE_APP_TEST_H_ */

/* [] END OF FILE */

/***************************************************************************//**
* \file ble_common.h
*
* \brief
* Header file for BLE common interfaces.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/
#ifndef _BLE_COMMON_H_
#define _BLE_COMMON_H_

#include <stdio.h>
#include "ble_cfg.h"
#include "cyhal.h"
#include "cy_retarget_io.h"
#include "cybsp.h"
#include "cycfg_ble.h"

/* The C binding of definitions if building with the C++ compiler */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************
* Macro definitions
***************************************/
/**
 * @brief Enable or disable the BLE debug function.
 */
#ifndef BLE_DEBUG_UART_ENABLED
#define BLE_DEBUG_UART_ENABLED                          DISABLED
#endif

/**
 * @brief Define the timeout value of BLE timer: Сounts in seconds.
 */
#define BLE_TIMER_TIMEOUT                               (1u)

/**
 * @brief Button the time of long press for bond list.
 */
#define SW2_PRESS_TIME_DEL_BOND_LIST                    (0x04u)

#define CY_BLE_CONN_INTRV_TO_MS                         (5.0f / 4.0f)

/***************************************
*   UART_DEB Macros / prototypes
***************************************/
#if (BLE_DEBUG_UART_ENABLED == ENABLED)
    #define BLE_DBG_PRINTF(...)                 (printf(__VA_ARGS__))
    #define BLE_UART_DEB_PUT_CHAR(...)          while(1UL != Cy_SCB_UART_Put(cy_retarget_io_uart_obj.base, __VA_ARGS__))
    #define BLE_UART_DEB_GET_CHAR(...)          (Cy_SCB_UART_Get(cy_retarget_io_uart_obj.base))
    #define BLE_UART_DEB_IS_TX_COMPLETE(...)    (Cy_SCB_UART_IsTxComplete(cy_retarget_io_uart_obj.base))
    #define BLE_UART_DEB_WAIT_TX_COMPLETE(...)  while(Cy_SCB_UART_IsTxComplete(cy_retarget_io_uart_obj.base) == 0);
    #define BLE_UART_DEB_SCB_CLEAR_RX_FIFO(...) (Cy_SCB_UART_ClearRxFifo(cy_retarget_io_uart_obj.base))
    #define BLE_UART_START(...)                 
#else
    #define BLE_DBG_PRINTF(...)                 
    #define BLE_UART_DEB_PUT_CHAR(...)
    #define BLE_UART_DEB_GET_CHAR(...)          (0u)
    #define BLE_UART_DEB_IS_TX_COMPLETE(...)    (1u)
    #define BLE_UART_DEB_WAIT_TX_COMPLETE(...)
    #define BLE_UART_DEB_SCB_CLEAR_RX_FIFO(...) (0u)
    #define BLE_UART_START(...)
#endif /* (BLE_DEBUG_UART_ENABLED == ENABLED) */

#define BLE_UART_DEB_NO_DATA                    (char8) CY_SCB_UART_RX_NO_DATA

/***************************************
* Public Function Prototypes
***************************************/
void ble_print_stack_version(void);
void ble_print_api_result(cy_en_ble_api_result_t apiResult);

/***************************************
* External data references
***************************************/
extern cy_stc_ble_conn_handle_t appConnHandle;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/* [] END OF FILE */

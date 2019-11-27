/***************************************************************************//**
* \file config.h
* \version 1.0
*
* \brief
* Header file for project configuartion.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/
#ifndef _BLE_CFG_H_
#define _BLE_CFG_H_

#include <stdio.h>

/***************************************
* Macro definitions
***************************************/
/**
 * @brief Enabled and disabled marco definition.
 */
#ifndef ENABLED
#define ENABLED                                         (1u)
#endif
#ifndef DISABLED
#define DISABLED                                        (0u)
#endif

/**
 * @brief Enable or disable the BLE uart debug function.
 */
#define BLE_DEBUG_UART_ENABLED                          ENABLED

/**
 * @brief Enable or Disable the system low power function
 */
#define ENABLE_SYS_LPM_FUNCTION                         DISABLED

/***************************************
* Data Types
***************************************/


#endif /* _BLE_CFG_H_ */

/* [] END OF FILE */

/***************************************************************************//**
* \file ble_custom_hi.c
* \version 1.0
*
* \brief
* Source file for BLE custom host interface service.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/
#include "ble_custom_hi.h"

/**
 * @brief Global Handle to internal BLE custom host interafce structure.
 */
static ble_custom_hi_config_t ble_custom_hi_config;

/* Global Handle to internal BLE structure which holds the BLE connected handle */
static cy_stc_ble_conn_handle_t m_psoc6_ble_conn_handle;


/*******************************************************************************
* Function Name: ble_custom_hi_init
****************************************************************************//**
*
* Initializes the custom host interface.
*
* \param config custom host interface configuration.
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
cy_en_ble_api_result_t ble_custom_hi_init(ble_custom_hi_config_t *config)
{
    if(config == NULL) {
        return CY_BLE_ERROR_INVALID_PARAMETER;
    }
    /* command write callback */
    if(NULL != config->cmd_callback_func) {
        ble_custom_hi_config.cmd_callback_func = config->cmd_callback_func;
    } else {
        ble_custom_hi_config.cmd_callback_func = NULL;
    }
    return CY_BLE_SUCCESS;
}

/*******************************************************************************
* Function Name: ble_custom_command_write_request
****************************************************************************//**
*
* The custom command write request handler.
*
* \param writeRequest The data received as part of the write request events.
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
static cy_en_ble_api_result_t ble_custom_command_write_request(cy_stc_ble_gatt_write_param_t *writeRequest)
{
    /* Check if the returned handle is matching to custom commad Write Attribute */
    if(CUSTOM_CMD_CHAR_HANDLE == writeRequest->handleValPair.attrHandle)
    {
        /* command callback */
        if((NULL != ble_custom_hi_config.cmd_callback_func) && (0 < writeRequest->handleValPair.value.len))
        {
            ble_custom_hi_config.cmd_callback_func(writeRequest->handleValPair.value.len, \
                                                   writeRequest->handleValPair.value.val);
            #if (BLE_DEBUG_UART_ENABLED == ENABLED)
            uint8_t n = 0;
            BLE_DBG_PRINTF("CMD: ");
            for(n=0; n<writeRequest->handleValPair.value.len; n++) {
                BLE_DBG_PRINTF("%02x ", (*((uint8_t *)(writeRequest->handleValPair.value.val)+n)));
            }
            BLE_DBG_PRINTF("\r\n");
            #endif
        }
    }
    return CY_BLE_SUCCESS;
}

/*******************************************************************************
* Function Name: ble_custom_hi_write_request_handler
****************************************************************************//**
*
* The command and response write request handler.
*
* \param writeRequest The data received as part of the write request events.
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
static cy_en_ble_api_result_t ble_custom_hi_write_request_handler(cy_stc_ble_gatt_write_param_t *writeRequest)
{
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;
    cy_en_ble_gatt_err_code_t gattErr = CY_BLE_GATT_ERR_NONE;
    bool need_send_rsp = true;
    
    if((writeRequest == NULL) || (m_psoc6_ble_conn_handle.bdHandle != writeRequest->connHandle.bdHandle)) {
        return CY_BLE_ERROR_INVALID_PARAMETER;
    }
    
    if(writeRequest->handleValPair.attrHandle == CUSTOM_RES_CCCD_HANDLE) {
        cy_stc_ble_gatts_db_attr_val_info_t dbAttrValInfo = {
            .handleValuePair = writeRequest->handleValPair,
            .connHandle      = writeRequest->connHandle,
            .offset          = 0u,
            .flags           = CY_BLE_GATT_DB_PEER_INITIATED
        };
        gattErr = Cy_BLE_GATTS_WriteAttributeValueCCCD(&dbAttrValInfo);
        if(gattErr != CY_BLE_GATT_ERR_NONE) {
            apiResult = CY_BLE_ERROR_INVALID_OPERATION;
        }
    } else if(writeRequest->handleValPair.attrHandle == CUSTOM_CMD_CHAR_HANDLE) {
        gattErr = Cy_BLE_GATTS_WriteAttributeValueLocal(&(writeRequest->handleValPair));
        if(gattErr != CY_BLE_GATT_ERR_NONE) {
            #if (BLE_DEBUG_UART_ENABLED == ENABLED)
            BLE_DBG_PRINTF("Cy_BLE_GATTS_WriteAttributeValueLocal return error\r\n");
            #endif
            apiResult = CY_BLE_ERROR_INVALID_OPERATION;
        }
    } else {
        need_send_rsp = false;
    }
    /* Send the response to the write request received. */
    if(need_send_rsp) {
        cy_stc_ble_gatt_err_param_t err_param = {
            .errInfo.opCode     = CY_BLE_GATT_WRITE_REQ,
            .errInfo.attrHandle = writeRequest->handleValPair.attrHandle,
            .connHandle         = writeRequest->connHandle
        };
        /* Send response when event was handled by the service */
        if(gattErr != CY_BLE_GATT_ERR_NONE) {
            #if (BLE_DEBUG_UART_ENABLED == ENABLED)
            BLE_DBG_PRINTF("Cy_BLE_GATTS_ErrorRsp\r\n");
            #endif
            /* Send an Error Response */
            err_param.errInfo.errorCode  = gattErr;
            (void)Cy_BLE_GATTS_ErrorRsp(&err_param);
        } else {
            #if (BLE_DEBUG_UART_ENABLED == ENABLED)
            BLE_DBG_PRINTF("Cy_BLE_GATTS_WriteRsp\r\n");
            #endif
            /* Send an Write Response */
            (void)Cy_BLE_GATTS_WriteRsp(writeRequest->connHandle);
        }
    }
    
    if(apiResult == CY_BLE_SUCCESS) {
        if(writeRequest->handleValPair.attrHandle == CUSTOM_CMD_CHAR_HANDLE) {
            apiResult = ble_custom_command_write_request(writeRequest);
        }
    }
    return apiResult;
}

/*******************************************************************************
* Function Name: ble_custom_hi_write_cmd_handler
****************************************************************************//**
*
* The custom write command handler.
*
* \param writeCmd The data received as part of the write cmd events.
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
static cy_en_ble_api_result_t ble_custom_hi_write_cmd_handler(cy_stc_ble_gatt_write_param_t *writeCmd)
{
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;

    if((writeCmd == NULL) || (m_psoc6_ble_conn_handle.bdHandle != writeCmd->connHandle.bdHandle))
    {
        return CY_BLE_ERROR_INVALID_PARAMETER;
    }
    if(writeCmd->handleValPair.attrHandle == CUSTOM_CMD_CHAR_HANDLE)
    {
        if(Cy_BLE_GATTS_WriteAttributeValueLocal(&(writeCmd->handleValPair)) != CY_BLE_GATT_ERR_NONE) {
            return CY_BLE_ERROR_INVALID_OPERATION;
        }
        apiResult = ble_custom_command_write_request(writeCmd);
    }
    return apiResult;
}

/*******************************************************************************
* Function Name: ble_custom_hi_service_evt_callback
****************************************************************************//**
*
* This is an event callback function for BLE custom host interface service.
*
* \param event the event code.
*
* \param eventParam the event parameters.
*
* \return none.
*
*******************************************************************************/
void ble_custom_hi_service_evt_callback(uint32_t event, void* eventParam)
{
    cy_stc_ble_gatt_write_param_t *gatt_write_param = NULL;
    cy_stc_ble_gatts_write_cmd_req_param_t *gatt_write_cmd = NULL;

    switch(event)
    {
    /**********************************************************
     *                       GATTS Events
     ***********************************************************/
    case CY_BLE_EVT_GATT_CONNECT_IND:
        m_psoc6_ble_conn_handle = *(cy_stc_ble_conn_handle_t *)eventParam;
        BLE_DBG_PRINTF("CY_BLE_EVT_GATT_CONNECT_IND: %x, %x \r\n", m_psoc6_ble_conn_handle.attId, m_psoc6_ble_conn_handle.bdHandle);
        break;
        
    case CY_BLE_EVT_GATTS_WRITE_REQ:
        gatt_write_param = (cy_stc_ble_gatt_write_param_t *)eventParam;
        if(CY_BLE_SUCCESS != ble_custom_hi_write_request_handler(gatt_write_param)) {
            BLE_DBG_PRINTF("ble_custom_hi_write_request_handler return error\r\n");
        }
        break;
    case CY_BLE_EVT_GATTS_WRITE_CMD_REQ:
        gatt_write_cmd = (cy_stc_ble_gatts_write_cmd_req_param_t *)eventParam;
        if(CY_BLE_SUCCESS != ble_custom_hi_write_cmd_handler(gatt_write_cmd)) {
            BLE_DBG_PRINTF("ble_custom_hi_write_cmd_handler return error\r\n");
        }
        break;
    /* Indication Response is received from the GATT Client */
    case CY_BLE_EVT_GATTS_HANDLE_VALUE_CNF:
        break;
    }
}

/*******************************************************************************
* Function Name: ble_custom_hi_response_fast
****************************************************************************//**
*
* This function updates the response data to host by notification.
*
* \param connHandle The ble connected handle.
*
* \param value Notification status. Valid values are CCCD_NOTIFY_DISABLED and CCCD_NOTIFY_ENABLED.
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
cy_en_ble_api_result_t ble_custom_hi_response_fast(uint16_t len, void *res)
{
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;
    
    if(Cy_BLE_GetConnectionState(m_psoc6_ble_conn_handle) != CY_BLE_CONN_STATE_CONNECTED) {
        apiResult = CY_BLE_ERROR_NO_CONNECTION;
    } else if(!CY_BLE_IS_NOTIFICATION_ENABLED(m_psoc6_ble_conn_handle.attId, CUSTOM_RES_CCCD_HANDLE)) {
        apiResult = CY_BLE_ERROR_NTF_DISABLED;
    } else {
        /* Wait for the stack is idle */
        while(Cy_BLE_GATT_GetBusyStatus(m_psoc6_ble_conn_handle.attId) == CY_BLE_STACK_STATE_BUSY) {
            Cy_BLE_ProcessEvents();
        }
        /* Make sure that stack is not busy, then send the notification. */
        if (Cy_BLE_GATT_GetBusyStatus(m_psoc6_ble_conn_handle.attId) == CY_BLE_STACK_STATE_FREE) {
            /* Get GATT MTU size */
            cy_stc_ble_gatt_xchg_mtu_param_t mtuParam = { .connHandle = m_psoc6_ble_conn_handle };
            if(CY_BLE_SUCCESS != (apiResult = Cy_BLE_GATT_GetMtuSize(&mtuParam))) {
                return apiResult;
            }
            if((mtuParam.mtu - CY_BLE_GATT_WRITE_HEADER_LEN) < len) {
                len = mtuParam.mtu - CY_BLE_GATT_WRITE_HEADER_LEN;
            }
            /* Send the updated value to the peer device using notification procedure */
            cy_stc_ble_gatt_handle_value_pair_t ntfReqParam = {
                .attrHandle = CUSTOM_RES_CHAR_HANDLE,
                .value.val  = res,
                .value.len  = len
            };
            apiResult = Cy_BLE_GATTS_SendNotification(&m_psoc6_ble_conn_handle, &ntfReqParam);
            if(apiResult != CY_BLE_SUCCESS) {
                BLE_DBG_PRINTF("Cy_BLE_GATTS_Notification API Error: 0x%x \r\n", apiResult);
            }
        } else {
            BLE_DBG_PRINTF("CY_BLE_STACK_STATE_BUSY\r\n");
            apiResult = CY_BLE_ERROR_INVALID_PARAMETER;
        }
    }
    /* Wait for the stack is idle */
    while(Cy_BLE_GATT_GetBusyStatus(m_psoc6_ble_conn_handle.attId) == CY_BLE_STACK_STATE_BUSY) {
        Cy_BLE_ProcessEvents();
    }
    return apiResult;
}

/******************************************************************************
* Function Name: ble_custom_hi_response
***************************************************************************//**
*
* This function updates the response data to host by indication.
*
*  \param len: The size of the characteristic value attribute.
*  \param res:The pointer to the characteristic value data that should be sent to the client's device.
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
******************************************************************************/
cy_en_ble_api_result_t ble_custom_hi_response(uint16_t len, void *res)
{
    cy_en_ble_api_result_t apiResult;

    if((len < 1) || (res == NULL)) {
        return CY_BLE_ERROR_INVALID_PARAMETER;
    }
    /* Send indication if it is enabled and connected */
    if(Cy_BLE_GetConnectionState(m_psoc6_ble_conn_handle) < CY_BLE_CONN_STATE_CONNECTED)
    {
        apiResult = CY_BLE_ERROR_INVALID_STATE;
    } else if(!CY_BLE_IS_INDICATION_ENABLED(m_psoc6_ble_conn_handle.attId, CUSTOM_RES_CCCD_HANDLE)){
        apiResult = CY_BLE_ERROR_IND_DISABLED;
    } else {
        /* Get GATT MTU size */
        cy_stc_ble_gatt_xchg_mtu_param_t mtuParam = { .connHandle = m_psoc6_ble_conn_handle };
        if(CY_BLE_SUCCESS != (apiResult = Cy_BLE_GATT_GetMtuSize(&mtuParam))) {
            return apiResult;
        }
        if((mtuParam.mtu - CY_BLE_GATT_WRITE_HEADER_LEN) < len) {
            len = mtuParam.mtu - CY_BLE_GATT_WRITE_HEADER_LEN;
        }
        /* Send the attribute value to to the peer device */
        cy_stc_ble_gatt_handle_value_pair_t indReqParam = {
            .attrHandle = CUSTOM_RES_CHAR_HANDLE,
            .value.val  = res,
            .value.len  = len
        };
        apiResult = Cy_BLE_GATTS_SendIndication(&m_psoc6_ble_conn_handle, &indReqParam);
    }
    return(apiResult);
}

/* [] END OF FILE */

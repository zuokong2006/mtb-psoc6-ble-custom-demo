/***************************************************************************//**
* \file ble_app.c
* \version 1.0
*
* \brief
* Source file for BLE application..
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/
#include "ble_common.h"
#include "ble_bond.h"
#include "ble_app.h"

#define BLESS_INTR_PRIORITY                         (1u)

/**
 * @brief Enable or Disable the BLE timer function
 */
#define ENABLE_BLE_MAIN_TIMER                       DISABLED

/**
 * @brief The default MTU size
 */
#define DEFAULT_MTU_SIZE                            (23)

/**
 * @brief Global Handle to internal BLE structure which holds the BLE connected handle.
 */
cy_stc_ble_conn_handle_t ble_app_conn_handle;

/**
 * @brief Global buffer for generated keys are store.
 */
static cy_stc_ble_gap_sec_key_info_t keyInfo =
{
    .localKeysFlag    = CY_BLE_GAP_SMP_INIT_ENC_KEY_DIST | 
                        CY_BLE_GAP_SMP_INIT_IRK_KEY_DIST | 
                        CY_BLE_GAP_SMP_INIT_CSRK_KEY_DIST,
    .exchangeKeysFlag = CY_BLE_GAP_SMP_INIT_ENC_KEY_DIST | 
                        CY_BLE_GAP_SMP_INIT_IRK_KEY_DIST | 
                        CY_BLE_GAP_SMP_INIT_CSRK_KEY_DIST |
                        CY_BLE_GAP_SMP_RESP_ENC_KEY_DIST |
                        CY_BLE_GAP_SMP_RESP_IRK_KEY_DIST |
                        CY_BLE_GAP_SMP_RESP_CSRK_KEY_DIST,
};

#if ENABLE_BLE_MAIN_TIMER == ENABLED
static volatile uint32_t        mainTimer  = 1u;
static cy_stc_ble_timer_info_t  timerParam = { .timeout = 1 };
#endif

/**
 * @brief The size of negotiate MTU.
 */
static uint16_t negotiatedMtu = DEFAULT_MTU_SIZE;


/******************************************************************************
* Function Name: bless_interrupt_handler
*******************************************************************************
* Summary:
*  Wrapper function for handling interrupts from BLESS.
*
******************************************************************************/
#if (CY_BLE_CONTR_CORE == CY_CPU_CORTEX_M4)
static void bless_interrupt_handler(void)
{
    Cy_BLE_BlessIsrHandler();
}
#endif

/*******************************************************************************
* Function Name: ble_app_callback
****************************************************************************//**
*
* This is an event callback function to receive events from the BLE Component.
*
* \param event the event code.
*
* \param eventParam the event parameters.
*
* \return none.
*
*******************************************************************************/
static void ble_app_callback(uint32_t event, void* eventParam)
{
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;

    switch (event)
    {
        /**********************************************************
        *                       General Events
        ***********************************************************/
        /* This event is received when the component is Started */
        case CY_BLE_EVT_STACK_ON:
            BLE_DBG_PRINTF("CY_BLE_EVT_STACK_ON\r\n");
            /* Enter into discoverable mode so that remote can find it. */
            if(CY_BLE_SUCCESS != (apiResult = Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, \
                                                           CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX)))
            {
                BLE_DBG_PRINTF("Cy_BLE_GAPP_StartAdvertisement API Error: 0x%x\r\n", apiResult);
            }
            /* Display Bond list */
            ble_display_bond_list();
            break;
            
        case CY_BLE_EVT_TIMEOUT:
            BLE_DBG_PRINTF("CY_BLE_EVT_TIMEOUT\r\n");
            #if ENABLE_BLE_MAIN_TIMER == ENABLED
            if((((cy_stc_ble_timeout_param_t *)eventParam)->reasonCode == CY_BLE_GENERIC_APP_TO) && 
               (((cy_stc_ble_timeout_param_t *)eventParam)->timerHandle == timerParam.timerHandle))
            {
                mainTimer++;
                BLE_DBG_PRINTF("mian timer out\r\n");
            }
            #endif
            break;
            
        case CY_BLE_EVT_HARDWARE_ERROR:
            /* This event indicates that some internal HW error has occurred. */
            BLE_DBG_PRINTF("CY_BLE_EVT_HARDWARE_ERROR\r\n");
            /* Halt CPU in Debug mode */
            CY_ASSERT(0u != 0u);
            break;
            
        case CY_BLE_EVT_STACK_BUSY_STATUS:
            BLE_DBG_PRINTF("CY_BLE_EVT_STACK_BUSY_STATUS: %x\r\n", *(uint8_t *)eventParam);
            break;
        case CY_BLE_EVT_SET_TX_PWR_COMPLETE:
            BLE_DBG_PRINTF("CY_BLE_EVT_SET_TX_PWR_COMPLETE \r\n");
            break;
        case CY_BLE_EVT_LE_SET_EVENT_MASK_COMPLETE:
            BLE_DBG_PRINTF("CY_BLE_EVT_LE_SET_EVENT_MASK_COMPLETE \r\n");
            break;
            
        case CY_BLE_EVT_SET_DEVICE_ADDR_COMPLETE:
            BLE_DBG_PRINTF("CY_BLE_EVT_SET_DEVICE_ADDR_COMPLETE \r\n");
            /* Reads the BD device address from BLE Controller's memory */
            Cy_BLE_GAP_GetBdAddress();
            break;
        case CY_BLE_EVT_GET_DEVICE_ADDR_COMPLETE:
            BLE_DBG_PRINTF("CY_BLE_EVT_GET_DEVICE_ADDR_COMPLETE: ");
            for(uint8_t i = CY_BLE_GAP_BD_ADDR_SIZE; i > 0u; i--)
            {
                BLE_DBG_PRINTF("%2.2x", ((cy_stc_ble_bd_addrs_t *)\
                ((cy_stc_ble_events_param_generic_t *)eventParam)->eventParams)->publicBdAddr[i-1]);
            }
            BLE_DBG_PRINTF(", private:");
            for(uint8_t i = CY_BLE_GAP_BD_ADDR_SIZE; i > 0u; i--)
            {
                BLE_DBG_PRINTF("%2.2x", ((cy_stc_ble_bd_addrs_t *)((cy_stc_ble_events_param_generic_t *)eventParam)->
                                                                 eventParams)->privateBdAddr[i-1]);
            }
            BLE_DBG_PRINTF("\r\n");
            /* Generates the security keys */
            if(CY_BLE_SUCCESS != (apiResult = Cy_BLE_GAP_GenerateKeys(&keyInfo)))
            {
                BLE_DBG_PRINTF("Cy_BLE_GAP_GenerateKeys API Error: 0x%x\r\n", apiResult);
            }
            break;
            
        case CY_BLE_EVT_STACK_SHUTDOWN_COMPLETE:
            BLE_DBG_PRINTF("CY_BLE_EVT_STACK_SHUTDOWN_COMPLETE\r\n");
            break;
            
        /** This event notifies the Host of a change to either the maximum Payload length or
         *  the maximum transmission time of Data Channel PDUs in either direction. The values 
         *  reported are the maximum that will actually be used on the connection following the change.
         *  The event parameter is of type 'cy_stc_ble_data_length_change_event_param_t'
         */
        case CY_BLE_EVT_DATA_LENGTH_CHANGE:
            BLE_DBG_PRINTF("CY_BLE_EVT_DATA_LENGTH_CHANGE: connMaxTxOctets=%d, connMaxTxTime=%d, "
                "connMaxRxOctets=%d, connMaxRxTime=%d, bdHandle=0x%x\r\n", 
                (*(cy_stc_ble_data_length_change_event_param_t *)eventParam).connMaxTxOctets, 
                (*(cy_stc_ble_data_length_change_event_param_t *)eventParam).connMaxTxTime, 
                (*(cy_stc_ble_data_length_change_event_param_t *)eventParam).connMaxRxOctets, 
                (*(cy_stc_ble_data_length_change_event_param_t *)eventParam).connMaxRxTime, 
                (*(cy_stc_ble_data_length_change_event_param_t *)eventParam).bdHandle);
            break;
        case CY_BLE_EVT_SET_SUGGESTED_DATA_LENGTH_COMPLETE:
            BLE_DBG_PRINTF("CY_BLE_EVT_SET_SUGGESTED_DATA_LENGTH_COMPLETE\r\n");
            break;
            
        case CY_BLE_EVT_GET_DATA_LENGTH_COMPLETE:
            BLE_DBG_PRINTF("CY_BLE_EVT_GET_DATA_LENGTH_COMPLETE: suggestedTxOctets=%d, suggestedTxTime=%d, "
                "TO=%d, TT=%d,RO=%d, RT=%d\r\n", 
                (*(cy_stc_ble_data_length_param_t *)(((cy_stc_ble_events_param_generic_t*)eventParam)->eventParams)).suggestedTxOctets, 
                (*(cy_stc_ble_data_length_param_t *)(((cy_stc_ble_events_param_generic_t*)eventParam)->eventParams)).suggestedTxTime, 
                (*(cy_stc_ble_data_length_param_t *)(((cy_stc_ble_events_param_generic_t*)eventParam)->eventParams)).maxTxOctets, 
                (*(cy_stc_ble_data_length_param_t *)(((cy_stc_ble_events_param_generic_t*)eventParam)->eventParams)).maxTxTime, 
                (*(cy_stc_ble_data_length_param_t *)(((cy_stc_ble_events_param_generic_t*)eventParam)->eventParams)).maxRxOctets,
                (*(cy_stc_ble_data_length_param_t *)(((cy_stc_ble_events_param_generic_t*)eventParam)->eventParams)).maxRxTime);
            break;
            
        #if(CY_BLE_CONFIG_ENABLE_PHY_UPDATE != 0u)
        case CY_BLE_EVT_SET_DEFAULT_PHY_COMPLETE:
            BLE_DBG_PRINTF("CY_BLE_EVT_SET_DEFAULT_PHY_COMPLETE \r\n");
            break;
        #endif  /* (CY_BLE_CONFIG_ENABLE_PHY_UPDATE != 0u) */
        
        /**********************************************************
        *                       GAP Events
        ***********************************************************/
        case CY_BLE_EVT_GAP_KEYS_GEN_COMPLETE:
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_KEYS_GEN_COMPLETE \r\n");
            keyInfo.SecKeyParam = (*(cy_stc_ble_gap_sec_key_param_t *)eventParam);
            apiResult = Cy_BLE_GAP_SetIdAddress(&cy_ble_deviceAddress);
            if(apiResult != CY_BLE_SUCCESS)
            {
                BLE_DBG_PRINTF("Cy_BLE_GAP_SetIdAddress API Error: 0x%x \r\n", apiResult);
            }
            #if(CY_BLE_CONFIG_ENABLE_PHY_UPDATE != 0u)
            {
                const cy_stc_ble_set_suggested_phy_info_t phyInfo =
                {
                    .allPhyMask = CY_BLE_PHY_NO_PREF_MASK_NONE,
                    .txPhyMask = CY_BLE_PHY_MASK_LE_2M,
                    .rxPhyMask = CY_BLE_PHY_MASK_LE_2M
                };
                apiResult = Cy_BLE_SetDefaultPhy(&phyInfo);
                if(apiResult != CY_BLE_SUCCESS)
                {
                    BLE_DBG_PRINTF("Cy_BLE_SetDefaultPhy API Error: 0x%x \r\n", apiResult);
                }
            }
            #endif  /* (CY_BLE_CONFIG_ENABLE_PHY_UPDATE != 0u) */
            break;
            
        case CY_BLE_EVT_GAP_AUTH_REQ:
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_AUTH_REQ: bdHandle=%x, security=%x, bonding=%x, ekeySize=%x, err=%x \r\n", 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bdHandle, (*(cy_stc_ble_gap_auth_info_t *)eventParam).security, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bonding, (*(cy_stc_ble_gap_auth_info_t *)eventParam).ekeySize, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).authErr);
            if(cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX].security == 
                (CY_BLE_GAP_SEC_MODE_1 | CY_BLE_GAP_SEC_LEVEL_1))
            {
                cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX].authErr = 
                    CY_BLE_GAP_AUTH_ERROR_PAIRING_NOT_SUPPORTED;
            }
            cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX].bdHandle = 
                ((cy_stc_ble_gap_auth_info_t *)eventParam)->bdHandle;
            /* Pass security information for authentication in reply to an authentication request 
             * from the master device */
            apiResult = Cy_BLE_GAPP_AuthReqReply(&cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX]);
            if(apiResult != CY_BLE_SUCCESS)
            {
                if(CY_BLE_SUCCESS != (apiResult = Cy_BLE_GAP_RemoveOldestDeviceFromBondedList()))
                {
                    BLE_DBG_PRINTF("Cy_BLE_GAP_RemoveOldestDeviceFromBondedList API Error: 0x%x \r\n", apiResult);
                }
                else
                {
                    apiResult = Cy_BLE_GAPP_AuthReqReply(&cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX]);
                    if(apiResult != CY_BLE_SUCCESS)
                    {
                        BLE_DBG_PRINTF("Cy_BLE_GAPP_AuthReqReply API Error: 0x%x \r\n", apiResult);
                    }
                }
            }
            break;
            
        case CY_BLE_EVT_GAP_PASSKEY_ENTRY_REQUEST:
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_PASSKEY_ENTRY_REQUEST\r\n");
            BLE_DBG_PRINTF("Please enter the passkey displayed on the peer device:\r\n");
            break;
            
        case CY_BLE_EVT_GAP_PASSKEY_DISPLAY_REQUEST:
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_PASSKEY_DISPLAY_REQUEST: %6.6ld\r\n", *(uint32_t *)eventParam);
            break;
            
        case CY_BLE_EVT_GAP_NUMERIC_COMPARISON_REQUEST:
            BLE_DBG_PRINTF("Compare this passkey with the one displayed in your peer device and press 'y' or 'n':"
                       " %6.6lu \r\n", *(uint32_t *)eventParam);
            break;
            
        case CY_BLE_EVT_GAP_KEYINFO_EXCHNGE_CMPLT:
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_KEYINFO_EXCHNGE_CMPLT \r\n");
            break;
            
        case CY_BLE_EVT_GAP_SMP_NEGOTIATED_AUTH_INFO:
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_SMP_NEGOTIATED_AUTH_INFO:"
                       " bdHandle=%x, security=%x, bonding=%x, ekeySize=%x, err=%x \r\n", 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bdHandle, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).security, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bonding, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).ekeySize, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).authErr);
            break;
            
        case CY_BLE_EVT_GAP_AUTH_COMPLETE:
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_AUTH_COMPLETE: bdHandle=%x, security=%x, bonding=%x, ekeySize=%x, err=%x \r\n", 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bdHandle, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).security, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).bonding, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).ekeySize, 
                (*(cy_stc_ble_gap_auth_info_t *)eventParam).authErr);
            break;
            
        case CY_BLE_EVT_GAP_AUTH_FAILED:
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_AUTH_FAILED, reason: ");
            switch((*(cy_stc_ble_gap_auth_info_t *)eventParam).authErr)
            {
                case CY_BLE_GAP_AUTH_ERROR_CONFIRM_VALUE_NOT_MATCH:
                    BLE_DBG_PRINTF("CONFIRM_VALUE_NOT_MATCH\r\n");
                    break;
                    
                case CY_BLE_GAP_AUTH_ERROR_INSUFFICIENT_ENCRYPTION_KEY_SIZE:
                    BLE_DBG_PRINTF("INSUFFICIENT_ENCRYPTION_KEY_SIZE\r\n");
                    break;
                
                case CY_BLE_GAP_AUTH_ERROR_UNSPECIFIED_REASON:
                    BLE_DBG_PRINTF("UNSPECIFIED_REASON\r\n");
                    break;
                    
                case CY_BLE_GAP_AUTH_ERROR_AUTHENTICATION_TIMEOUT:
                    BLE_DBG_PRINTF("AUTHENTICATION_TIMEOUT\r\n");
                    break;
                    
                default:
                    BLE_DBG_PRINTF("0x%x  \r\n", (*(cy_stc_ble_gap_auth_info_t *)eventParam).authErr);
                    break;
            }
            break;

            
        case CY_BLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
            BLE_DBG_PRINTF("CY_BLE_EVT_GAPP_ADVERTISEMENT_START_STOP, state: %d \r\n", Cy_BLE_GetAdvertisementState());
            break;
            
    #if(CY_BLE_LL_PRIVACY_FEATURE_ENABLED)
        case CY_BLE_EVT_GAP_ENHANCE_CONN_COMPLETE:
        {
            cy_stc_ble_gap_enhance_conn_complete_param_t *enhConnParameters;
            uint8_t connectedBdHandle;
            enhConnParameters = (cy_stc_ble_gap_enhance_conn_complete_param_t *)eventParam;
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_ENHANCE_CONN_COMPLETE: %x, %x(%.2f ms), %x ", enhConnParameters->status,
                enhConnParameters->connIntv, (float)enhConnParameters->connIntv * CY_BLE_CONN_INTRV_TO_MS, enhConnParameters->connLatency);
            connectedBdHandle = enhConnParameters->bdHandle;
            if(enhConnParameters->status == 0x00)
            {
                BLE_DBG_PRINTF("\x1B[31m supervisionTO=%dms peerBdAddrType=%d peerBdAddr=",
                    enhConnParameters->supervisionTo*10, enhConnParameters->peerBdAddrType);
                for(uint8_t i = CY_BLE_GAP_BD_ADDR_SIZE; i > 0u; i--)
                {
                    BLE_DBG_PRINTF("%2.2x", enhConnParameters->peerBdAddr[i-1]);
                }
                if(enhConnParameters->peerBdAddrType == CY_BLE_GAP_RANDOM_RESOLVABLE_ADDR_TYPE)
                    BLE_DBG_PRINTF(" CY_BLE_GAP_RANDOM_RESOLVABLE_ADDR_TYPE");
                BLE_DBG_PRINTF("\x1B[0m");
            }
            BLE_DBG_PRINTF("\r\n");
    #else
        case CY_BLE_EVT_GAP_DEVICE_CONNECTED:
        {
            uint8_t connectedBdHandle;
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_DEVICE_CONNECTED: %x, %x(%.2f ms), %x, %x \r\n",   
                ((cy_stc_ble_gap_connected_param_t *)eventParam)->status,
                ((cy_stc_ble_gap_connected_param_t *)eventParam)->connIntv,
                (float)((cy_stc_ble_gap_connected_param_t *)eventParam)->connIntv * CY_BLE_CONN_INTRV_TO_MS,
                ((cy_stc_ble_gap_connected_param_t *)eventParam)->connLatency,
                ((cy_stc_ble_gap_connected_param_t *)eventParam)->supervisionTO);
            connectedBdHandle = ((cy_stc_ble_gap_connected_param_t *)eventParam)->bdHandle;
    #endif  /* CY_BLE_LL_PRIVACY_FEATURE_ENABLED */
            /* Initiate pairing process */
            if((cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX].security & CY_BLE_GAP_SEC_LEVEL_MASK) > 
                CY_BLE_GAP_SEC_LEVEL_1)
            {
                cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX].bdHandle = ble_app_conn_handle.bdHandle;
                apiResult = Cy_BLE_GAP_AuthReq(&cy_ble_configPtr->authInfo[CY_BLE_SECURITY_CONFIGURATION_0_INDEX]);
                if(apiResult != CY_BLE_SUCCESS)
                {
                    BLE_DBG_PRINTF(" Cy_BLE_GAP_AuthReq API Error: 0x%x \r\n", apiResult);
                }
            }
            /* Set security keys for new device which is not already bonded */
            if(false == ble_is_device_in_bond_list(connectedBdHandle))
            {
                keyInfo.SecKeyParam.bdHandle = connectedBdHandle;
                apiResult = Cy_BLE_GAP_SetSecurityKeys(&keyInfo);
                if(apiResult != CY_BLE_SUCCESS)
                {
                    BLE_DBG_PRINTF("Cy_BLE_GAP_SetSecurityKeys API Error: 0x%x \r\n", apiResult);
                }
            }
        }
            break;

        case CY_BLE_EVT_L2CAP_CONN_PARAM_UPDATE_RSP:
            BLE_DBG_PRINTF("CY_BLE_EVT_L2CAP_CONN_PARAM_UPDATE_RSP, result = %d\r\n", 
                (*(cy_stc_ble_l2cap_conn_update_rsp_param_t *)eventParam).result);
            break;
            
        case CY_BLE_EVT_GAP_CONNECTION_UPDATE_COMPLETE:
            /* in milliseconds / 1.25ms */
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_CONNECTION_UPDATE_COMPLETE: connIntv = %dms connLatency=%d supervisionTO=%dms\r\n",
                        ((cy_stc_ble_gap_conn_param_updated_in_controller_t *)eventParam)->connIntv * 5u /4u,
                        ((cy_stc_ble_gap_conn_param_updated_in_controller_t *)eventParam)->connLatency,
                        ((cy_stc_ble_gap_conn_param_updated_in_controller_t *)eventParam)->supervisionTO*10);
            break;
            
        case CY_BLE_EVT_GAP_DEVICE_DISCONNECTED:
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_DEVICE_DISCONNECTED: bdHandle=%x, reason=%x, status=%x\r\n",
                (*(cy_stc_ble_gap_disconnect_param_t *)eventParam).bdHandle, 
                (*(cy_stc_ble_gap_disconnect_param_t *)eventParam).reason, 
                (*(cy_stc_ble_gap_disconnect_param_t *)eventParam).status);
            negotiatedMtu = DEFAULT_MTU_SIZE;
            break;
            
        case CY_BLE_EVT_GAP_ENCRYPT_CHANGE:
            BLE_DBG_PRINTF("CY_BLE_EVT_GAP_ENCRYPT_CHANGE: %x \r\n", *(uint8_t *)eventParam);
            break;

        /**********************************************************
        *                       GATT Events
        ***********************************************************/
        /* This event is received when device is connected over GATT level */
        case CY_BLE_EVT_GATT_CONNECT_IND:
            ble_app_conn_handle = *(cy_stc_ble_conn_handle_t *)eventParam;
            BLE_DBG_PRINTF("CY_BLE_EVT_GATT_CONNECT_IND: %x, %x \r\n", 
                (*(cy_stc_ble_conn_handle_t *)eventParam).attId, (*(cy_stc_ble_conn_handle_t *)eventParam).bdHandle);
            break;
            
        /* This event is received when device is disconnected */
        case CY_BLE_EVT_GATT_DISCONNECT_IND:
            BLE_DBG_PRINTF("CY_BLE_EVT_GATT_DISCONNECT_IND: %x, %x \r\n", 
                (*(cy_stc_ble_conn_handle_t *)eventParam).attId, (*(cy_stc_ble_conn_handle_t *)eventParam).bdHandle);
            break;
            
        case CY_BLE_EVT_GATTS_XCNHG_MTU_REQ:
            negotiatedMtu = (((cy_stc_ble_gatt_xchg_mtu_param_t *)eventParam)->mtu < CY_BLE_GATT_MTU) ?
                        ((cy_stc_ble_gatt_xchg_mtu_param_t *)eventParam)->mtu : CY_BLE_GATT_MTU;
            BLE_DBG_PRINTF("CY_BLE_EVT_GATTS_XCNHG_MTU_REQ mtu=%d\r\n", ((cy_stc_ble_gatt_xchg_mtu_param_t *)eventParam)->mtu);
            break;
            
        case CY_BLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ:
            BLE_DBG_PRINTF("CY_BLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ %x %x: handle: %x \r\n", 
                ((cy_stc_ble_gatts_char_val_read_req_t *)eventParam)->connHandle.attId,
                ((cy_stc_ble_gatts_char_val_read_req_t *)eventParam)->connHandle.bdHandle,
                ((cy_stc_ble_gatts_char_val_read_req_t *)eventParam)->attrHandle);
            break;

        case CY_BLE_EVT_GATTS_WRITE_REQ:
            BLE_DBG_PRINTF("CY_BLE_EVT_GATTS_WRITE_REQ\r\n");
            break;
            
        case CY_BLE_EVT_GATTS_WRITE_CMD_REQ:
            BLE_DBG_PRINTF("CY_BLE_EVT_GATTS_WRITE_CMD_REQ\r\n");
            break;
        
        case CY_BLE_EVT_GATTS_HANDLE_VALUE_CNF:
            BLE_DBG_PRINTF("CY_BLE_EVT_GATTS_HANDLE_VALUE_CNF: %x, %x \r\n", 
                (*(cy_stc_ble_conn_handle_t *)eventParam).attId, (*(cy_stc_ble_conn_handle_t *)eventParam).bdHandle);
            break;
        
        case CY_BLE_EVT_GATTS_INDICATION_DISABLED:
            BLE_DBG_PRINTF("CY_BLE_EVT_GATTS_INDICATION_DISABLED\r\n");
            break;
            
        case CY_BLE_EVT_GATTS_INDICATION_ENABLED:
            BLE_DBG_PRINTF("CY_BLE_EVT_GATTS_INDICATION_ENABLED\r\n");
            break;
        
        /**********************************************************
        *                       Other Events
        ***********************************************************/
        case CY_BLE_EVT_PENDING_FLASH_WRITE:
            /* Inform application that flash write is pending. Stack internal data 
            * structures are modified and require to be stored in Flash using 
            * Cy_BLE_StoreBondingData() */
            BLE_DBG_PRINTF("CY_BLE_EVT_PENDING_FLASH_WRITE\r\n");
            break;
            
        default:
            BLE_DBG_PRINTF("Other event: 0x%x\r\n", (unsigned int)event);
            break;
    }
    
    /* Custom host interface event callback */
    ble_custom_hi_service_evt_callback(event, eventParam);
}

/*******************************************************************************
* Function Name: ble_app_init
****************************************************************************//**
*
* Initializes the BLE application.
*
* \param None
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
cy_en_ble_api_result_t ble_app_init(void)
{
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;
    cy_stc_ble_stack_lib_version_t stackVersion;
    
    /* Initialize Debug UART for BLE */
    BLE_UART_START();
    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    BLE_DBG_PRINTF("\x1b[2J\x1b[;H");
    BLE_DBG_PRINTF("****************************************************************\r\n");
    BLE_DBG_PRINTF("*                  PSoC6 Custom Service Demo                   *\r\n");
    BLE_DBG_PRINTF("****************************************************************\r\n");

#if (CY_BLE_CONTR_CORE == CY_CPU_CORTEX_M4)
    BLE_DBG_PRINTF("BLE stack controller on CM4 core, ");
#else
    BLE_DBG_PRINTF("BLE stack controller on CM0+ core, ");
#endif
#if (CY_BLE_HOST_CORE == CY_CPU_CORTEX_M4)
    BLE_DBG_PRINTF("host on CM4 core\r\n");
#else
    BLE_DBG_PRINTF("host on CM0+ core\r\n");
#endif

#if (CY_BLE_CONTR_CORE == CY_CPU_CORTEX_M4)
    static const cy_stc_sysint_t bless_isr_config =
    {
      /* The BLESS interrupt */
      .intrSrc = bless_interrupt_IRQn,

      /* The interrupt priority number */
      .intrPriority = BLESS_INTR_PRIORITY
    };
    /* Hook interrupt service routines for BLESS */
    (void) Cy_SysInt_Init(&bless_isr_config, bless_interrupt_handler);
    /* Store the pointer to blessIsrCfg in the BLE configuration structure */
    cy_ble_config.hw->blessIsrConfig = &bless_isr_config;
#endif

    /* Registers the generic callback functions  */
    Cy_BLE_RegisterEventCallback(ble_app_callback);

    /* Initializes the BLE host */
    if(CY_BLE_SUCCESS != (apiResult = Cy_BLE_Init(&cy_ble_config))) {
        BLE_DBG_PRINTF("Cy_BLE_Init API Error: ");
        ble_print_api_result(apiResult);
        return apiResult;
    }

    /* Enables BLE */
    if(CY_BLE_SUCCESS != (apiResult = Cy_BLE_Enable())) {
        BLE_DBG_PRINTF("Cy_BLE_Enable API Error: ");
        ble_print_api_result(apiResult);
        return apiResult;
    }

    /* Enables BLE Low-power mode (LPM)*/
    Cy_BLE_EnableLowPowerMode();

    /* Output current stack version to UART */
    apiResult = Cy_BLE_GetStackLibraryVersion(&stackVersion);
    if(apiResult != CY_BLE_SUCCESS) {
        BLE_DBG_PRINTF("CyBle_GetStackLibraryVersion API Error: 0x%2.2x \r\n", apiResult);
    } else {
        BLE_DBG_PRINTF("Stack Version: %d.%d.%d.%d \r\n", stackVersion.majorVersion, 
            stackVersion.minorVersion, stackVersion.patch, stackVersion.buildNumber);
    }
    return apiResult;
}

/*******************************************************************************
* Function Name: ble_app_task
****************************************************************************//**
*
* BLE application task.
*
* \param none.
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
cy_en_ble_api_result_t ble_app_task(void)
{
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;
    
    /* Cy_BLE_ProcessEvents() allows BLE stack to process pending events */
    Cy_BLE_ProcessEvents();
    
    /* To achieve low power in the device */
    if(BLE_UART_DEB_IS_TX_COMPLETE() != 0u) {
        /* Entering into the Deep Sleep */
        Cy_SysPm_DeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
    }
    
    /* Restart the advertisement */
    if((Cy_BLE_GetState() == CY_BLE_STATE_ON) \
        && (Cy_BLE_GetAdvertisementState() == CY_BLE_ADV_STATE_STOPPED) \
        && (Cy_BLE_GetNumOfActiveConn() < 1))
    {
        apiResult = Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);
        if(apiResult != CY_BLE_SUCCESS)
        {
            BLE_DBG_PRINTF("Task StartAdvertisement API Error: ");
            ble_print_api_result(apiResult);
        }
        BLE_DBG_PRINTF("Task StartAdvertisement\r\n");
        Cy_BLE_ProcessEvents();
    }
    
    /* Restart timer */
    #if ENABLE_BLE_MAIN_TIMER == ENABLED
    if(mainTimer != 0u)
    {
        mainTimer = 0u;
        Cy_BLE_StartTimer(&timerParam);
    }
    #endif
    
    /* Store bonding data to flash only when all debug information has been sent */
    #if(CY_BLE_BONDING_REQUIREMENT == CY_BLE_BONDING_YES)
    if(cy_ble_pendingFlashWrite != 0u) 
    {
        apiResult = Cy_BLE_StoreBondingData();
        BLE_DBG_PRINTF("Store bonding data, status: %x, pending: %x \r\n", apiResult, cy_ble_pendingFlashWrite);
    }
    #endif /* CY_BLE_BONDING_REQUIREMENT == CY_BLE_BONDING_YES */
    
    return CY_BLE_SUCCESS;
}

/*******************************************************************************
* Function Name: ble_app_stop
****************************************************************************//**
*
* Stops any ongoing operation in the BLE Stack and forces the BLE Stack to shut down.
*
* \param none
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
cy_en_ble_api_result_t ble_app_stop(void)
{
    cy_en_ble_api_result_t apiResult = Cy_BLE_Disable();
    if(apiResult != CY_BLE_SUCCESS)
    {
        BLE_DBG_PRINTF("Cy_BLE_Stop API Error: ");
        ble_print_api_result(apiResult);
    }
    else
    {
        BLE_DBG_PRINTF("Wait for the BLE to Stop\r\n");
        while(CY_BLE_STATE_STOPPED != Cy_BLE_GetState())
        {
            Cy_BLE_ProcessEvents();
        }
        BLE_DBG_PRINTF("BLE Stopped!\r\n");
        BLE_UART_DEB_WAIT_TX_COMPLETE();
    }
    return apiResult;
}

/*******************************************************************************
* Function Name: ble_app_connection_param_update_request
****************************************************************************//**
*
* Request the peer Central device to update the connection parameters.
*
* \param interval_min  Minimum value for the connection event interval. This shall be less than
*                      or equal to conn_Interval_Max. Minimum connection interval will be
*                      connIntvMin * 1.25 ms. Time Range: 7.5 ms to 4 sec
*
* \param interval_max  Maximum value for the connection event interval. This shall be greater
*                      than or equal to conn_Interval_Min. Maximum connection interval will be
*                      connIntvMax * 1.25 ms. Time Range: 7.5 ms to 4 sec
*
* \param slave_latency Slave latency for the connection in number of connection events.
*                      Range: 0x0000 to 0x01F3
*
* \param timeout_multiplier Supervision timeout for the LE Link. Supervision timeout will be
*                           supervisionTO * 10 ms. Time Range: 100 msec to 32 secs
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
cy_en_ble_api_result_t ble_app_connection_param_update_request(uint16_t interval_min, \
    uint16_t interval_max, uint16_t slave_latency, uint16_t timeout_multiplier)
{
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;
    cy_stc_ble_gap_conn_update_param_info_t param;
    
    if(((6 > interval_min) || (interval_min > 3200)) || ((6 > interval_max) || (interval_max > 3200)) 
    || (500 < slave_latency) || ((10 > timeout_multiplier) || (timeout_multiplier > 3200)))
    {
        BLE_DBG_PRINTF("ble_app_connection_param_update_request Input param error\r\n");
        return CY_BLE_ERROR_INVALID_PARAMETER;
    }
    if((Cy_BLE_GetConnectionState(ble_app_conn_handle) != CY_BLE_CONN_STATE_CONNECTED) \
        && (Cy_BLE_GetNumOfActiveConn() < 1))
    {
        BLE_DBG_PRINTF("ble_app_connection_param_update_request No connection\r\n");
        return CY_BLE_ERROR_NO_CONNECTION;
    }
    param.bdHandle = ble_app_conn_handle.bdHandle;
    param.connIntvMin = interval_min;
    param.connIntvMax = interval_max;
    param.connLatency = slave_latency;
    param.supervisionTO = timeout_multiplier;
    if(CY_BLE_SUCCESS != (apiResult = Cy_BLE_L2CAP_LeConnectionParamUpdateRequest(&param)))
    {
        BLE_DBG_PRINTF("Cy_BLE_L2CAP_LeConnectionParamUpdateRequest API Error: 0x%x \r\n", apiResult);
        return apiResult;
    }
    return apiResult;
}

/*******************************************************************************
* Function Name: ble_app_negotiate_mtu
****************************************************************************//**
*
* Get the negotiate mtu size.
*
* \param none
*
* \return Return the negotiate mtu size.
*
*******************************************************************************/
uint16_t ble_app_negotiate_mtu(void)
{
    return negotiatedMtu;
}

/* [] END OF FILE */

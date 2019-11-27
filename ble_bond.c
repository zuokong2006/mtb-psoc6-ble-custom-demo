/***************************************************************************//**
* \file ble_bas.c
* \version 1.0
*
* \brief
* Source file for BLE bond list helper APIs.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/
#include "ble_common.h"


/*******************************************************************************
* Function Name: ble_display_bond_list
****************************************************************************//**
*
* This function displays the bond list.
*
* \param none.
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
cy_en_ble_api_result_t ble_display_bond_list(void)
{
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;
    #if (CY_BLE_BONDING_REQUIREMENT == CY_BLE_BONDING_YES)
    cy_stc_ble_gap_peer_addr_info_t bondedDeviceInfo[CY_BLE_MAX_BONDED_DEVICES];
    cy_stc_ble_gap_bonded_device_list_info_t bondedDeviceList = {.bdHandleAddrList = bondedDeviceInfo};
    uint8_t deviceCount = 0;
    
    /* Find out whether the device has bonded information stored already or not */
    apiResult = Cy_BLE_GAP_GetBondList(&bondedDeviceList);
    if(apiResult != CY_BLE_SUCCESS)
    {
        BLE_DBG_PRINTF("Cy_BLE_GAP_GetBondList API Error: 0x%x\r\n", apiResult);
    }
    else
    {
        deviceCount = bondedDeviceList.noOfDevices;
        if(deviceCount != 0u)
        {
            uint8_t counter;
            BLE_DBG_PRINTF("Bond list: \r\n");
            do
            {
                BLE_DBG_PRINTF("%d. ", deviceCount);
                deviceCount--;
                /* Bluetooth Device Address type */
                if(bondedDeviceList.bdHandleAddrList[deviceCount].bdAddr.type == CY_BLE_GAP_ADDR_TYPE_RANDOM)
                {
                    BLE_DBG_PRINTF("Peer Random Address:");
                }
                else
                {
                    BLE_DBG_PRINTF("Peer Public Address:");
                }
                /* Bluetooth Device Address */
                for(counter = CY_BLE_GAP_BD_ADDR_SIZE; counter > 0u; counter--)
                {
                    BLE_DBG_PRINTF(" %2.2x", bondedDeviceList.bdHandleAddrList[deviceCount].bdAddr.bdAddr[counter - 1u]);
                }
                BLE_DBG_PRINTF(", bdHandle: %x \r\n", bondedDeviceList.bdHandleAddrList[deviceCount].bdHandle);
            } while(deviceCount != 0u);
        }
    }
    #endif
    return apiResult;
}

/*******************************************************************************
* Function Name: ble_remove_devices_from_bond_list
****************************************************************************//**
*
* Remove devices from the bond list.
*
* \param none.
*
* \return Return value indicates if the function succeeded or failed.
* see \ref cy_en_ble_api_result_t.
*
*******************************************************************************/
cy_en_ble_api_result_t ble_remove_devices_from_bond_list(void)
{
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;
    
    #if(CY_BLE_BONDING_REQUIREMENT == CY_BLE_BONDING_YES)
    cy_stc_ble_gap_bd_addr_t peerBdAddr = { .type = 0u };
    BLE_DBG_PRINTF("\r\nCleaning Bond List.....\r\n");
    /* Remove all bonded devices in the list */
    apiResult = Cy_BLE_GAP_RemoveBondedDevice(&peerBdAddr);
    if(apiResult != CY_BLE_SUCCESS)
    {
        BLE_DBG_PRINTF("Cy_BLE_GAP_RemoveBondedDevice API Error: 0x%x\r\n", apiResult);
    }
    else
    {
        BLE_DBG_PRINTF("Cy_BLE_GAP_RemoveBondedDevice done\r\n\r\n");
    }
    #else
    BLE_DBG_PRINTF("\r\nBonding is disabled. Cleaning Bond List.....skip\r\n");
    #endif /* (CY_BLE_BONDING_REQUIREMENT == CY_BLE_BONDING_YES) */
    return apiResult;
}

/*******************************************************************************
* Function Name: ble_get_count_of_bonded_devices
****************************************************************************//**
*
* This function returns the count of bonded devices.
*
* \param none.
*
* \return Number of bonded devices.
*
*******************************************************************************/
uint32_t ble_get_count_of_bonded_devices(void)
{
    #if (CY_BLE_BONDING_REQUIREMENT == CY_BLE_BONDING_YES)
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;
    cy_stc_ble_gap_peer_addr_info_t bondedDeviceInfo[CY_BLE_MAX_BONDED_DEVICES];
    cy_stc_ble_gap_bonded_device_list_info_t bondedDeviceList = {.bdHandleAddrList = bondedDeviceInfo};
    uint32_t deviceCount = 0u;
    
    /* Find out whether the device has bonded information stored already or not */
    apiResult = Cy_BLE_GAP_GetBondList(&bondedDeviceList);
    if(apiResult != CY_BLE_SUCCESS)
    {
        BLE_DBG_PRINTF("Cy_BLE_GAP_GetBondedDevicesList API Error: 0x%x \r\n", apiResult);
    }
    else
    {
        deviceCount = bondedDeviceList.noOfDevices;
    }
    return (deviceCount);
    #else
    return 0;
    #endif
}

/*******************************************************************************
* Function Name: ble_is_device_in_bond_list
****************************************************************************//**
*
* This function check if device with bdHandle is in the bond list.
*
* \param bdHandle bond device handler.
*
* \return true value when bdHandle exists in bond list.
*
*******************************************************************************/
bool ble_is_device_in_bond_list(uint32_t bdHandle)
{
    #if (CY_BLE_BONDING_REQUIREMENT == CY_BLE_BONDING_YES)
    cy_en_ble_api_result_t apiResult = CY_BLE_SUCCESS;
    cy_stc_ble_gap_peer_addr_info_t bondedDeviceInfo[CY_BLE_MAX_BONDED_DEVICES];
    cy_stc_ble_gap_bonded_device_list_info_t bondedDeviceList ={.bdHandleAddrList = bondedDeviceInfo};
    bool deviceIsDetected = false;
    uint32_t deviceCount = 0;
    
    /* Find out whether the device has bonding information stored already or not */
    apiResult = Cy_BLE_GAP_GetBondList(&bondedDeviceList);
    if(apiResult != CY_BLE_SUCCESS)
    {
        BLE_DBG_PRINTF("Cy_BLE_GAP_GetBondedDevicesList API Error: 0x%x\r\n", apiResult);
    }
    else
    {
        deviceCount = bondedDeviceList.noOfDevices;
        if(deviceCount != 0u)
        {
            do
            {
                deviceCount--;
                if(bdHandle == bondedDeviceList.bdHandleAddrList[deviceCount].bdHandle)
                {
                    deviceIsDetected = true;
                }
            } while(deviceCount != 0u);
        }
    }
    return(deviceIsDetected);
    #else
    return false;
    #endif
}

/* [] END OF FILE */

/******************************************************************************
 * @file     EEPROM_FRU_Table.h
 * @brief    EEPROM FRU Table header file
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __EEPROM_FRU_Table_H__
#define __EEPROM_FRU_Table_H__

 /*!<Includes */
#include "NuMicro.h"
#define FMC_FLASH_PAGE_SIZE 512
/*------Define FRU table content in EEPROM---------------------*/
#define EEPROM_SIZE                                   256
//#define EEPROM_FRU_TABLE_TOTAL_COUNT                  11
#define EEPROM_FRU_TABLE_TOTAL_COUNT                  12
#define EEPROM_OFFSET_FRU_BOARD_PART_NUMBER           0x72
#define EEPROM_SIZE_FRU_BOARD_PART_NUMBER             24
#define EEPROM_OFFSET_FRU_OEM_INFORMATION             0x1F
#define EEPROM_SIZE_FRU_OEM_INFORMATION               32
#define EEPROM_OFFSET_FRU_SERIAL_NUMBER               0x59
#define EEPROM_SIZE_FRU_SERIAL_NUMBER                 24
#define EEPROM_OFFSET_FRU_MARKETING_NAME              0x40
#define EEPROM_SIZE_FRU_MARKETING_NAME                24
#define EEPROM_OFFSET_FRU_BUILD_DATE                  0x1B
#define EEPROM_SIZE_FRU_BUILD_DATE                    3
#define EEPROM_OFFSET_FRU_HW_VERSION                  0x8C
#define EEPROM_SIZE_FRU_HW_VERSION                    8
#define EEPROM_OFFSET_FRU_FW_VERSION                  0x95
#define EEPROM_SIZE_FRU_FW_VERSION                    14
#define EEPROM_OFFSET_FRU_PCI_CONFIG_VENDOR_ID        0xA4
#define EEPROM_SIZE_FRU_PCI_CONFIG_VENDOR_ID          4
#define EEPROM_OFFSET_FRU_PCI_CONFIG_DEVICE_ID        0xA9
#define EEPROM_SIZE_FRU_PCI_CONFIG_DEVICE_ID          4
#define EEPROM_OFFSET_FRU_PCI_CONFIG_SUB_VENDOR_ID    0xAE
#define EEPROM_SIZE_FRU_PCI_CONFIG_SUB_VENDOR_ID      4
#define EEPROM_OFFSET_FRU_PCI_CONFIG_SUB_DEVICE_ID    0xB3
#define EEPROM_SIZE_FRU_PCI_CONFIG_SUB_DEVICE_ID      4
#define EEPROM_OFFSET_COMPANY_INFO                    0xE0
#define EEPROM_SIZE_COMPANY_INFO                      5

#define EEPROM_MAX_VALID_OFFSET                       0xE6
/*-------------------------------------------------------------*/

#endif  /* __EEPROM_FRU_Table_H__ */

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/

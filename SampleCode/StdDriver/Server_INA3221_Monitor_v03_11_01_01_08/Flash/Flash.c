/******************************************************************************//**
 * @file     Flash.c
 * @version  V1.00
 * @brief    Flash sample file
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include <stdio.h>
#include "device.h"
#include "NuMicro.h"
#include "Flash.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
volatile uint8_t EEPROM_Table[EEPROM_SIZE];
const FRU_Data_T FRU_Data_Attr[EEPROM_FRU_TABLE_TOTAL_COUNT] =
{
{EEPROM_OFFSET_FRU_OEM_INFORMATION,          EEPROM_SIZE_FRU_OEM_INFORMATION,          FLASH_ADDR_FRU_OEM_INFORMATION},
{EEPROM_OFFSET_FRU_BOARD_PART_NUMBER,        EEPROM_SIZE_FRU_BOARD_PART_NUMBER,        FLASH_ADDR_FRU_BOARD_PART_NUMBER},
{EEPROM_OFFSET_FRU_SERIAL_NUMBER,            EEPROM_SIZE_FRU_SERIAL_NUMBER,            FLASH_ADDR_FRU_SERIAL_NUMBER},
{EEPROM_OFFSET_FRU_MARKETING_NAME,           EEPROM_SIZE_FRU_MARKETING_NAME,           FLASH_ADDR_FRU_MARKETING_NAME},
{EEPROM_OFFSET_FRU_BUILD_DATE,               EEPROM_SIZE_FRU_BUILD_DATE,               FLASH_ADDR_FRU_BUILD_DATE},
{EEPROM_OFFSET_FRU_HW_VERSION,               EEPROM_SIZE_FRU_HW_VERSION,               FLASH_ADDR_FRU_HW_VERSION},
{EEPROM_OFFSET_FRU_FW_VERSION,               EEPROM_SIZE_FRU_FW_VERSION,               FLASH_ADDR_FRU_FW_VERSION},
{EEPROM_OFFSET_FRU_PCI_CONFIG_VENDOR_ID,     EEPROM_SIZE_FRU_PCI_CONFIG_VENDOR_ID,     FLASH_ADDR_FRU_PCI_CONFIG_VENDOR_ID},
{EEPROM_OFFSET_FRU_PCI_CONFIG_DEVICE_ID,     EEPROM_SIZE_FRU_PCI_CONFIG_DEVICE_ID,     FLASH_ADDR_FRU_PCI_CONFIG_DEVICE_ID},
{EEPROM_OFFSET_FRU_PCI_CONFIG_SUB_VENDOR_ID, EEPROM_SIZE_FRU_PCI_CONFIG_SUB_VENDOR_ID, FLASH_ADDR_FRU_PCI_CONFIG_SUB_VENDOR_ID},
{EEPROM_OFFSET_FRU_PCI_CONFIG_SUB_DEVICE_ID, EEPROM_SIZE_FRU_PCI_CONFIG_SUB_DEVICE_ID, FLASH_ADDR_FRU_PCI_CONFIG_SUB_DEVICE_ID},
{EEPROM_OFFSET_COMPANY_INFO, EEPROM_SIZE_COMPANY_INFO, FLASH_ADDR_COMPANY_INFO},
};

void Init_EEPROM_Content(void)
{
    uint8_t i;

    /* Get each FRU table attribute to EEPROM */
    for(i = 0; i < EEPROM_FRU_TABLE_TOTAL_COUNT; i++)
    {
        Read_Produce_Info((uint8_t *)(EEPROM_Table + FRU_Data_Attr[i].Offset),
                          FRU_Data_Attr[i].Flash_Addr,
                          FRU_Data_Attr[i].Size
                         );
    }
}

void Read_Produce_Info(uint8_t *des_data, uint32_t src_data_addr, uint8_t size)
{
    uint8_t i, j;
    uint32_t temp;

    /* Unlock write-protected registers */
    SYS_UnlockReg();

    /* Enable FMC ISP function */
    RMC_Open();

    for(i = 0; i < size;)
    {
        /* Read data from Flash */
        temp = RMC_Read(src_data_addr + i);
        for(j = 0; j < 4; j++)
        {
            des_data[i++] = (uint8_t)((temp & (0xFF << (j*8))) >> (j*8));

            /* Finish read */
            if(i == size)
            {
                /* Lock protected registers */
                SYS_LockReg();

                return;
            }
        }
    }

    /* Lock protected registers */
    SYS_LockReg();
}


int32_t RMC_Erase_JC(uint32_t u32PageAddr)
{
    int   idx;
    uint32_t  tout, u32Len, u32Addr;

    /* Workaround solution: Check ISPADDR to know if wakeup from power-down mode.
       If Magic Number exists, call Read CID command to avoid issue 2.5 (Please refer to Errata Sheet)
     */
    if(RMC_CHECK_MAGICNUM())
        RMC_DummyReadCID();

    g_RMC_i32ErrCode = 0;

    u32Addr = u32PageAddr;
    
    if((u32Addr % 256) != 0)
        return -2;

    if (u32Addr < RMC_APROM_END)
    {
        if((u32Addr + FMC_FLASH_PAGE_SIZE) > RMC_APROM_END)
            return -2;
    }
    else if ((u32Addr >= RMC_LDROM_BASE) && (u32Addr < RMC_LDROM_END))
    {
        if((u32Addr + FMC_FLASH_PAGE_SIZE) > RMC_LDROM_END)
            return -2;
    }
    else
        return -2;

    while(u32Addr < u32PageAddr + FMC_FLASH_PAGE_SIZE)
    {
        u32Len = RMC_MULTI_WORD_PROG_MAX_LEN;
        RMC->ISPCTL = RMC->ISPCTL | RMC_ISPCTL_MPEN_Msk; 
        RMC->ISPCMD = RMC_ISPCMD_CLEAR_DATA_BUFFER;
        RMC->ISPADDR = 0x00000000;
        RMC->ISPTRG = RMC_ISPTRG_ISPGO_Msk;
        tout = RMC_TIMEOUT_WRITE;

        while ((--tout > 0) && (RMC->ISPTRG & RMC_ISPTRG_ISPGO_Msk)) {}

        if (tout == 0)
            goto erase_fail;

        if (RMC->ISPSTS & RMC_ISPSTS_ISPFF_Msk)
        {
            RMC->ISPSTS |= RMC_ISPSTS_ISPFF_Msk;
            goto erase_fail;
        }
        idx = 0;
        while (u32Len > 0)
        {
            RMC->ISPCMD = RMC_ISPCMD_LOAD_DATA_BUFFER;
            RMC->ISPADDR = u32Addr + idx * 4;
            RMC->ISPDAT = 0xFFFFFFFF;
            RMC->MPDAT1 = 0xFFFFFFFF;
            RMC->ISPTRG = RMC_ISPTRG_ISPGO_Msk;
            idx += 2;
            tout = RMC_TIMEOUT_WRITE;

            while ((--tout > 0) && (RMC->ISPTRG & RMC_ISPTRG_ISPGO_Msk)) {}
            
            if (tout == 0)
                goto erase_fail;

            if (RMC->ISPSTS & RMC_ISPSTS_ISPFF_Msk)
            {
                RMC->ISPSTS |= RMC_ISPSTS_ISPFF_Msk;
                goto erase_fail;
            }
            u32Len -= 8;
        }

        RMC->ISPCMD = RMC_ISPCMD_PROGRAM;
        RMC->ISPADDR = u32Addr;
        RMC->ISPDAT = 0xFFFFFFFF;
        RMC->ISPTRG = RMC_ISPTRG_ISPGO_Msk;
        tout = RMC_TIMEOUT_WRITE;

        while ((--tout > 0) && (RMC->ISPTRG & RMC_ISPTRG_ISPGO_Msk)) {}

        if (tout == 0)
            goto erase_fail;

        if (RMC->ISPSTS & RMC_ISPSTS_ISPFF_Msk)
        {
            RMC->ISPSTS |= RMC_ISPSTS_ISPFF_Msk;
            goto erase_fail;
        }
        u32Addr = u32Addr + RMC_MULTI_WORD_PROG_MAX_LEN;
    }
    RMC->ISPCTL = RMC->ISPCTL & ~RMC_ISPCTL_MPEN_Msk; 
    return 0;
erase_fail:
    g_RMC_i32ErrCode = -1;
    RMC->ISPCTL = RMC->ISPCTL & ~RMC_ISPCTL_MPEN_Msk; 

    RMC->ISPCMD = RMC_ISPCMD_CLEAR_DATA_BUFFER;
    RMC->ISPADDR = 0x00000000;
    RMC->ISPTRG = RMC_ISPTRG_ISPGO_Msk;
    tout = RMC_TIMEOUT_WRITE;

    while ((--tout > 0) && (RMC->ISPTRG & RMC_ISPTRG_ISPGO_Msk)) {}

    if (RMC->ISPSTS & RMC_ISPSTS_ISPFF_Msk)
    {
        RMC->ISPSTS |= RMC_ISPSTS_ISPFF_Msk;
    }
    return -1;
}


void Update_FRU(uint32_t addr, uint8_t *data, uint8_t size)
{
    uint32_t i, j;
    uint32_t temp;

    /* Unlock write-protected registers */
    SYS_UnlockReg();

    /* Enable FMC ISP function */
    RMC_Open();

    /* Enable update APROM */
    RMC_ENABLE_AP_UPDATE();

    /* Erase target page */
    RMC_Erase_JC(addr);

    /* Write new data */
    for(i = 0; i < size;)
    {
        temp = 0;

        for(j = 0; j < 4; j++)
        {
            temp |= (data[i++] << (j*8));

            if(i == size)
            {
                break;
            }
        }

        /* Write to the Flash */
        RMC_Write(addr, temp);
        /* Add target address */
        addr += 4;
    }

    /* Disable update APROM */
    RMC_DISABLE_AP_UPDATE();

    /* Lock protected registers */
    SYS_LockReg();
}

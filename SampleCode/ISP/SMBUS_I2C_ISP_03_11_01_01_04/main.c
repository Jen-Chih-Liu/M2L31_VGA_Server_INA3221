/***************************************************************************//**
 * @file     main.c
 * @brief    ISP tool main function
 * @version  0x32
 * @date     14, June, 2017
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2017-2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "targetdev.h"
#include "i2c_transfer.h"
#include <string.h>

//03_10_00_00_02 init release
//14_10_00_01_03 aprom size modify 0xe00
//03_11_01_01_04 porting to m2l31
int32_t  g_RMC_i32ErrCode;
volatile uint32_t  cksum = 0, code_check_flash = 0;
#define aprom_size 0xe000  //20251014 updated offset address
//#define aprom_size 0xEa00
#define g_ckbase  (aprom_size - 4)
uint32_t Pclk0;
uint32_t Pclk1;
extern volatile uint8_t u8eraseflashflag;
extern volatile uint8_t i2c_ack_data;
extern volatile uint8_t u8JMPAPflag;
extern volatile uint8_t u8PROGAPflag;
volatile uint32_t StartAddress;
__ALIGNED(4) volatile uint8_t Write_buff[32];
__ALIGNED(4) volatile uint8_t Read_buff[32];
/* This is a dummy implementation to replace the same function in clk.c for size limitation. */
uint32_t CLK_GetPLLClockFreq(void)
{
    return FREQ_48MHZ;
}

void SYS_Init(void)
{
    /* Unlock write-protected registers to operate SYS_Init and FMC ISP function */
    SYS_UnlockReg();

    /* Enable Internal RC clock */
    CLK->PWRCTL |= CLK_PWRCTL_HIRC48MEN_Msk;

    /* Waiting for external XTAL clock ready */
    while (!(CLK->STATUS & CLK_STATUS_HIRC48MSTB_Msk));

    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLK0SEL_Msk)) | CLK_CLKSEL0_HCLKSEL_HIRC48M;
    CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_HCLK0DIV_Msk)) | CLK_CLKDIV0_HCLK(1);

	    /* Switch RMC access cycle to suitable value base on HCLK */
    RMC->CYCCTL = 3;
	
    /* Set both PCLK0 and PCLK1 as HCLK/2 */
    CLK->PCLKDIV = CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2;
    SystemCoreClock = __HIRC48;
    CyclesPerUs = (SystemCoreClock + 500000) / 1000000;
    Pclk0           = SystemCoreClock / 2;
    Pclk1           = SystemCoreClock / 2;
    /* Enable PA clock */
    CLK->AHBCLK0 |=  CLK_AHBCLK0_GPACKEN_Msk;

    /* Enable I2C1 clock */

    CLK->APBCLK0 |= CLK_APBCLK0_I2C1CKEN_Msk;

    /* Set PA multi-function pins for I2C1 SDA and SCL */
    SYS->GPA_MFP0 = (SYS->GPA_MFP0 & ~(SYS_GPA_MFP0_PA2MFP_Msk | SYS_GPA_MFP0_PA3MFP_Msk)) | \
                    (SYS_GPA_MFP0_PA2MFP_I2C1_SDA | SYS_GPA_MFP0_PA3MFP_I2C1_SCL);

SYS->UTCPDCTL|=0x03; //disable dead battery for pc0, pc1
    /* I2C clock pin enable schmitt trigger */
    PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk | GPIO_SMTEN_SMTEN3_Msk;

}
__attribute__((aligned(4))) static uint8_t aprom_buf[FMC_FLASH_PAGE_SIZE];
static uint16_t Checksum(unsigned char *buf, int len)
{
    int i;
    uint16_t c;

    for (c = 0, i = 0; i < len; i++)
    {
        c += buf[i];
    }

    return (c);
}
static uint16_t CalCheckSum(uint32_t start, uint32_t len)
{
    int i;
    uint16_t lcksum = 0;

    for (i = 0; i < len; i += FMC_FLASH_PAGE_SIZE)
    {
        ReadData(start + i, start + i + FMC_FLASH_PAGE_SIZE, (uint32_t *)aprom_buf);

        if (len - i >= FMC_FLASH_PAGE_SIZE)
            lcksum += Checksum(aprom_buf, FMC_FLASH_PAGE_SIZE);
        else
            lcksum += Checksum(aprom_buf, len - i);
    }

    return lcksum;

}
int EraseAP_jc(uint32_t addr_start, uint32_t size)
{
	 uint32_t StartAddress, TotalLen;
 memset((unsigned int *)aprom_buf,0xFF,FMC_FLASH_PAGE_SIZE);
            TotalLen = g_apromSize;
            StartAddress = 0;
            do {
                if (WriteData(StartAddress, StartAddress+ FMC_FLASH_PAGE_SIZE, (uint32_t *)&aprom_buf)!=0)
									 return -1;
                TotalLen -= FMC_FLASH_PAGE_SIZE;
                StartAddress += FMC_FLASH_PAGE_SIZE;
            } while ( TotalLen >0); 
						return 0;
}		

/*
 *  Please check default I2C1 multi-function pins in SYS_Init() are available in target chip
 *
 */
int main(void)
{
    uint32_t i;
    //uint32_t cmd_buff[16];
    SYS_Init();

    CLK->AHBCLK0 |= CLK_AHBCLK0_ISPCKEN_Msk;
    RMC->ISPCTL |= (RMC_ISPCTL_ISPEN_Msk | RMC_ISPCTL_APUEN_Msk);
    g_apromSize = aprom_size;
    //g_apromSize = GetApromSize();
    //GetDataFlashInfo(&g_dataFlashAddr, &g_dataFlashSize);
    RMC_Read_User(g_ckbase, (uint32_t *)&cksum);
    g_dataFlashAddr = aprom_size;
    g_dataFlashSize = 0;
    I2C_Init();

    //SysTick->LOAD = 300000 * CyclesPerUs;
    //SysTick->VAL  = (0x00);
    //SysTick->CTRL = SysTick->CTRL | SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
#if 1

if((SYS->RSTSTS & SYS_RSTSTS_CPURF_Msk) == SYS_RSTSTS_CPURF_Msk)
    {
        SYS->RSTSTS &= SYS->RSTSTS;
        goto _LDROM_LOOP;
    }

#endif
    code_check_flash = CalCheckSum(0x0, g_ckbase);

    if (code_check_flash == (cksum & 0xffff))
    {
        goto _APROM;
    }

#if 0

    while (1)
    {
        if (bI2cDataReady == 1)
        {
            goto _ISP;
        }

        if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
        {
            goto _APROM;
        }
    }

#endif
_LDROM_LOOP:

    while (1)
    {

        if (u8eraseflashflag == 1)
        {
            if (EraseAP_jc(RMC_APROM_BASE, g_apromSize) == 0)
            {
                StartAddress = 0;
                i2c_ack_data = 0xaa; //ok
            }
            else
            {
                i2c_ack_data = 0xff; //false
            }

            u8eraseflashflag = 0;
        }

        if (u8JMPAPflag == 1)
        {
            u8JMPAPflag = 0;
            goto _APROM;
        }

        if (u8PROGAPflag == 1)
        {
            u8PROGAPflag = 0;
            WriteData(StartAddress, StartAddress + 32, (uint32_t *)Write_buff); //WriteData(StartAddress, StartAddress + srclen, (uint32_t*)pSrc);
            ReadData(StartAddress, StartAddress + 32, (uint32_t *)Read_buff);
            StartAddress += 32;

            for (i = 0; i < 32; i++)
            {
                if (Write_buff[i] != Read_buff[i])
                {
                    break;
                }
            }

            if (i == 32)
            {
                i2c_ack_data = 0xaa; //ok
            }
            else
            {
                i2c_ack_data = 0xff; //ok
            }
        }

#if 0

        if (bI2cDataReady == 1)
        {

            memcpy(cmd_buff, i2c_rcvbuf, 64);
            bI2cDataReady = 0;
            ParseCmd((unsigned char *)cmd_buff, 64);
        }

#endif
    }

_APROM:
    SYS->RSTSTS = (SYS_RSTSTS_PORF_Msk | SYS_RSTSTS_PINRF_Msk);//clear bit
		__set_PRIMASK(1);
    RMC_SetVectorPageAddr(RMC_APROM_BASE);
    RMC_SET_APROM_BOOT();
    NVIC_SystemReset();

    /* Trap the CPU */
    while (1);
}

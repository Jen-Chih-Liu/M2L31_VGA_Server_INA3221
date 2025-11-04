/******************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * @brief
 *           Demonstrate how to control Gen1 ARGB LED lighting.
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "device.h"
#include "NuMicro.h"
#include "Flash.h"
#include "I2C_Control.h"
#include "Monitor_Control.h"

// 2025/09/17 v25_09_00_00_01    First released
// 2025/09/17 v25_09_00_00_02    Changed shunt resistor value
// 2025/09/20 v25_09_00_00_03    Changed all FRU table to be programmable
// 2025/09/26 v26_09_00_00_05    smbus read 0xf0-0xff
// 2025/10/03 v03_10_00_00_06    ARPOM offset 0xc0 read version. isp software jumper
// 2025/10/21 v21_10_00_01_07    data offset 0xe000 and  add company informaiton
// 2025/11/03 v03_11_01_01_08    porting to m2l31
/*--------------------------------------------------------------------------*/
void SYS_Init(void)
{
    uint32_t Temp;

    /* Clear reset flag for safety */
   // SYS->RSTSTS &= SYS->RSTSTS;

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable HIRC clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRC48MEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRC48MSTB_Msk);

    /* Set core clock to HCLK_CLK MHz */
    CLK_SetCoreClock(HCLK_CLK);

    /* Update the Variable SystemCoreClock */
    SystemCoreClockUpdate();
}

void SysTick_Initial(void)
{
    /* Set SysTick clock source to HCLK */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk;

    /* Set load value to make SysTick period is 1 ms */
    SysTick->LOAD = HCLK_CLK / 1000;

    /* Clear SysTick counter */
    SysTick->VAL = 0;

    /* Enable SysTick interrupt */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

    /* Enable SysTick NVIC */
    NVIC_EnableIRQ(SysTick_IRQn);
    /* Change interrupt priority to normal */
    NVIC_SetPriority(SysTick_IRQn, INT_PRIORITY_NORMAL);
}

void SysTick_Handler(void)
{
   uint8_t i;

    /* Clear interrupt flag */
    SysTick->VAL = 0;

    /* Get monitor data */
    TimeCounterMonitorUpdate++;
#if ((USE_MONITOR_0 == TRUE) | (USE_MONITOR_1 == TRUE))
    if(TimeCounterMonitorUpdate >= TIMER_MONITOR_UPDATE)
    {
        u8MonitorFlag = 1;
        TimeCounterMonitorUpdate -= TIMER_MONITOR_UPDATE;
    }
#endif
}

uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp)
{
    /* It is casued by hardfault. Just process the hard fault */
    /* TODO: Implement your hardfault handle code here */
    while(1);

    return lr;
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
    uint32_t i, j;
    uint8_t target_port;

    /* Unlock write-protected registers */
    SYS_UnlockReg();

    /* Init system and multi-funcition I/O */
    SYS_Init();
SYS->UTCPDCTL|=0x03; //disable dead battery for pc0, pc1
    /* Lock protected registers */
    SYS_LockReg();

    /* Init EEPROM content */
    Init_EEPROM_Content();

    /* Init I2C for communication */
    I2C1_Init();

#if (USE_MONITOR_0 == TRUE)
    /* Init I2C0 for get monitor data */
    I2C0_Init();
#endif

#if (USE_MONITOR_1 == TRUE)
    /* Init UI2C1 for get monitor data */
    UI2C1_Init();
#endif

    /* Initial SysTick, enable interrupt and 1000 interrupt tick per second to add counter */
    SysTick_Initial();

    /* Start SysTick */
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    while (1)
    {
        /* Check if get monitor data */
        if(u8MonitorFlag == 1)
        {
#if (USE_MONITOR_0 == TRUE)
            /* Get monitor data */
            Read_Monitor_Data_0();
#endif

#if (USE_MONITOR_1 == TRUE)
            /* Get monitor data */
            Read_Monitor_Data_1();
#endif

            /* Clear flag */
            u8MonitorFlag = 0;
        }

        /* Check if update FRU data */
        if(u8UpdateFRUDataFlag == 1)
        {
            /* Update FRU data */
            Update_FRU(u32UpdateTargetAddress, au8UpdateData, u8UpdateTargetSize);

            /* Update current FRU table */
            Read_Produce_Info((uint8_t *)(EEPROM_Table + u8UpdateTargetOffset),
                              u32UpdateTargetAddress,
                              u8UpdateTargetSize
                             );

            /* Clear flag */
            u8UpdateFRUDataFlag = 0;
        }
				if (u8UpdateISPFlag== 1)
            {
						 SYS_UnlockReg();
							SYS->RSTSTS &= SYS->RSTSTS;
            RMC_Open();
            RMC_SetBootSource(1);          // Boot from LDROM
            NVIC->ICPR[0] = 0xFFFFFFFF;    // Clear Pending Interrupt
            /* Set VECMAP to LDROM for booting from LDROM */
            RMC_SetVectorPageAddr(RMC_LDROM_BASE);
            SYS_ResetCPU();
 
            while(1);
		
						}
				
    }
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/

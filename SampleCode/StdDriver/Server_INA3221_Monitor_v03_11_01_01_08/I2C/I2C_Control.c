 /******************************************************************************//**
 * @file     I2C_Control.c
 * @version  V1.00
 * @brief    I2C_Control sample file
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

 /*!<Includes */
#include "device.h"
#include "NuMicro.h"
#include "I2C_Control.h"
#include "Monitor_Control.h"
#include "Flash.h"
#define fw_version 0x08
/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
volatile uint16_t u16SlvDataLen;
uint8_t au8SlvRxData[LEN_MAX_I2C_DATA];
volatile uint8_t u8RxPtr = 0;
volatile uint8_t u8ReportEEPROMFlag = 0;
volatile uint8_t u8ReportMonitorFlag = 0;
uint8_t au8UpdateData[LEN_MAX_I2C_DATA];
volatile uint8_t u8UpdateFRUDataFlag = 0;
volatile uint32_t u32UpdateTargetAddress = 0;
volatile uint8_t u8UpdateTargetOffset = 0;
volatile uint8_t u8UpdateTargetSize = 0;
volatile uint8_t u8UpdateISPFlag=0;
uint8_t u8RxLen = 0;

/* Report Power Information */
const uint8_t CMD_Get_Power_Info[CMD_LEN_GET_POWER_INFO]
= {0xDC, 0x03, 0x07, 0x80, 0x00
  };

	const uint8_t CMD_ISP[CMD_LEN_ISP] //jmp to ldrom
= {0x4E, 0X05, 0x4A, 0x4D, 0x50, 0x4C, 0x44
  };
	
uint8_t Data_Get_Power_Info[DATA_LEN_GET_POWER_INFO]
= {/* Fixed Header */
   0x0F, 0x07, 0x1F, 0x00,
   /* Volatge and Current data, */
   0x00, 0x00,    // Voltage 1, unit in 10 mV
   0x00, 0x00,    // Current 1, unit in 10 mA
   0x00, 0x00,    // Voltage 2, unit in 10 mV
   0x00, 0x00,    // Current 2, unit in 10 mA
   0x00, 0x00,    // Voltage 3, unit in 10 mV
   0x00, 0x00,    // Current 4, unit in 10 mA
  };

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C1 IRQ Handler                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void I2C1_IRQHandler(void)
{
    uint32_t u32Status;

    u32Status = I2C_GET_STATUS(I2C1);

    if(I2C_GET_TIMEOUT_FLAG(I2C1))
    {
        /* Clear I2C1 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C1);

        I2C_Close(I2C1);
        I2C1_Init();
    }
    else
    {
        if(s_I2C1HandlerFn != NULL)
            s_I2C1HandlerFn(I2C1, u32Status);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C TRx Callback Function                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_SlaveTRx(I2C_T *i2c, uint32_t u32Status)
{
    uint8_t i;
    uint8_t u8Data;

    if(u32Status == 0x60)                      /* Own SLA+W has been receive; ACK has been return */
    {
        u16SlvDataLen = 0;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0x80)                 /* Previously address with own SLA address
                                                  Data has been received; ACK has been returned*/
    {
        u8Data = (unsigned char) I2C_GET_DATA(i2c);

        if(u16SlvDataLen < LEN_MAX_I2C_DATA)
        {
            au8SlvRxData[u16SlvDataLen++] = u8Data;
        }

        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0xA8)                 /* Own SLA+R has been receive; ACK has been return */
    {
        /* Request FRU table */
        if(u8ReportEEPROMFlag == 1)
        {
						if((u8RxPtr >= 0xF0) && (u8RxPtr <= 0xFF))
								I2C_SET_DATA(i2c, Data_Get_Power_Info[u8RxPtr++ - 0xF0]);
						else if(u8RxPtr == 0xc0)
								I2C_SET_DATA(i2c, fw_version); //fw version
						else
								I2C_SET_DATA(i2c, EEPROM_Table[u8RxPtr++]);
        }
        /* Request Power Information */
        else if(u8ReportMonitorFlag == 2)
        {
            I2C_SET_DATA(i2c, Data_Get_Power_Info[u8RxPtr++]);
        }
        else
        {
            I2C_SET_DATA(i2c, 0x00);
        }

        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0xB8)                 /* Data byte in I2CDAT has been transmitted ACK has been received */
    {
        /* Request FRU table */
        if(u8ReportEEPROMFlag == 1)
        {
					  
            if(u8RxPtr < EEPROM_MAX_VALID_OFFSET)
            {
								
								I2C_SET_DATA(i2c, EEPROM_Table[u8RxPtr++]);
            }
						else if(u8RxPtr==0xc0)
						{
						I2C_SET_DATA(i2c, 0x07); //fw version
						}
						else if((u8RxPtr >= 0xF0) && (u8RxPtr <= 0xFF))
						{	
							I2C_SET_DATA(i2c, Data_Get_Power_Info[u8RxPtr++ - 0xF0]);
						}	
							else
            
            {
                I2C_SET_DATA(i2c, 0x00);
            }
        }
        /* Request Power Information */
        else if(u8ReportMonitorFlag == 2)
        {
            if(u8RxPtr < DATA_LEN_GET_POWER_INFO)
            {
                I2C_SET_DATA(i2c, Data_Get_Power_Info[u8RxPtr++]);
            }
            else
            {
                I2C_SET_DATA(i2c, 0x00);
            }
        }
        else
        {
            I2C_SET_DATA(i2c, 0x00);
        }

        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0xC0)                 /* Data byte or last data in I2CDAT has been transmitted
                                                  Not ACK has been received */
    {
        /* Clear flag */
        u8ReportEEPROMFlag = u8ReportMonitorFlag = 0;

        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0x88)                 /* Previously addressed with own SLA address; NOT ACK has
                                                  been returned */
    {
        u16SlvDataLen = 0;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else if(u32Status == 0xA0)                 /* A STOP or repeated START has been received while still
                                                  addressed as Slave/Receiver*/
    {
			
        /* Request FRU table */
        if((u16SlvDataLen == 1) &&
           (au8SlvRxData[0] < EEPROM_MAX_VALID_OFFSET)
          )
        {
            /* Set pointer */
            u8RxPtr = au8SlvRxData[0];
            /* Set flag */
            u8ReportEEPROMFlag = 1;
            /* Clear flag */
            u8ReportMonitorFlag = 0;
        }
				else if((u16SlvDataLen == 1) &&
           (au8SlvRxData[0] ==0xc0)
          )
        {
			      u8RxPtr = au8SlvRxData[0];
            /* Set flag */
            u8ReportEEPROMFlag = 1;
            /* Clear flag */
            u8ReportMonitorFlag = 0;
				}
			
		else if((u16SlvDataLen == 1) &&
					(au8SlvRxData[0] >= 0xF0) && (au8SlvRxData[0] <= 0xFF))
          
        {
            /* Set pointer */
            u8RxPtr = au8SlvRxData[0];
            /* Set flag */
            u8ReportEEPROMFlag = 1;
            /* Clear flag */
            u8ReportMonitorFlag = 0;
        }
				//ISP COMMAND
				 else if((u16SlvDataLen == CMD_LEN_ISP) &&
                (au8SlvRxData[0] == CMD_ISP[0])
               )
        {
            /* Check if get correct command */
            for(i = 0; i < CMD_LEN_ISP; i++)
            {
                if(au8SlvRxData[i] != CMD_ISP[i])
                {
                    break;
                }
            }
            /* Get correct command */
            if(i == CMD_LEN_ISP)
            {
                /* Set flag */
                u8UpdateISPFlag = 1;
            }

            /* Clear flag */
            u8ReportEEPROMFlag = 0;
        }
				
        /* Write Request Power Information Command */
        else if((u16SlvDataLen == CMD_LEN_GET_POWER_INFO) &&
                ((au8SlvRxData[0] == CMD_Get_Power_Info[0]))
               )
        {
            /* Check if get correct command */
            for(i = 0; i < CMD_LEN_GET_POWER_INFO; i++)
            {
                if(au8SlvRxData[i] != CMD_Get_Power_Info[i])
                {
                    break;
                }
            }
            /* Get correct command */
            if(i == CMD_LEN_GET_POWER_INFO)
            {
                /* Set pointer */
                u8RxPtr = 0;
                /* Set flag */
                u8ReportMonitorFlag = 1;
            }

            /* Clear flag */
            u8ReportEEPROMFlag = 0;
        }
        /* Read Request Power Information */
        else if((u8ReportMonitorFlag == 1) &&
                (u16SlvDataLen == 1) &&
                (au8SlvRxData[0] == CMD_OFFSET_GET_POWER_INFO)
               )
        {
            /* Set pointer */
            u8RxPtr = 0;
            /* Set flag */
            u8ReportMonitorFlag = 2;

            /* Clear flag */
            u8ReportEEPROMFlag = 0;
        }
        /* Update FRU Serial Number */
        else if((au8SlvRxData[0] > CMD_OFFSET_UPDATE_START) &&
                (u16SlvDataLen == (2 + FRU_Data_Attr[au8SlvRxData[0] - CMD_OFFSET_UPDATE_START].Size))
               )
        {
            /* Copy update data */
            for(i = 0; i < (FRU_Data_Attr[au8SlvRxData[0] - CMD_OFFSET_UPDATE_START].Size); i++)
            {
                au8UpdateData[i] = au8SlvRxData[2 + i];
            }

            /* Copy update target */
            u32UpdateTargetAddress = FRU_Data_Attr[au8SlvRxData[0] - CMD_OFFSET_UPDATE_START].Flash_Addr;
            u8UpdateTargetOffset = FRU_Data_Attr[au8SlvRxData[0] - CMD_OFFSET_UPDATE_START].Offset;
            u8UpdateTargetSize = FRU_Data_Attr[au8SlvRxData[0] - CMD_OFFSET_UPDATE_START].Size;

            /* Set flag */
            u8UpdateFRUDataFlag = 1;

            /* Clear flag */
            u8ReportEEPROMFlag = u8ReportMonitorFlag = 0;
        }
        else
        {
            /* Clear flag */
            u8ReportEEPROMFlag = u8ReportMonitorFlag = 0;
        }

        u16SlvDataLen = 0;
        I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
    }
    else
    {
        if(u32Status == 0x68)              /* Slave receive arbitration lost, clear SI */
        {
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
        }
        else if(u32Status == 0xB0)         /* Address transmit arbitration lost, clear SI  */
        {
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI_AA);
        }
        else                               /* Slave bus error, stop I2C and clear SI */
        {
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_STO_SI);
            I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
        }
    }
}

void I2C1_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable I2C1 module clock */
    CLK_EnableModuleClock(I2C1_MODULE);
   /* Enable GPIO clock */
    CLK_EnableModuleClock(GPA_MODULE);
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PA multi-function pins for I2C1 SDA and SCL */
    SYS->GPA_MFP0 = (SYS->GPA_MFP0 & ~(SYS_GPA_MFP0_PA2MFP_Msk | SYS_GPA_MFP0_PA3MFP_Msk)) | \
                    (SYS_GPA_MFP0_PA2MFP_I2C1_SDA | SYS_GPA_MFP0_PA3MFP_I2C1_SCL);
    PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk | GPIO_SMTEN_SMTEN3_Msk;
    /* Open I2C module and set bus clock */
    I2C_Open(I2C1, SPEED_I2C_BUS);

    /* Set I2C Slave Addresses */
    I2C_SetSlaveAddr(I2C1, 0, ADDRESS_I2C_SLAVE_7BIT, I2C_GCMODE_DISABLE);

    /* Enable I2C interrupt */
    I2C_EnableInt(I2C1);
    NVIC_EnableIRQ(I2C1_IRQn);
    NVIC_SetPriority(I2C1_IRQn, INT_PRIORITY_HIGH);

    /* Add hold time */
//    I2C1->TMCTL = 0x02 << I2C_TMCTL_HTCTL_Pos;

    /* Lock protected registers */
    SYS_LockReg();

    /* I2C function to Slave receive/transmit data */
    s_I2C1HandlerFn = I2C_SlaveTRx;

    /* I2C enter no address SLV mode */
    I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI_AA);
}

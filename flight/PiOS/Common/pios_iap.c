/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_IAP IAP Functions
 * @brief STM32F4xx Hardware dependent I2C functionality
 * @{
 *
 * @file       pios_iap.c  
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      In application programming functions
 * @see        The GNU Public License (GPL) Version 3
 * 
 *****************************************************************************/
/* 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/****************************************************************************************
 *  Header files
 ****************************************************************************************/
#include <pios.h>

/****************************************************************************************
 *  Private Definitions/Macros
 ****************************************************************************************/
#if defined(STM32F4XX)
#define ReadBackupRegister RTC_ReadBackupRegister
#define WriteBackupRegister RTC_WriteBackupRegister
#elif defined(STM32F10X_MD) || defined(STM32F10X_HD)
#define ReadBackupRegister BKP_ReadBackupRegister
#define WriteBackupRegister BKP_WriteBackupRegister
#else
#error non supported architecture for pios_IAP
#endif
/* these definitions reside here for protection and privacy. */
#define IAP_MAGIC_WORD_1	0x1122
#define IAP_MAGIC_WORD_2	0xAA55

#define UPPERWORD16(lw)	(uint16_t)((uint32_t)(lw)>>16)
#define LOWERWORD16(lw)	(uint16_t)((uint32_t)(lw)&0x0000ffff)
#define UPPERBYTE(w)	(uint8_t)((w)>>8)
#define LOWERBYTE(w)	(uint8_t)((w)&0x00ff)

/****************************************************************************************
 *  Private Functions
 ****************************************************************************************/

/****************************************************************************************
 *  Private (static) Data
 ****************************************************************************************/
const uint16_t pios_iap_cmd_list[] = 
        { 
                IAP_CMD1, 
                IAP_CMD2, 
                IAP_CMD3 
        };
/****************************************************************************************
 *  Public/Global Data
 ****************************************************************************************/

/*!
 * \brief	PIOS_IAP_Init - performs required initializations for iap module.
 * \param   none.
 * \return	none.
 * \retval	none.
 *
 *	Created: Sep 8, 2010 10:10:48 PM by joe
 */
void PIOS_IAP_Init( void )
{

#if  defined(STM32F10X_MD) || defined(STM32F10X_HD)
	/* Enable CRC clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);

	/* Enable PWR and BKP clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

	/* Enable write access to Backup domain */
	PWR_BackupAccessCmd(ENABLE);

	/* Clear Tamper pin Event(TE) pending flag */
	BKP_ClearFlag();
#elif defined (STM32F4XX)
	/* Enable CRC clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC | RCC_AHB1Periph_BKPSRAM, ENABLE);

	/* Enable PWR and BKP clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* Enable write access to Backup domain */
	PWR_BackupAccessCmd(ENABLE);

	/* Clear Tamper pin Event(TE) pending flag */
	RTC_ClearFlag(RTC_FLAG_TAMP1F);
#else
#error non supported architecture for pios_IAP
#endif
}

/*!
 * \brief     Determines if an In-Application-Programming request has been made.
 * \param   *comm - Which communication stream to use for the IAP (USB, Telemetry, I2C, SPI, etc)
 * \return    TRUE - if correct sequence found, along with 'comm' updated.
 * 			FALSE - Note that 'comm' will have an invalid comm identifier.
 * \retval
 *
 */
uint32_t	PIOS_IAP_CheckRequest( void )
{
	uint32_t	retval = false;
	uint16_t	reg1;
	uint16_t	reg2;

	reg1 = ReadBackupRegister( MAGIC_REG_1 );
	reg2 = ReadBackupRegister( MAGIC_REG_2 );

	if( reg1 == IAP_MAGIC_WORD_1 && reg2 == IAP_MAGIC_WORD_2 ) {
		// We have a match.
		retval = true;
	} else {
		retval = false;
	}
	return retval;
}



/*!
 * \brief   Sets the 1st word of the request sequence.
 * \param   n/a
 * \return  n/a
 * \retval
 */
void	PIOS_IAP_SetRequest1(void)
{
	WriteBackupRegister( MAGIC_REG_1, IAP_MAGIC_WORD_1);
}

void	PIOS_IAP_SetRequest2(void)
{
	WriteBackupRegister( MAGIC_REG_2, IAP_MAGIC_WORD_2);
}

void	PIOS_IAP_ClearRequest(void)
{
	WriteBackupRegister( MAGIC_REG_1, 0);
	WriteBackupRegister( MAGIC_REG_2, 0);
}

uint16_t PIOS_IAP_ReadBootCount(void)
{
	return ReadBackupRegister ( IAP_BOOTCOUNT );
}

void PIOS_IAP_WriteBootCount (uint16_t boot_count)
{
	WriteBackupRegister ( IAP_BOOTCOUNT, boot_count );
}

/**
  * @brief  Return one of the IAP command values passed from bootloader.
  * @param  number: the index of the command value (0..2).
  * @retval the selected command value.
  */
uint32_t PIOS_IAP_ReadBootCmd(uint8_t number)
{
	if(PIOS_IAP_CMD_COUNT < number)
	{
		PIOS_Assert(0);
	}
	else
	{
		return ReadBackupRegister(pios_iap_cmd_list[number]);
	}	
}

/**
  * @brief  Write one of the IAP command values to be passed to firmware from bootloader.
  * @param  number: the index of the command value (0..2).
  * @param  value: value to be written.
  */
void PIOS_IAP_WriteBootCmd(uint8_t number, uint32_t value)
{
	if(PIOS_IAP_CMD_COUNT < number)
	{
		PIOS_Assert(0);
	}
	else
	{
		WriteBackupRegister(pios_iap_cmd_list[number], value);
	}	
}
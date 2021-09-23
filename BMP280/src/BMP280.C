/*
 * BMO280.C
 *
 *  Created on: Sep 16, 2021
 *      Author: Bruno Otávio
 */
#include "BMP280.h"
#include <stdint.h>

#define BMP280_ADDR 0x58
#define TEMP_XLSB 0xFC
#define TEMP_LSB 0xFB
#define TEMP_MSB 0xFA
#define PRESS_XLSB 0xF9
#define PRESS_LSB 0xF8
#define PRESS_MSB 0xF7
#define CONFIG 0xF5
#define CTRL_MEAS 0xF4
#define STATUS 0xF3
#define RESET 0xE0
#define ID 0xD0
#define FIRST_CALIB 0x88
#define LAST_CALIB 0xA1

I2C_HandleTypeDef bmpI2C;
SPI_HandleTypeDef bmpSPI;

/*
 *	@brief: Configure spi to communicate with BMP
 *	@spix: One @SPI_CHOOSE to use for communication
 *
 */
void BMP_SPI_Init(SPI_CHOOSE spix)
{

	//TODO
}


/*
 * 	@brief: Configure i2c to communicate with BMP
 * 	@i2cx: One @I2C_CHOOSE to use for communication
 *
 */
void BMP_I2C_Init(I2C_CHOOSE i2cx)
{

	bmpI2C.Init.ClockSpeed = 400000;
	bmpI2C.Init.DutyCycle 		= I2C_DUTYCYCLE_2;
	bmpI2C.Init.OwnAddress1 		= 0;
	bmpI2C.Init.AddressingMode	= I2C_ADDRESSINGMODE_7BIT;
	bmpI2C.Init.DualAddressMode 	= I2C_DUALADDRESS_DISABLE;
	bmpI2C.Init.OwnAddress2 		= 0;
	bmpI2C.Init.GeneralCallMode 	= I2C_GENERALCALL_DISABLE;
	bmpI2C.Init.NoStretchMode 	= I2C_NOSTRETCH_DISABLE;

	switch(i2cx)
	{
		case BMP_I2C1:

			bmpI2C.Instance = I2C1;
			break;
		case BMP_I2C2:

			bmpI2C.Instance = I2C2;
			break;
		case BMP_I2C3:

			bmpI2C.Instance = I2C3;
			break;
	}

	HAL_I2C_Init(&bmpI2C);
}

/*
 * @brief: Read a number of bytes from BMP in interrupt mode
 *
 * @param: reg is the firsts BMP register to be read
 * @param: count is the number of register to be read
 * @param: data is a vector pointer where the data will be placed
 *
 */
void __BMP_Read(uint8_t reg, int count,uint8_t *data)
{

	HAL_I2C_Master_Transmit_IT(&bmpI2C, BMP280_ADDR, reg, sizeof(uint8_t));
	HAL_I2C_Master_Receive_IT(&bmpI2C, BMP280_ADDR, data, count);
}

/*
 * @brief: Write a number of bytes into BMP in interrupt mode
 *
 * @param: register to write to
 * @param: data to write into register, the register(i) corresponds to data(i)
 * @param: count is the number of register that must be written
 *
 */
void __BMP_Write(uint8_t *reg, uint8_t *data, uint8_t count)
{

	uint8_t toSend[count][2];

	for(uint8_t i = 0; i<count; i++)
	{
		toSend[i][0] = reg[i];
		toSend[i][1] = data[i];
	}

	HAL_I2C_Master_Transmit_IT(&bmpI2C, BMP280_ADDR, toSend, sizeof(toSend));
}

/*
 * @brief: Return the BMP id, should be 0x58
 *
 *	@retval: One byte representing BMP ID
 */
uint8_t BMP_WhoAmI()
{
	uint8_t toReturn;
	return __BMP_Read(ID, 1, &toReturn);
}

/*
 * @brief: Returns the sensor status, see BMP datasheet for more information
 *
 * @param: measuring: Automatically set to ‘1’ whenever a conversion is running
 *			          and back to ‘0’ when the results have been transferred
 *					  to the data registers.
 * @param: imUpdate: Automatically set to ‘1’ when the NVM data are being
 *					 copied to image registers and back to ‘0’ when the
 *					 copying is done. The data are copied at power-on-reset
 *					 and before every conversion.
 */

void BMP_Status(uint8_t *measuring, uint8_t *imUpdate)
{

	uint8_t toReturn;

	__BMP_Read(STATUS, 1, &toReturn);

	*measuring = (toReturn >> 3) & 0x01;
	*imUpdate = (toReturn) & 0x01;
}

/*
 *
 * @brief: Configuration of BMP operation measurement mode
 *
 * @param tempOverSampling: One @BMP_Oversampling option
 * @param preeOverSampling: One @BMP_Oversampling option
 * @BMP_PWRMode: One @BMP_PWRMode
 */

void BMP_ConfigMeasurement(BMP_Oversampling tempOverSampling, BMP_Oversampling pressOverSampling, BMP_PWRMode pwrMode)
{

	uint8_t toSend = (tempOverSampling) << 5 | pressOverSampling << 2 | pwrMode;
	__BMP_Write(CTRL_MEAS, toSend, 1);
}

/*
 * @brief: Configuration of BMP standby and filter
 *
 * @param normalOpStandBy: One @BMP_StandByTime option
 * @param iirFilter: One @BMP_IIRFIlter
 * @param spiEnable: 1 to enable spi, 0 otherwise
 */
void BMP_Config(BMP_StandByTime normalOpStandBy, BMP_IIRFIlter iirFilter, uint8_t spiEnable)
{
	uint8_t toSend = normalOpStandBy << 5 | iirFilter << 2 | spiEnable;
	__BMP_Write(CONFIG, toSend, 1);
}

/*
 * @brief: Read raw (ADC) and uncompensated pressure sensor data
 *
 * @retval: An int representing pressure output
 */
int32_t BMP_ReadRawPress()
{
	int32_t pressureData;

	int8_t toRead[3];

	__BMP_Read(PRESS_MSB, 3, toRead);
	pressureData = ( (toRead[2] << 12) | (toRead[1] << 4) | (toRead[0] & 0xF0) ) & 0xFFFFF;

	return pressureData;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{

	return;
}




/**
  * @brief  Master Rx Transfer completed callback.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{

	return;
}


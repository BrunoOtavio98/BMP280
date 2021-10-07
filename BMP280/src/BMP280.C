/*
 * BMO280.C
 *
 *  Created on: Sep 16, 2021
 *      Author: Bruno Otávio
 */
#include "BMP280.h"
#include <stdint.h>
#include <stdbool.h>

uint8_t BMP280_ADDR = 0x76;
uint8_t TEMP_XLSB = 0xFC;
uint8_t TEMP_LSB = 0xFB;
uint8_t TEMP_MSB = 0xFA;
uint8_t PRESS_XLSB = 0xF9;
uint8_t PRESS_LSB = 0xF8;
uint8_t PRESS_MSB = 0xF7;
uint8_t CONFIG = 0xF5;
uint8_t CTRL_MEAS = 0xF4;
uint8_t STATUS = 0xF3;
uint8_t RESET_BMP = 0xE0;
uint8_t ID = 0xD0;
uint8_t FIRST_CALIB = 0x88;
uint8_t LAST_CALIB = 0xA1;

static I2C_HandleTypeDef bmpI2C;
static SPI_HandleTypeDef bmpSPI;

static uint8_t pressureCalibrationP1[2];
static int8_t pressureCalibration[16];
static int8_t tempCalibration[4];
static uint8_t tempCalibrationT1[2];
static int t_fine;


static void BMP_ReadCalibrationParam();
static float TemperatureCompensateFormula(int tempRaw);
static float PressureCompesateFormula(int tempPress);

static void DefaultInitialization();

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

	DefaultInitialization();

	BMP_ReadCalibrationParam();
}

static void DefaultInitialization()
{
	BMP_ConfigMeasurement(OVERSAMPLING_BY2, OVERSAMPLING_BY4, NORMAL);
	BMP_Config(T_62dot5, F_COEF4, false);
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

	HAL_I2C_Master_Transmit(&bmpI2C, BMP280_ADDR << 1 , &reg, sizeof(uint8_t), HAL_MAX_DELAY);
	HAL_I2C_Master_Receive(&bmpI2C, BMP280_ADDR << 1, data, count, HAL_MAX_DELAY);
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

	HAL_I2C_Master_Transmit(&bmpI2C, BMP280_ADDR << 1, (uint8_t *)toSend, sizeof(toSend), HAL_MAX_DELAY);
}

/*
 * @brief: Return the BMP id, should be 0x58
 *
 *	@retval: One byte representing BMP ID
 */
uint8_t BMP_WhoAmI()
{
	uint8_t toReturn;
	__BMP_Read(ID, 1, &toReturn);
	return toReturn;
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
	__BMP_Write(&CTRL_MEAS, &toSend, 1);
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
	__BMP_Write(&CONFIG, &toSend, 1);
}

/*
 * @brief: Read (burst) raw (ADC) and uncompensated temperature and pressure data
 *
 * @retval: An int representing pressure output
 */
void BMP_ReadRaw(int *rawPress, int *rawTemp)
{

	int8_t toRead[6];

	__BMP_Read(PRESS_MSB, 6, (uint8_t *)toRead);

	rawPress[0] = toRead[2];
	rawPress[1] = toRead[1];
	rawPress[2] = toRead[0];

	rawTemp[0] = toRead[6];
	rawTemp[1] = toRead[5];
	rawTemp[2] = toRead[4];
}


static void BMP_ReadCalibrationParam()
{

	int toRead[26];

	__BMP_Read(0x88, 26, toRead);

	tempCalibrationT1[0] = (uint8_t)toRead[0];
	tempCalibrationT1[1] = (uint8_t)toRead[1];

	for(int i = 0; i<4; i++)
	{
 		tempCalibration[i] = toRead[i+2];
	}

	pressureCalibrationP1[0] = (uint8_t)toRead[6];
	pressureCalibrationP1[1] = (uint8_t)toRead[7];

	for(int i = 0; i<16; i++)
	{
		pressureCalibration[i] = toRead[i+8];
	}
}


float BMP_ReadTemperature()
{
	int rawTemp[3];
	int dummyPress[3];
	int tempRaw = 0;


	BMP_ReadRaw(dummyPress, rawTemp);

	tempRaw = (rawTemp[0] >> 4) | ( rawTemp[1] << 4 ) | ( rawTemp[2] << 12 );
	return TemperatureCompensateFormula(tempRaw);

}


static float TemperatureCompensateFormula(int tempRaw)
{

	int var1,var2;

	uint16_t dig_T1 = tempCalibrationT1[0] | tempCalibrationT1[1] << 8;
	int16_t dig_T2 = tempCalibration[0] | tempCalibration[1] << 8;
	int16_t dig_T3 = tempCalibration[2] | tempCalibration[3] << 8;

	var1 = ((((tempRaw>>3) - ((int)dig_T1<<1))) * ((int)dig_T2)) >> 11;
	var2 = (((((tempRaw>>4) - ((int)dig_T1)) * ((tempRaw>>4) - ((int)dig_T1))) >> 12) * ((int)dig_T3)) >> 14;
	t_fine = var1 + var2;

	return (t_fine * 5 + 128) >> 8;
}

float BMP_ReadPression()
{

	int rawPress[3];
	int dummyTemp[3];
	int pressRaw = 0;


	BMP_ReadRaw(rawPress, dummyTemp);

	pressRaw = (rawPress[0] >> 4) | (rawPress[1] <<  4) | (rawPress[2] << 12);
	return PressureCompesateFormula(pressRaw);
}

static float PressureCompesateFormula(int tempPress)
{

	int64_t var1,var2,p;

	uint16_t dig_P1 = pressureCalibrationP1[0] | pressureCalibrationP1[1] << 8;
	int16_t dig_P2 = pressureCalibration[0] | pressureCalibration[1] << 8;
	int16_t dig_P3 = pressureCalibration[2] | pressureCalibration[3] << 8;
	int16_t dig_P4 = pressureCalibration[4] | pressureCalibration[5] << 8;
	int16_t dig_P5 = pressureCalibration[6] | pressureCalibration[7] << 8;
	int16_t dig_P6 = pressureCalibration[8] | pressureCalibration[9] << 8;
	int16_t dig_P7 = pressureCalibration[10] | pressureCalibration[11] << 8;
	int16_t dig_P8 = pressureCalibration[12] | pressureCalibration[13] << 8;
	int16_t dig_P9 = pressureCalibration[14] | pressureCalibration[15] << 8;


	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)dig_P6;
	var2 = var2 + ((var1*(int64_t)dig_P5)<<17);
	var2 = var2 + (((int64_t)dig_P4)<<35);
	var1 = ((var1 * var1 * (int64_t)dig_P3)>>8) + ((var1 * (int64_t)dig_P2)<<12);
	var1 = (((((int64_t)1)<<47)+var1))*((int64_t)dig_P1)>>33;

	if (var1 == 0)
	{
		return 0; // avoid exception caused by division by zero
	}

	p = 1048576-tempPress;
	p = (((p<<31)-var2)*3125)/var1;
	var1 = (((int64_t)dig_P9) * (p>>13) * (p>>13)) >> 25;
	var2 = (((int64_t)dig_P8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7)<<4);
	return p/256;
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


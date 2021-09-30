/*
 * BMO280.h
 *
 *  Created on: Sep 16, 2021
 *      Author: Bruno Otávio
 */

#ifndef SRC_BMP280_H_
#define SRC_BMP280_H_

#include "stm32f4xx_hal.h"

typedef enum
{
	BMPSPI1,
	BMPSPI2,
	BMPSPI3
}SPI_CHOOSE;


typedef enum
{
	BMP_I2C1,
	BMP_I2C2,
	BMP_I2C3
}I2C_CHOOSE;

typedef enum
{

	NO_OVERSAMPLING,
	OVERSAMPLING_BY1,
	OVERSAMPLING_BY2,
	OVERSAMPLING_BY4,
	OVERSAMPLING_BY8,
	OVERSAMPLING_BY16
}BMP_Oversampling;

typedef enum
{

	SLEEP,
	FORCED,
	NORMAL
}BMP_PWRMode;

typedef enum
{
	T_0dot5,
	T_62dot5,
	T_125,
	T_250,
	T_500,
	T_1000,
	T_2000,
	T_4000
}BMP_StandByTime;

typedef enum
{
	F_COEF1,
	F_COEF2,
	F_COEF4,
	F_COEF8,
	F_COEF16
}BMP_IIRFIlter;


void __BMP_Read(uint8_t reg, int count,uint8_t *data);
void __BMP_Write(uint8_t *reg, uint8_t *data, uint8_t count);
void BMP_ConfigMeasurement(BMP_Oversampling tempOverSampling, BMP_Oversampling pressOverSampling, BMP_PWRMode pwrMode);
void BMP_Status(uint8_t *measuring, uint8_t *imUpdate);
uint8_t BMP_WhoAmI();
void BMP_Config(BMP_StandByTime normalOpStandBy, BMP_IIRFIlter iirFilter, uint8_t spiEnable);
void BMP_ReadRaw(int *rawPress, int *rawTemp);
float BMP_ReadTemperature();
float BMP_ReadPression();


#endif /* SRC_BMP280_H_ */

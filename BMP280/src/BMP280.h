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
	SPI1,
	SPI2,
	SPI3
}SPI_CHOOSE;


typedef enum
{
	BMP_I2C1,
	BMP_I2C2,
	BMP_I2C3
}I2C_CHOOSE;


void __BMP_Read(uint8_t reg, int count,uint8_t *data);
void __BMP_Write(uint8_t reg, uint8_t data);

#endif /* SRC_BMP280_H_ */

/*
 * ADS1220.c
 *
 *  Created on: Jul 17, 2025
 *      Author: kevinj
 */


#include "main.h"
#include "ADS1220.h"


void ADS1220_writeRegister(SPI_HandleTypeDef *hspi, uint8_t address, uint8_t value)
{
	uint8_t arr[2] =
	{ ADS1220_WREG | (address << 2), value };

	HAL_SPI_Transmit(hspi, arr, 2, 100);
}

uint8_t ADS1220_readRegister(SPI_HandleTypeDef *hspi, uint8_t address)
{
	uint8_t data[2] =
	{ 0, 0 };

	uint8_t txd[2] =
	{ (ADS1220_RREG | (address << 2)), 0xFF };

	HAL_SPI_TransmitReceive(hspi, txd, data, 2, 1000); // When doing bidirectional, transmit a dummy byte(0xFF), 2 in total, received register is in [1]
	return data[1];
}

void ADS1220_reset(SPI_HandleTypeDef *hspi)
{
	const uint8_t cmd = ADS1220_RESET;
	HAL_SPI_Transmit(hspi, (uint8_t*) &cmd, 1, 100);
}

void ADS1220_start_conversion(SPI_HandleTypeDef *hspi)
{
	const uint8_t cmd = ADS1220_START;
	HAL_SPI_Transmit(hspi, (uint8_t*) &cmd, 1, 100);
}

uint8_t ADS1220_init(SPI_HandleTypeDef *hspi, ADS1220_regs *r)
{
	ADS1220_reset(hspi);
	HAL_Delay(100);

	ADS1220_writeRegister(hspi, ADS1220_CONFIG_REG0_ADDRESS, r->cfg_reg0);
	ADS1220_writeRegister(hspi, ADS1220_CONFIG_REG1_ADDRESS, r->cfg_reg1);
	ADS1220_writeRegister(hspi, ADS1220_CONFIG_REG2_ADDRESS, r->cfg_reg2);
	ADS1220_writeRegister(hspi, ADS1220_CONFIG_REG3_ADDRESS, r->cfg_reg3);

	HAL_Delay(10);

	uint8_t CR0 = ADS1220_readRegister(hspi, ADS1220_CONFIG_REG0_ADDRESS);
	uint8_t CR1 = ADS1220_readRegister(hspi, ADS1220_CONFIG_REG1_ADDRESS);
	uint8_t CR2 = ADS1220_readRegister(hspi, ADS1220_CONFIG_REG2_ADDRESS);
	uint8_t CR3 = ADS1220_readRegister(hspi, ADS1220_CONFIG_REG3_ADDRESS);

	HAL_Delay(10);

	ADS1220_start_conversion(hspi);

	return (CR0 == r->cfg_reg0 && CR1 == r->cfg_reg1 && CR2 == r->cfg_reg2 && CR3 == r->cfg_reg3);
}


void ADS1220_select_mux_config(SPI_HandleTypeDef *hspi, int channels_conf, ADS1220_regs *r)
{
	r->cfg_reg0 &= ~ADS1220_REG_CONFIG0_MUX_MASK;
	r->cfg_reg0 |= channels_conf;
	ADS1220_writeRegister(hspi, ADS1220_CONFIG_REG0_ADDRESS, r->cfg_reg0);
}



/*
 * Function to poll data without DRDY. May not be a new sample if sent too fast.
 * Uses RDATA, new data sent on very next clock so uses a 4 byte buffer
 * Must have already init with continuous conversion, and started conversion
 *
 * Not as good as interrupt based read
 */

int32_t ADS1220_read_data_polling(SPI_HandleTypeDef *hspi)
{

	uint8_t tx[4] = {ADS1220_RDATA, 0xFF, 0xFF, 0xFF};  // RDATA command + 3 dummy bytes
	uint8_t rx[4];
	int32_t result;

	HAL_SPI_TransmitReceive(hspi, tx, rx, 4, 10); //10ms timeout

	// rx[1], rx[2], rx[3] contain the ADC result (24-bit signed), convert into 32bit
	result = ((int32_t)rx[1] << 16) | ((int32_t)rx[2] << 8) | rx[3];

	if (result & 0x800000) // check is sign bit (23) is negative
		result |= 0xFF000000;  // Sign extend 24-bit to 32-bit

	return result;
}

// lossy conversion, need to update
double ADS1220_ConvertToMilliVolts(int32_t raw) {

	double mvref = 2048; // reference in millivolts
	int gain = 4;

    return ((double)raw / 8388608.0) * (mvref / gain);
}


// This function reads 3 bytes from ADS1220 after DRDY interrupt
int32_t ADS1220_read_after_DRDYM(SPI_HandleTypeDef *hspi)
{
    uint8_t tx[3] = {0xFF, 0xFF, 0xFF};  // RDATA command + 3 dummy bytes
	uint8_t rx[3];
	int32_t result;

	HAL_SPI_TransmitReceive(hspi, tx, rx, 3, 10); //10ms timeout

	// rx[0], rx[1], rx[2] contain the ADC result (24-bit signed), convert into 32bit
	result = ((int32_t)rx[0] << 16) | ((int32_t)rx[1] << 8) | rx[2];

	if (result & 0x800000) // check is sign bit (23) is negative
		result |= 0xFF000000;  // Sign extend 24-bit to 32-bit

	return result;
}


// Start reading on a new channel (ch=0 AC V, ch=1 AC I)
void ADS1220_switch_CH_V_I(SPI_HandleTypeDef *hspi, uint8_t channel)
{
    if (channel == 0)
	{
		ADS1220_writeRegister(hspi, 0x00, 0x04); // AC_V (0, 1)
        ADS1220_start_conversion(hspi);
	}
	else
	{
		ADS1220_writeRegister(hspi, 0x00, 0x54); // AC_I (2, 3)
		ADS1220_start_conversion(hspi);
	}
}

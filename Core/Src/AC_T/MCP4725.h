/***************************************************************************************************/
/*
   This is a HAL based library for MCP4725, 12-bit Digital-to-Analog Converter with EEPROM

	 source code: https://github.com/SMotlaq
*/
/***************************************************************************************************/
#ifndef MCP4725_h
#define MCP4725_h

#include "main.h"
#include <math.h>

#define MCP4725_DISABLE_SANITY_CHECK     //disable some sanity checks to increase speed, use with caution

/* dac addresses */
typedef enum
{
  MCP4725A0_ADDR_A00         = 0x62,                              //i2c address, A0 = 0 DN/DP A2,A1 = 01
  MCP4725A0_ADDR_A01         = 0x63,                              //i2c address, A0 = 1 D1
}
MCP4725Ax_ADDRESS;

/* dac register, command bits */
typedef enum
{
  MCP4725_FAST_MODE          = 0x00,                              //writes data to DAC register
  MCP4725_REGISTER_MODE      = 0x40,                              //writes data & configuration bits to DAC register
  MCP4725_EEPROM_MODE        = 0x60                               //writes data & configuration bits to DAC register & EEPROM
}
MCP4725_COMMAND_TYPE;

/* dac register, power down bits */
typedef enum
{
  MCP4725_POWER_DOWN_OFF     = 0x00,                              //power down off
  MCP4725_POWER_DOWN_1KOHM   = 0x01,                              //power down on, with 1.0 kOhm to ground
  MCP4725_POWER_DOWN_100KOHM = 0x02,                              //power down on, with 100 kOhm to ground
  MCP4725_POWER_DOWN_500KOHM = 0x03                               //power down on, with 500 kOhm to ground
}
MCP4725_POWER_DOWN_TYPE;

/* dac library specific command */
typedef enum
{
  MCP4725_READ_SETTINGS      = 1,                                 //read 1 byte,  settings data
  MCP4725_READ_DAC_REG       = 3,                                 //read 3 bytes, DAC register data
  MCP4725_READ_EEPROM        = 5                                  //read 5 bytes, EEPROM data
}
MCP4725_READ_TYPE;

/* Some arithmatic functions */
#define lowByte(x)		((uint8_t)(x%256))
#define highByte(x)		((uint8_t)(x/256))

/* dac general call command */
#define MCP4725_GENERAL_CALL_ADDRESS 0x00                         //general call address
#define MCP4725_GENERAL_CALL_RESET   0x06                         //general call hard reset command
#define MCP4725_GENERAL_WAKE_UP      0x09                         //general call wake-up command

/* dac mics. */
#define MCP4725_RESOLUTION           12                           //resolution 12-bit
#define MCP4725_STEPS                4096 //pow(2, (MCP4725_RESOLUTION)) //quantity of DAC steps 2^12-bits = 4096
#define MCP4725_EEPROM_WRITE_TIME    25                           //non-volatile memory write time, maximum 50 msec

#define MCP4725_REFERENCE_VOLTAGE    3.30                         //supply-reference votltage
#define MCP4725_MAX_VALUE            4095 //((MCP4725_STEPS) - 1)
#define MCP4725_ERROR                0xFFFF                       //returns 65535, if communication error is occurred

typedef struct MCP
{
	// Privates:
	I2C_HandleTypeDef* hi2c;
	MCP4725Ax_ADDRESS _i2cAddress;
  float             _refVoltage;
  uint16_t          _bitsPerVolt;
} MCP4725;


// Publics:
MCP4725 	MCP4725_init(I2C_HandleTypeDef* hi2c, MCP4725Ax_ADDRESS addr, float refV);

uint8_t 	MCP4725_isConnected(MCP4725* _MCP4725);

void			MCP4725_setReferenceVoltage(MCP4725* _MCP4725, float value);
float			MCP4725_getReferenceVoltage(MCP4725* _MCP4725);

uint8_t		MCP4725_setValue(MCP4725* _MCP4725, uint16_t value, MCP4725_COMMAND_TYPE mode, MCP4725_POWER_DOWN_TYPE powerType);
uint8_t		MCP4725_setVoltage(MCP4725* _MCP4725, float voltage, MCP4725_COMMAND_TYPE mode, MCP4725_POWER_DOWN_TYPE powerType);

uint16_t	MCP4725_getValue(MCP4725* _MCP4725);
float			MCP4725_getVoltage(MCP4725* _MCP4725);
uint16_t	MCP4725_getStoredValue(MCP4725* _MCP4725);
float			MCP4725_getStoredVoltage(MCP4725* _MCP4725);

uint16_t	MCP4725_getPowerType(MCP4725* _MCP4725);
uint16_t	MCP4725_getStoredPowerType(MCP4725* _MCP4725);

void			MCP4725_reset(MCP4725* _MCP4725);
void			MCP4725_wakeUP(MCP4725* _MCP4725);




// Privates:
uint8_t		MCP4725_getEepromBusyFlag(MCP4725* _MCP4725);
uint8_t		MCP4725_writeComand(MCP4725* _MCP4725, uint16_t value, MCP4725_COMMAND_TYPE mode, MCP4725_POWER_DOWN_TYPE powerType);
uint16_t	MCP4725_readRegister(MCP4725* _MCP4725, MCP4725_READ_TYPE dataType);

void test(void);

#endif

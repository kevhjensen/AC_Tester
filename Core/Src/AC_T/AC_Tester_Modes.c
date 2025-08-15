/*
 * AC_Tester_Modes.c
 *
 *  Created on: Jul 16, 2025
 *      Author: kevinj
 */

#include "AC_T_globals.h"

volatile uint16_t STDdataPeriodMS = 100;


char *trim(char *str) {
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '\0') return str;

    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
        *end-- = '\0';

    return str;
}

uint8_t strToUInt(char *str, uint16_t *output)
{
	char *endptr;
	uint16_t value = strtoul(str, &endptr, 10);
	if (*endptr == '\0') { // error checking
		*output = value;
		return 1;
	}
	return 0;
}

void invalidCMD(void)
{
	snprintf(buf, sizeof(buf), "invalid CMD\r\n");
	CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}
void validRespondStr(char *key, char*value)
{
	snprintf(buf, sizeof(buf), "%s: %s\r\n", key, value);
	CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}
void validRespondUint(char *key, uint8_t value)
{
	snprintf(buf, sizeof(buf), "%s: %u\r\n", key, value);
	CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}




volatile uint8_t printSamples = 0;
volatile uint8_t data_mode = 0;

void set_data_mode(uint16_t mode)
{
	data_mode = mode;
	validRespondUint("data_mode", mode);
}
void set_std_data_interval(int interval);

void set_manual_cp(uint16_t cp_value)
{
	if (set_cp_state(cp_value) == 1) {
		validRespondUint("manual_cp", cp_value);
		return;
	}
	invalidCMD();
}



int ParseUSBCommand(char *line)
{
	// get key : value, remove all whitespace
    char *sep = strchr(line, ':');
    if (!sep) return -1;

    *sep = '\0';
    char *key = line;
    char *rawValue = sep + 1;
    key = trim(key);
    rawValue = trim(rawValue);

    uint16_t value;
    if (!strToUInt(rawValue, &value)) {
    	invalidCMD();
    	return -2;
    }


    if ((strcmp(key, "data_mode") == 0) || (strcmp(key, "dm") == 0))
        set_data_mode(value);
    else if (strcmp(key, "ct") == 0)
    	update_charger_type(value);
    else if ((strcmp(key, "manual_cp") == 0) || (strcmp(key, "cp") == 0))
        set_manual_cp(value);
    else if (strcmp(key, "ma") == 0)
    	update_CT_mA_setpoint(value*1000);
    else if (strcmp(key, "sm") == 0)
    	update_Sense_Mode(value);
    else if (strcmp(key, "veh_prof") == 0)
        ; // etc.
    else
        return -2; // unknown command

    return 0;
}

void USB_Reenumerate(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Disable USB peripheral
    __HAL_RCC_USB_CLK_DISABLE();

    // Take control of PA12 as GPIO
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Pull D+ low
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);

    // Wait long enough for host to detect disconnect
    HAL_Delay(50);

    // Return PA12 to USB (Alternate Function Push Pull)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Re-enable USB peripheral
    __HAL_RCC_USB_CLK_ENABLE();

    // Re-init USB device
    MX_USB_DEVICE_Init();
}

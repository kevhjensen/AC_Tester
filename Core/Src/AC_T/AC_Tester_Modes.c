/*
 * AC_Tester_Modes.c
 *
 *  Created on: Jul 16, 2025
 *      Author: kevinj
 */

#include "AC_T_globals.h"

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
	addToMsgQ(buf);
}
void validRespondStr(char *key, char*value)
{
	snprintf(buf, sizeof(buf), "%s: %s\r\n", key, value);
	addToMsgQ(buf);
}
void validRespondUint(char *key, uint32_t value)
{
	snprintf(buf, sizeof(buf), "%s: %lu\r\n", key, value);
	addToMsgQ(buf);
}





volatile uint8_t data_mode = 0;

void set_data_mode(uint16_t mode)
{
	data_mode = mode;
	validRespondUint("data_mode", mode);
}


void set_charger_type(uint8_t type)
{
	if (update_charger_type(type) == 1) {
		validRespondUint("charger_type", type);
		return;
	}
	invalidCMD();
}

void set_ct_ratio(uint32_t new_conv_factor)
{
	if (update_AC_I_raw_mV_per_actual_A_x1000(new_conv_factor) == 1) {
		validRespondUint("ct_ratio", new_conv_factor);
		return;
	}
	invalidCMD();
}

void set_sense_mode(uint8_t mode)
{
	if (update_Sense_Mode(mode) == 1) {
		validRespondUint("sense_mode", mode);
		return;
	}
	invalidCMD();
}

void set_manual_cp(uint16_t cp_value)
{
	if (set_cp_state(cp_value) == 1) {
		isVehicleControlNotManual = 0;
		validRespondUint("manual_cp", cp_value);
		return;
	}
	invalidCMD();
}

void set_mm(int32_t milliamp_sp)
{
	if (update_CT_mA_setpoint(milliamp_sp) == 1) {
		isVehicleControlNotManual = 0;
		validRespondUint("manual_milliamps", milliamp_sp);
		return;
	}
	invalidCMD();
}

void set_ma(int32_t amp_sp)
{
	if (update_CT_mA_setpoint(1000 * amp_sp) == 1) {
		isVehicleControlNotManual = 0;
		validRespondUint("manual_amps", amp_sp);
		return;
	}
	invalidCMD();
}

void set_phase_shift(int32_t new_CT_phase_us_offset)
{
	if (update_CT_phase_us_offset(new_CT_phase_us_offset) == 1) {
		validRespondUint("manual_phase_shift_us", new_CT_phase_us_offset);
		return;
	}
	invalidCMD();
}

void set_vehicle_control(uint8_t new_vehicle_control)
{
	if (update_vehicle_control(new_vehicle_control) == 1) {
		validRespondUint("vehicle_control", new_vehicle_control);
		return;
	}
	invalidCMD();
}

void set_vehicle_amp_limit(uint8_t new_vehicle_amp_limit)
{
	if (update_vehicle_amp_limit(new_vehicle_amp_limit) == 1) {
		validRespondUint("vehicle_amp_limit", new_vehicle_amp_limit);
		return;
	}
	invalidCMD();
}



void ParseUSBCommand(char *line)
{
	// get key : value, remove all whitespace
    char *sep = strchr(line, ':');
    if (!sep) {
    	invalidCMD();
    	return;
    }

    *sep = '\0';
    char *key = line;
    char *rawValue = sep + 1;
    key = trim(key);
    rawValue = trim(rawValue);

    uint16_t value;
    if (!strToUInt(rawValue, &value)) {
    	invalidCMD();
    	return;
    }


    if ((strcmp(key, "data_mode") == 0) || (strcmp(key, "dm") == 0))
        set_data_mode(value);
    else if (strcmp(key, "ct") == 0)
    	set_charger_type(value);
    else if (strcmp(key, "cr") == 0)
    	set_ct_ratio(value);
    else if (strcmp(key, "sm") == 0)
    	set_sense_mode(value);
    else if ((strcmp(key, "manual_cp") == 0) || (strcmp(key, "cp") == 0))
        set_manual_cp(value);
    else if (strcmp(key, "mm") == 0)
        set_mm(value);
    else if (strcmp(key, "ma") == 0)
    	set_ma(value);
    else if (strcmp(key, "mp") == 0)
    	set_phase_shift(value);
    else if (strcmp(key, "vc") == 0)
    	set_vehicle_control(value);
    else if (strcmp(key, "vl") == 0)
    	set_vehicle_amp_limit(value);
    else
    	invalidCMD();
}

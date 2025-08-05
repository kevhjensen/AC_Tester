/*
 * myHelpers.c
 *
 *  Created on: Jul 18, 2025
 *      Author: kevinj
 */


#include "AC_T_globals.h"


char buf[64];
char strTmp[64];

void addToTrace(char * s) {
    snprintf(buf, sizeof(buf), "[%ld] %s\r\n", HAL_GetTick(), s);
    CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}


uint32_t int_sqrt64(uint64_t x) {
    uint64_t op = x;
    uint64_t res = 0;
    uint64_t one = 1ULL << 62;
    while (one > op) one >>= 2;
    while (one != 0) {
        if (op >= res + one) {
            op -= res + one;
            res = (res >> 1) + one;
        } else {
            res >>= 1;
        }
        one >>= 2;
    }
    return (uint32_t)res;
}


// accurate
int32_t cp_V_x100_from_raw(uint16_t raw)
{
    return 165 + (330 * raw - 675675) * 11 / 4095;
}


#define ADS_FULL_SCALE     8388608    // 2^23
#define ADS_SCALE_FACTOR   512        // (2048 mV / Gain 4)

#define AC_V_RESISTOR_RATIO    991       // actual AC mV = 991 * ADS1220 input mV for AC_T PCB

uint32_t x1000_CT_ratio_divBy_burdenResistance = 320512; //ax80 206,954 ax48


// = (raw * 512 / 2^23) * 991  // first part gives mV
int32_t AC_V_mV_from_raw(int32_t raw)
{
    int64_t temp = (int64_t)raw * 512 * AC_V_RESISTOR_RATIO;

    return (int32_t)((temp + (ADS_FULL_SCALE / 2)) / ((int64_t)ADS_FULL_SCALE)); // rounds
} // ads max out 507,392 mV

// = (raw * 512 / 2^23) * 2500 / 7.8 //CT ratio / resistance
int32_t AC_I_mA_from_raw(int32_t raw)
{
    int64_t temp = (int64_t)raw * ADS_SCALE_FACTOR * x1000_CT_ratio_divBy_burdenResistance;

    return (int32_t)((temp + (ADS_FULL_SCALE / 2)) / ((int64_t)ADS_FULL_SCALE * 1000));
} // ads max out 164,102 mA

int64_t AC_P_uW_from_raw(int64_t raw) //raw is 2 24bit multiplied = 7.037e13
{
    // Step 1: divide early to prevent oversized intermediate
    int64_t scaled =  x1000_CT_ratio_divBy_burdenResistance * raw / ADS_FULL_SCALE;
    scaled = (scaled* AC_V_RESISTOR_RATIO) / 1000; // max is 6.977e17, int64 max is 9.22e18
    scaled = scaled * ADS_SCALE_FACTOR * ADS_SCALE_FACTOR / ADS_FULL_SCALE;

    return scaled; // µW both V I at max ADS output is 83,168,462,396 (83.168kW), can be multiplied by 110,888,472 for int64
}

// safe for max uW above and 500,000uS
int64_t energy_uWh_from_uW_us(int64_t power_uW, int64_t time_us)
{
    // Multiply first; safe for inputs up to ~10^13 and ~10^6 respectively
    int64_t energy_uWus = power_uW * time_us;

    // Convert to µWh
    return energy_uWus / 3600000000;
}


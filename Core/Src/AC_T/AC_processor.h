/*
 * AC_processor.h
 *
 *  Created on: Jul 17, 2025
 *      Author: kevinj
 */

#ifndef SRC_AC_T_AC_PROCESSOR_H_
#define SRC_AC_T_AC_PROCESSOR_H_

#include "AC_T_globals.h"

uint8_t update_CT_mA_setpoint(int32_t new_mA_setpoint);
uint8_t update_CT_phase_us_offset(int32_t new_CT_phase_us_offset); // may need to add limits, positive for leading current
uint8_t update_Sense_Mode(uint8_t new_sense_mode); // 1 for sense mode (read 2ch), 0 for emulate mode, also resets energy register

// 30 cycles = 500ms, can reduce to increase data update frequency, but more fluctuations / noise. Min is 3
#define NUM_CYCLES 30 // number of AC V cycles that is processed at a time (10 cycles 60Hz = 167.7ms, 322samples 1ch, 146samples 2ch)
#define NUM_ZC (NUM_CYCLES) // after seeing NUM_ZC it will have NUM_CYCLES in buf

#define AC_BUFFER_SIZE 1024 // 512 is enough for 265ms 1ch, >500ms 2ch




void new_ADS_AC_V_sample(int32_t sample, uint16_t sampleTime);
void new_ADS_AC_I_sample(int32_t sample, uint16_t sampleTime);



typedef struct {
    float Vrms; // volts
    float Irms; // amps
    float RealPower_W;
    float ApparentPower_W;

    float VrmsRaw;
    uint64_t Vsq_sum_raw;
    uint64_t Isq_sum_raw;

    uint16_t SampleNum;
    uint32_t Duration_us;

    float PF;
    float Frequency_Hz;

    float avgSampleTime;
    uint8_t zc_count;
} AC_Results_float;

extern volatile AC_Results_float cur_AC_data;

extern volatile float AC_EnergyRegister_Wh;





void processACdata10ms(void);

#endif /* SRC_AC_T_AC_PROCESSOR_H_ */

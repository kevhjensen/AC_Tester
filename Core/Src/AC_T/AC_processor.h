/*
 * AC_processor.h
 *
 *  Created on: Jul 17, 2025
 *      Author: kevinj
 */

#ifndef SRC_AC_T_AC_PROCESSOR_H_
#define SRC_AC_T_AC_PROCESSOR_H_

#include "AC_T_globals.h"


// #define AC_PROCESS_INTERVAL 100 // MS
#define NUM_CYCLES 10 // number of AC V cycles that is processed at a time (60Hz = 167.7ms, 322samples 1ch, 146samples 2ch)
#define NUM_ZC (NUM_CYCLES) // after seeing NUM_ZC it will have NUM_CYCLES in buf

#define AC_BUFFER_SIZE 512 // enough for 265ms 1ch, >500ms 2ch

// #define ZC_INDEX_BUFFER_SIZE 32



void new_ADS_AC_V_sample(int32_t sample);
void new_ADS_AC_I_sample(int32_t sample);



typedef struct {
    int32_t Vrms;
    int32_t Irms;
    int64_t RealPower;
    int64_t ApparentPower;

    int64_t Vsq_sum;
    int64_t Isq_sum;
    uint16_t SampleNum;
    uint32_t Duration_us;

    int32_t PF_x1000;
    int32_t PhaseAngle_deg_x10;
    uint32_t Frequency_Hz_x100;
} AC_Results_t;
// raw values
extern AC_Results_t cur_AC_data;


extern volatile int64_t AC_EnergyRegister_uWh;

extern volatile int64_t AC_PowerReal_uW;
extern volatile int32_t AC_Vrms_mV;
extern volatile int32_t AC_Irms_mA;



void processACdata10ms(void);

#endif /* SRC_AC_T_AC_PROCESSOR_H_ */

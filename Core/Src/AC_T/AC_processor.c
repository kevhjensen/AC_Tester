/*
 * AC_processor.c
 *
 *  Created on: Jul 17, 2025
 *      Author: kevinj
 */

#include "AC_T_globals.h"

/*
 * buffer 0 index is point after zero crossing after initial buffer processed (settling time)
 *
 * last V sample(V_bufX_head - 1) will be point after 11th zero crossing
 * This sample also written to start of new buffer
 *
 * process 0 to V_bufX_head - 2
 *
 * Even though BufferFull called before getting a I sample in 2ch mode,
 * 	this is fine since last data point (head - 1) discarded. Next I sample will go to start of buffer and match timewise
 *
 */
int32_t V_buf1[AC_BUFFER_SIZE];
int32_t V_buf2[AC_BUFFER_SIZE]; //write to a buffer while process the other one


uint16_t V_buf1_head = 0; // index to write new samples to; when ready it gives numSamples in buffer
uint16_t V_buf2_head = 0;

uint8_t V_buf_to_write = 1; // which buffer currently writing to 1 for 1, 2 for 2

uint8_t V_buf_to_process = 0; // which buffer is ready to process
uint8_t V_buf_to_process_zc_count = 0; // should be 11 with AC signal, but w no signal it will be less
uint32_t V_buf_to_process_start_time = 0;
uint32_t V_buf_to_process_end_time = 0;


int32_t I_buf1[AC_BUFFER_SIZE];
int32_t I_buf2[AC_BUFFER_SIZE];

int32_t last_V_sample = 0;
int32_t last_I_sample = 0;

uint32_t to_process_time = 0;

int8_t last_sample_sign = 0;
uint8_t zero_cross_count = 0;

void bufferFull(int32_t dupSample) // switch V_buf_to_write, set to_process flag, dupSample at start of new buf
{
	V_buf_to_process_start_time = V_buf_to_process_end_time;
	V_buf_to_process_end_time = micros();
	to_process_time = V_buf_to_process_end_time - V_buf_to_process_start_time;
	V_buf_to_process_zc_count = zero_cross_count;
	zero_cross_count = 0;

	if (V_buf_to_write == 1)
	{
		V_buf_to_process = 1; // TO DO if != 0 then didn't finish processing last

		V_buf_to_write = 2;
		V_buf2_head = 0; // reset new write buffer
		V_buf2[V_buf2_head++] = dupSample; // start with duplicate post zc value
	}
	else
	{
		V_buf_to_process = 2; // TO DO if != 0 then didn't finish last

		V_buf_to_write = 1;
		V_buf1_head = 0;
		V_buf1[V_buf1_head++] = dupSample;
	}
}

/* call after already writing newSample
 * If sample is right after 11th zero crossing, it will also be written to the new buffer
 */
void processRisingZeroCrossing(int32_t newSample)
{
    int8_t current_sign = (newSample >= 0) ? 1 : -1;

    // Check for rising edge
    if (last_sample_sign < 0 && current_sign >= 0)
    {
        zero_cross_count++;

        if (zero_cross_count >= NUM_ZC)
        {
            bufferFull(newSample);
        }
    }

    last_sample_sign = current_sign;
}


void new_ADS_AC_V_sample(int32_t sample) // ADS1220 interrupt fired, so keep lightweight
{
    if (V_buf_to_write == 1)
    {
        if (V_buf1_head < AC_BUFFER_SIZE)
            V_buf1[V_buf1_head++] = sample; // write to head index then increment
        else
        	bufferFull(sample); // process data even though no 60hz signal
    }
    else
    {
        if (V_buf2_head < AC_BUFFER_SIZE)
            V_buf2[V_buf2_head++] = sample;
        else
        	bufferFull(sample);
    }

	processRisingZeroCrossing(sample); // sample index is head - 1 at this point


	V_numSamples++;
	if (data_mode == 1 || data_mode == 2) {
		snprintf(buf, sizeof(buf), "%lu, V, %li \r\n", micros(), sample);
		CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
	}
}

void new_ADS_AC_I_sample(int32_t sample) // handles I current values when ADS1220_read_both_CHs = 1, values alternate with V time wise
{
	// Interpolate midpoint, so I sample time aligns with V (alternating sampling)
	// First value will be wrong, but we will ignore 1st buffer process anyways
	int32_t interpolated_I = (last_I_sample + sample) / 2;

	if (V_buf_to_write == 1)
	{
		if (V_buf1_head > 0 && V_buf1_head - 1 < AC_BUFFER_SIZE) {
			I_buf1[V_buf1_head - 1] = interpolated_I; // align with latest V
		}
	}
	else
	{
		if (V_buf2_head > 0 && V_buf2_head - 1 < AC_BUFFER_SIZE) {
			I_buf2[V_buf2_head - 1] = interpolated_I; // align with latest V
		}
	}

	last_I_sample = sample;





	I_numSamples++;
	if (data_mode == 1 || data_mode == 2) {
		snprintf(buf, sizeof(buf), "%lu, I, %li \r\n", micros(), sample);
		CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
	}
}

#define LEADING   -1
#define LAGGING    1
#define UNKNOWN    0
int8_t get_leading_lagging(int32_t *V_buf, int32_t *I_buf, uint16_t length)
{
    uint16_t v_zc_index = -1;
    uint16_t i_zc_index = -1;

    // Find the first rising zero crossing for V (index 0 is point after a V rising zc)
    for (uint16_t n = 1; n < length; n++) {
        if (V_buf[n - 1] < 0 && V_buf[n] >= 0) {
            v_zc_index = n;
            break;
        }
    }

    // Find the first rising zero crossing for I
    for (uint16_t n = 1; n < length; n++) {
        if (I_buf[n - 1] < 0 && I_buf[n] >= 0) {
            i_zc_index = n;
            break;
        }
    }

    if (v_zc_index < 0 || i_zc_index < 0)
        return UNKNOWN;  // no zero crossing detected

    return (i_zc_index >= v_zc_index) ? LAGGING : LEADING;
}

int32_t calc_phase_deg_x10(int32_t PF_x1000)
{
	if (PF_x1000 < 0) PF_x1000 = -PF_x1000;
	if (PF_x1000 > 1000) PF_x1000 = 1000;

	// Linear approximation: (1 - PF) * 90° * 10
	return (int32_t)((1000 - PF_x1000) * 9 / 10);
}

int32_t calculate_signed_phase_deg_x10(int32_t *V_buf, int32_t *I_buf, uint16_t length, int32_t PF_x1000)
{
    int32_t angle = calc_phase_deg_x10(PF_x1000);
    int8_t dir = get_leading_lagging(V_buf, I_buf, length);

    if (dir == LEADING) {
        angle = -angle;  // Negative angle for leading
    }
    return angle;  // Positive for lagging, negative for leading
}

// Estimate frequency * 100 based on number of zero-crossings and duration in microseconds
uint32_t calc_frequency_hz_x100(uint8_t zc_count, uint32_t duration_us)
{
    if (zc_count < 1 || duration_us == 0) return 0;

    // frequency * 100 = ((zc_count - 1) * 1e8) / duration_us
    return (uint32_t)(((uint64_t)(zc_count) * 100000000UL) / duration_us);
}



AC_Results_t cur_AC_data = {0};
volatile int64_t AC_EnergyRegister_uWh = 0;
volatile int64_t AC_PowerReal_uW = 0;
volatile int32_t AC_Vrms_mV = 0;
volatile int32_t AC_Irms_mA = 0;

AC_Results_t analyze_AC_waveform(int32_t* V_buf, int32_t* I_buf, uint16_t N, uint8_t zc_count, uint32_t duration_us) {


    uint64_t Vsq_sum = 0, Isq_sum = 0;
    int64_t Psum = 0;

    for (uint16_t i = 0; i < N; i++) { // N samples is = len of buf valid samples
        int32_t V = V_buf[i];
        int32_t I = I_buf[i];
        Vsq_sum += (int64_t)V * V;
        Isq_sum += (int64_t)I * I;
        Psum    += (int64_t)V * I; // 2 24bit signed multiplied = 70,368,744,177,649, can add up to 131,071 before overflow

    }

    int32_t Vrms = int_sqrt64(Vsq_sum / N);
    int32_t Irms = int_sqrt64(Isq_sum / N);
    int64_t RealPower = Psum / N;
    int64_t ApparentPower = ((int64_t)Vrms * Irms);

    int32_t PF_x1000 = 0;  // Power factor * 1000
    if (ApparentPower != 0)
        PF_x1000 = (int32_t)((RealPower * 1000) / ApparentPower);

    AC_Results_t result;
    result.Vrms = Vrms;
    result.Irms = Irms;
    result.RealPower = RealPower;
    result.ApparentPower = ApparentPower;

    result.Vsq_sum = Vsq_sum;
    result.Isq_sum = Isq_sum;
    result.SampleNum = N;
    result.Duration_us = duration_us;

    result.PF_x1000 = PF_x1000; // could be negative if - real power (apparent power always +)
    result.PhaseAngle_deg_x10 = calculate_signed_phase_deg_x10(V_buf, I_buf, N, PF_x1000);

    result.Frequency_Hz_x100 = calc_frequency_hz_x100(zc_count, duration_us);

    return result;
}


/*
 * ADE7858
 * 	Vrms
 * 		1) HPF -> square value -> LPF -> sum and sqrt
 * 		2) absolute value, extract DC offset, get average * 2^0.5
 *
 *
 * CT AX48: +0.03% current error, 0.1 degree phase angle lag ~ 4.63 µs delay, did not account )0.000015% error in real power
 *
 *
 */
void processACdata10ms(void) // run this task faster than AC_CYCLE_NUMBER * AC mains period(60Hz, 10cycles is 167ms)
{
	if (V_buf_to_process == 0) {
		return;
	}
//	// power factor = 1 I raw data = V
//	if (V_buf_to_process == 1) {
//		cur_AC_data = analyze_AC_waveform(V_buf1, V_buf1, V_buf1_head - 2, V_buf_to_process_zc_count, to_process_time);
//		V_buf_to_process = 0;
//
//	}
//	else if (V_buf_to_process == 2) {
//
//		cur_AC_data = analyze_AC_waveform(V_buf2, V_buf2, V_buf2_head - 2, V_buf_to_process_zc_count, to_process_time);
//		V_buf_to_process = 0;
//	}

	if (V_buf_to_process == 1) {
		cur_AC_data = analyze_AC_waveform(V_buf1, I_buf1, V_buf1_head - 2, V_buf_to_process_zc_count, to_process_time);
		V_buf_to_process = 0;

	}
	else if (V_buf_to_process == 2) {

		cur_AC_data = analyze_AC_waveform(V_buf2, I_buf2, V_buf2_head - 2, V_buf_to_process_zc_count, to_process_time);
		V_buf_to_process = 0;
	}
	AC_Vrms_mV = AC_V_mV_from_raw(cur_AC_data.Vrms);
	AC_Irms_mA = AC_I_mA_from_raw(cur_AC_data.Irms);
	AC_PowerReal_uW = AC_P_uW_from_raw(cur_AC_data.RealPower);
	AC_EnergyRegister_uWh += energy_uWh_from_uW_us(AC_PowerReal_uW, cur_AC_data.Duration_us); // uJ microJoule

	snprintf(buf, sizeof(buf),
	         "%lu, %ld,%ld, %lld, %lld, %lld, %li\r\n",
			 HAL_GetTick(),
			 AC_Vrms_mV,                   // Vrms
			 AC_Irms_mA,                   // Irms

			 AC_EnergyRegister_uWh / 1000000, //Wh

			 AC_PowerReal_uW / 1000000,       // W
			 AC_P_uW_from_raw(cur_AC_data.ApparentPower)/ 1000000, // W

			 cur_AC_data.PF_x1000

	);
	if (data_mode == 7 || data_mode == 8) {
	    CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
	}

	// result.Energy = ((int64_t)RealPower * duration_us);
}













///*
// * AC_processor.c
// *
// *  Created on: Jul 17, 2025
// *      Author: kevinj
// */
//
//#include "AC_T_globals.h"
//
///*
// * buffer 0 index is point after zero crossing after initial buffer processed (settling time)
// *
// * last V sample(V_bufX_head - 1) will be point after 11th zero crossing
// * This sample also written to start of new buffer
// *
// * process 0 to V_bufX_head - 2
// *
// * Even though BufferFull called before getting a I sample in 2ch mode,
// * 	this is fine since last data point (head - 1) discarded. Next I sample will go to start of buffer and match timewise
// *
// */
//int32_t V_buf1[AC_BUFFER_SIZE];
//int32_t V_buf2[AC_BUFFER_SIZE]; //write to a buffer while process the other one
//
//
//uint16_t V_buf1_head = 0; // index to write new samples to; when ready it gives numSamples in buffer
//uint16_t V_buf2_head = 0;
//
//uint8_t V_buf_to_write = 1; // which buffer currently writing to 1 for 1, 2 for 2
//
//uint8_t V_buf_to_process = 0; // which buffer is ready to process
//uint8_t V_buf_to_process_zc_count = 0; // should be 11 with AC signal, but w no signal it will be less
//uint32_t V_buf_to_process_start_time = 0;
//uint32_t V_buf_to_process_end_time = 0;
//
//int8_t last_sample_sign = 0;
//uint8_t zero_cross_count = 0;
//
//
//uint8_t bufferSwitchPending = 0;
//
//int32_t I_buf1[AC_BUFFER_SIZE];
//int32_t I_buf2[AC_BUFFER_SIZE];
//
//int32_t last_V_sample = 0;
//int32_t last_I_sample = 0;
//
//
//void bufferFull(int32_t dupSample) // switch V_buf_to_write, set to_process flag, dupSample at start of new buf
//{
//	V_buf_to_process_start_time = V_buf_to_process_end_time;
//	V_buf_to_process_end_time = micros();
//	V_buf_to_process_zc_count = zero_cross_count;
//	zero_cross_count = 0;
//
//	if (V_buf_to_write == 1)
//	{
//		V_buf_to_process = 1; // TO DO if != 0 then didn't finish processing last
//
//		V_buf_to_write = 2;
//		V_buf2_head = 0; // reset new write buffer
//		V_buf2[V_buf2_head++] = dupSample; // start with duplicate post zc value
//	}
//	else
//	{
//		V_buf_to_process = 2; // TO DO if != 0 then didn't finish last
//
//		V_buf_to_write = 1;
//		V_buf1_head = 0;
//		V_buf1[V_buf1_head++] = dupSample;
//	}
//}
//
///*
// * call after already writing newSample
// * If sample is right after a zero crossing, it will also be written to the new buffer
// */
//void processRisingZeroCrossing(int32_t newSample)
//{
//    int8_t current_sign = (newSample >= 0) ? 1 : -1;
//
//    // Check for rising edge
//    if (last_sample_sign < 0 && current_sign >= 0)
//    {
//    	bufferFull(newSample);
//    }
//
//    last_sample_sign = current_sign;
//}
//
//
//void new_AC_V_sample(int32_t sample) // ADS1220 interrupt fired, so keep lightweight
//{
//    if (V_buf_to_write == 1)
//    {
//        if (V_buf1_head < AC_BUFFER_SIZE)
//            V_buf1[V_buf1_head++] = sample; // write to head index then increment
//        else
//        	bufferFull(sample); // process data even though no 60hz signal
//    }
//    else
//    {
//        if (V_buf2_head < AC_BUFFER_SIZE)
//            V_buf2[V_buf2_head++] = sample;
//        else
//        	bufferFull(sample);
//    }
//
//	processRisingZeroCrossing(sample); // sample index is head - 1 at this point
//
//
//	V_numSamples++;
//	if (data_mode == 1 || data_mode == 2) {
//		snprintf(buf, sizeof(buf), "%lu, V, %li \r\n", micros(), sample);
//		CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
//	}
//}
//
//void new_AC_I_sample(int32_t sample)
//{
//	// Interpolate midpoint, so I sample time aligns with V (alternating sampling)
//	// First value will be wrong, but we will ignore 1st buffer process anyways
//	int32_t interpolated_I = (last_I_sample + sample) / 2;
//
//	if (V_buf_to_write == 1)
//	{
//		if (V_buf1_head > 0 && V_buf1_head - 1 < AC_BUFFER_SIZE) {
//			I_buf1[V_buf1_head - 1] = interpolated_I; // align with latest V
//		}
//	}
//	else
//	{
//		if (V_buf2_head > 0 && V_buf2_head - 1 < AC_BUFFER_SIZE) {
//			I_buf2[V_buf2_head - 1] = interpolated_I; // align with latest V
//		}
//	}
//
//	last_I_sample = sample;
//
//
//
//
//
//	I_numSamples++;
//	if (data_mode == 1 || data_mode == 2) {
//		snprintf(buf, sizeof(buf), "%lu, I, %li \r\n", micros(), sample);
//		CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
//	}
//}
//
//
///*
// * ADE7858
// * 	Vrms
// * 		1) HPF -> square value -> LPF -> sum and sqrt
// * 		2) absolute value, extract DC offset, get average * 2^0.5
// *
// *
// * CT AX48: +0.03% current error, 0.1 degree phase angle lag ~ 4.63 µs delay, did not account )0.000015% error in real power
// *
// *
// */
//void processACdata10ms(void) // run this task faster than AC_CYCLE_NUMBER * AC mains period(60Hz, 10cycles is 167ms)
//{
//	if (V_buf_to_process == 0)
//		return;
//	// result.Energy = ((int64_t)RealPower * duration_us);
//}
//
//
//
//
//
//
//
//
//#define LEADING   -1
//#define LAGGING    1
//#define UNKNOWN    0
//
//int8_t get_leading_lagging(int32_t *V_buf, int32_t *I_buf, uint16_t length)
//{
//    uint16_t v_zc_index = -1;
//    uint16_t i_zc_index = -1;
//
//    // Find the first rising zero crossing for V
//    for (uint16_t n = 1; n < length; n++) {
//        if (V_buf[n - 1] < 0 && V_buf[n] >= 0) {
//            v_zc_index = n;
//            break;
//        }
//    }
//
//    // Find the first rising zero crossing for I
//    for (uint16_t n = 1; n < length; n++) {
//        if (I_buf[n - 1] < 0 && I_buf[n] >= 0) {
//            i_zc_index = n;
//            break;
//        }
//    }
//
//    if (v_zc_index < 0 || i_zc_index < 0)
//        return UNKNOWN;  // no zero crossing detected
//
//    return (i_zc_index >= v_zc_index) ? LAGGING : LEADING;
//}
//
//int32_t calc_phase_deg_x10(int32_t PF_x1000)
//{
//	if (PF_x1000 < 0) PF_x1000 = -PF_x1000;
//	if (PF_x1000 > 1000) PF_x1000 = 1000;
//
//	// Linear approximation: (1 - PF) * 90° * 10
//	return (int32_t)((1000 - PF_x1000) * 9 / 10);
//}
//
//int32_t calculate_signed_phase_deg_x10(int32_t *V_buf, int32_t *I_buf, uint16_t length, int32_t PF_x1000)
//{
//    int32_t angle = calc_phase_deg_x10(PF_x1000);
//    int8_t dir = get_leading_lagging(V_buf, I_buf, length);
//
//    if (dir == LEADING) {
//        angle = -angle;  // Negative angle for leading
//    }
//    return angle;  // Positive for lagging, negative for leading
//}
//
//// Estimate frequency * 100 based on number of zero-crossings and duration in microseconds
//uint32_t calc_frequency_hz_x100(uint8_t zc_count, uint32_t duration_us)
//{
//    if (zc_count < 2 || duration_us == 0) return 0;
//
//    // frequency * 100 = ((zc_count - 1) * 1e8) / duration_us
//    return (uint32_t)(((uint64_t)(zc_count - 1) * 100000000UL) / duration_us);
//}
//
//// N samples is = len of buf valid samples
//AC_Results_t analyze_AC_waveform(int32_t* V_buf, int32_t* I_buf, uint16_t N, uint8_t zc_count, uint32_t duration_us) {
//
//	// if (N == 0) return;
//
//	// remove dc offset (high pass filter)
////	int64_t Vsum = 0, Isum = 0;
////    for (uint16_t i = 0; i < N; i++) {
////        Vsum += V_buf[i];
////        Isum += I_buf[i];
////    }
////    int32_t Vmean = Vsum / N;
////    int32_t Imean = Isum / N;
//
//    uint64_t Vsq_sum = 0, Isq_sum = 0;
//    int64_t Psum = 0;
//
//    for (uint16_t i = 0; i < N; i++) {
//        int32_t V = V_buf[i]; //- Vmean; dc offset
//        int32_t I = I_buf[i]; //- Imean;
//        Vsq_sum += (int64_t)V * V;
//        Isq_sum += (int64_t)I * I;
//        Psum    += (int64_t)V * I;
//    }
//
//    int32_t Vrms = int_sqrt64(Vsq_sum / N);
//    int32_t Irms = int_sqrt64(Isq_sum / N);
//    int32_t RealPower = Psum / N;
//    int32_t ApparentPower = ((int64_t)Vrms * Irms);
//
//    int32_t PF_x1000 = 0;  // Power factor * 1000
//    if (ApparentPower != 0)
//        PF_x1000 = (int32_t)((RealPower * 1000LL) / ApparentPower);
//
//    AC_Results_t result;
//    result.Vrms = Vrms;
//    result.Irms = Irms;
//    result.RealPower = RealPower;
//    result.ApparentPower = ApparentPower;
//
//    result.Vsq_sum = Vsq_sum;
//    result.Isq_sum = Isq_sum;
//    result.SampleNum = N;
//    result.Duration_us = duration_us;
//
//    result.PF_x1000 = PF_x1000; // could be negative if - real power (apparent power always +)
//    result.PhaseAngle_deg_x10 = calculate_signed_phase_deg_x10(V_buf, I_buf, N, PF_x1000);
//
//    result.Frequency_Hz_x100 = calc_frequency_hz(zc_count, duration_us);
//
//    return result;
//}
//
//
//
//
//
//
//
//
//
//
//

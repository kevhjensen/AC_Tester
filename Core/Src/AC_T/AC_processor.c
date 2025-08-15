/*
 * AC_processor.c
 *
 *  Created on: Jul 17, 2025
 *      Author: kevinj
 */

#include "AC_T_globals.h"

volatile int32_t CT_mA_rms_setpoint = 0; // set this value to simulate current
volatile float CT_A_rms_setpoint = 0.0f;


volatile float CT_phase_us_offset = 0;



uint8_t update_CT_mA_setpoint(int32_t new_mA_setpoint) // may need to add limits, negative value means flowing into charger
{
	CT_mA_rms_setpoint = new_mA_setpoint;
	CT_A_rms_setpoint = (float)CT_mA_rms_setpoint / 1000.0f;
	return 1;
}

uint8_t update_CT_phase_us_offset(int32_t new_CT_phase_us_offset) // may need to add limits, positive for leading current
{
	CT_phase_us_offset = (float)new_CT_phase_us_offset;
	return 1;
}



/*
 * buffer 0 index is point after zero crossing after initial buffer processed (settling time)
 *
 * last V sample(V_bufX_head - 1) will be point after 3rd zero crossing
 * This sample also written to start of new buffer
 *
 * process 0 to V_bufX_head - 2
 *
 * Even though BufferFull called before getting a I sample in 2ch mode,
 * 	this is fine since last data point (head - 1) discarded. Next I sample will go to start of buffer and match timewise
 *
 */

int32_t V_buf1[AC_BUFFER_SIZE]; // raw values
int32_t V_buf2[AC_BUFFER_SIZE]; //write to a buffer while process the other one
int32_t I_buf1[AC_BUFFER_SIZE];
int32_t I_buf2[AC_BUFFER_SIZE];

float I_buf1_emul[AC_BUFFER_SIZE]; // scaled to actual Amps
float I_buf2_emul[AC_BUFFER_SIZE];


volatile uint8_t sense_mode = 0;  //0 for emulate mode, 1 for sense mode (read 2ch)
uint8_t update_Sense_Mode(uint8_t new_sense_mode)
{
	if (new_sense_mode > 1)
			return 0;

	sense_mode = new_sense_mode;
	ADS1220_read_both_CHs = sense_mode; // read 2 chs in sense mode


	memset(V_buf1, 0, sizeof(V_buf1));
	memset(V_buf2, 0, sizeof(V_buf2));
	memset(I_buf1, 0, sizeof(I_buf1));
	memset(I_buf2, 0, sizeof(I_buf2));

	memset(I_buf1_emul, 0, sizeof(I_buf1_emul));
	memset(I_buf2_emul, 0, sizeof(I_buf2_emul));

	AC_EnergyRegister_Wh = 0;
	CT_mA_rms_setpoint = 0;
	CT_A_rms_setpoint = 0.0f;

	return 1;
}


volatile uint16_t V_buf1_head = 0; // index to write new samples to; when ready it gives numSamples in buffer
volatile uint16_t V_buf2_head = 0;

// which buffer currently writing to 1 for 1, 2 for 2; other buffer for CT emulation / processing
volatile uint8_t V_buf_to_write = 1;
volatile uint8_t buf_rdy_to_process = 0; // flag for main loop to process opposite of write buffer







volatile uint32_t lastTimeElapsed = 0; // time Elapsed of last sample

volatile uint32_t timeElapsed = 0; // time elapsed on !v_buf_to_write since its first sample (after a ZC)
volatile uint32_t to_process_time = 0; // last buffer timeElapsed, sample after zc0 to after



// time relative to timeElapsed = 0/ buf index 0 at exact zc times, index 0 is copy of last end last
// index 0 is negative, since zc0 is before first sample in buffer
float ZC_times_buf1[NUM_CYCLES + 1];
float ZC_times_buf2[NUM_CYCLES + 1];


volatile int32_t last_V_sample = 0;
volatile int32_t last_I_sample = 0;

volatile int8_t last_sample_sign = 0;

volatile uint8_t zero_cross_count = 0;
volatile uint8_t V_buf_to_process_zc_count = 0;


// data updated in processACdata, includes reference signal for CT emulate
volatile AC_Results_float cur_AC_data = {0};
volatile float AC_EnergyRegister_Wh = 0;

int32_t ref_V_buf[AC_BUFFER_SIZE];
float ref_ZC_times_buf[NUM_CYCLES + 1];



float interpolate_v_at_time(
    const int32_t *buf,
    size_t len,
    float dt,        // time between samples
    float t_query    // time since buf[0] (same units as dt)
) {
    // Find index just before t_query
    float idx_f = t_query / dt;      // fractional index
    int idx = (int)floorf(idx_f);

    // Clamp to array range
    if (idx < 0) {
        return (float)buf[0];
    }
    if (idx >= (int)len - 1) {
        return (float)buf[len - 1];
    }

    // Fraction between two samples
    float frac = idx_f - (float)idx;

    // Linear interpolation
    float v0 = (float)buf[idx];
    float v1 = (float)buf[idx + 1];
    return v0 + frac * (v1 - v0);
}

// references last buffer, using its middle ZC time (for 3 cycles: 1, 5:2, 10:5)
// uses the current timeElapsed since recent ZC, and interpolates V point from last buffer
float emulateCT_Signal(int32_t sample, uint32_t timeElapsed) // timeElapsed since sample after ZC0
{
	float timeSinceLastZC; // time since sample after this period's post ZC sample
	float timeSinceRefBufferStart;
	float refV = 0; // raw voltage, scale to get current out

	if (CT_A_rms_setpoint == 0.0f || cur_AC_data.zc_count < NUM_CYCLES || cur_AC_data.VrmsRaw == 0.0f)
	{
		MCP4725_DP_DN_setValue(2048); // 0 current
		return 0.0f;
	}

	if (V_buf_to_write == 1)
		timeSinceLastZC = (float)timeElapsed - ZC_times_buf1[zero_cross_count];
	else
		timeSinceLastZC = (float)timeElapsed - ZC_times_buf2[zero_cross_count];

	timeSinceRefBufferStart = ref_ZC_times_buf[NUM_CYCLES/2] + timeSinceLastZC + CT_phase_us_offset;
	timeSinceRefBufferStart += cur_AC_data.avgSampleTime; // offset by a sample time since output aligns with next V sample

	refV = interpolate_v_at_time(ref_V_buf, cur_AC_data.SampleNum, cur_AC_data.avgSampleTime, timeSinceRefBufferStart);


	float CT_A = CT_A_rms_setpoint * (refV / cur_AC_data.VrmsRaw); // (~ -1.41 to 1.41 range) magnitude relative to RMS

	uint16_t value = MCP_DP_DN_Value_from_A_setpoint(CT_A); // TO DO: recompute CT_A for actual output
	MCP4725_DP_DN_setValue(value);

	float output_A = actual_A_from_MCP_DP_DN_Value(value); // actual output from dac

	return output_A;
}




void bufferFull(int32_t dupSample, float dupCurrent) // switch V_buf_to_write, set to_process flag, dupSample at start of new buf
{
	to_process_time = timeElapsed;
	timeElapsed = 0;

	V_buf_to_process_zc_count = zero_cross_count;
	zero_cross_count = 0;

	if (V_buf_to_write == 1)
	{
		buf_rdy_to_process = 1;
		V_buf_to_write = 2;

		V_buf2_head = 0; // reset new write buffer
		I_buf2_emul[V_buf2_head] = dupCurrent;
		V_buf2[V_buf2_head++] = dupSample; // start with duplicate post zc value

		ZC_times_buf2[0] = ZC_times_buf1[NUM_CYCLES] - (float)to_process_time; // negative value
	}
	else
	{
		buf_rdy_to_process = 1;
		V_buf_to_write = 1;

		V_buf1_head = 0;
		I_buf1_emul[V_buf1_head] = dupCurrent;
		V_buf1[V_buf1_head++] = dupSample;

		ZC_times_buf1[0] = ZC_times_buf2[NUM_CYCLES] - (float)to_process_time;
	}
}


float interpolate_zero_crossing_time(
    int32_t last_V_sample,
    int32_t newSample,
    uint32_t lastTime,
    uint32_t curTime
) {
    int32_t slope = newSample - last_V_sample;

    if (slope == 0) {
        return lastTime; // avoid divide by zero, return start time
    }

    // Fraction of the way from last sample to zero crossing
    // frac = -last_V_sample / slope
    float frac = -(float)last_V_sample / (float)slope;

    // Linear interpolation for time
    float t_cross = (float)lastTime + frac * (float)(curTime - lastTime);

    return t_cross; // round to nearest
}


/* call after already writing newSample
 * If sample is right after 11th zero crossing, it will also be written to the new buffer
 */
void processRisingZeroCrossing(int32_t newSample, float last_CT_emulate_A, uint32_t lastTimeElapsed, uint32_t timeElapsed)
{
    int8_t current_sign = (newSample >= 0) ? 1 : -1;

    // Check for rising edge
    if (last_sample_sign < 0 && current_sign >= 0)
    {
    	zero_cross_count++;

    	if (V_buf_to_write == 1)
		{
    		ZC_times_buf1[zero_cross_count] = interpolate_zero_crossing_time(last_V_sample, newSample, lastTimeElapsed, timeElapsed);
		}
		else
		{
			ZC_times_buf2[zero_cross_count] = interpolate_zero_crossing_time(last_V_sample, newSample, lastTimeElapsed, timeElapsed);
		}



        if (zero_cross_count >= NUM_ZC)
        {
            bufferFull(newSample, last_CT_emulate_A);
        }
    }

    last_V_sample = newSample;
    last_sample_sign = current_sign;
}




volatile uint16_t last_sample_time = 0; // last sample time (uint16 micros)
volatile uint16_t cur_sample_time_elapsed = 0; // amount of time for sample to come

float last_CT_emulate_A = 0;
float cur_CT_emulate_A = 0;

void new_ADS_AC_V_sample(int32_t sample, uint16_t sampleTime) // ADS1220 interrupt fired, so keep lightweight
{
	cur_sample_time_elapsed = sampleTime - last_sample_time;
	timeElapsed += cur_sample_time_elapsed;

	if (sense_mode == 0)
	{
		cur_CT_emulate_A = emulateCT_Signal(sample, timeElapsed);
	}
    if (V_buf_to_write == 1)
    {
        if (V_buf1_head < AC_BUFFER_SIZE)
        {
        	I_buf1_emul[V_buf1_head] = last_CT_emulate_A;
            V_buf1[V_buf1_head++] = sample; // write to head index then increment
        }
        else
        {
        	bufferFull(sample, last_CT_emulate_A); // process data even though no 60hz signal
        }
    }
    else
    {
        if (V_buf1_head < AC_BUFFER_SIZE)
        {
        	I_buf2_emul[V_buf2_head] = last_CT_emulate_A;
            V_buf2[V_buf2_head++] = sample; // write to head index then increment
        }
        else
        {
        	bufferFull(sample, last_CT_emulate_A); // process data even though no 60hz signal
        }
    }

	processRisingZeroCrossing(sample, last_CT_emulate_A, lastTimeElapsed, timeElapsed); // sample index is head - 1 at this point

	last_sample_time = sampleTime;
	lastTimeElapsed = timeElapsed;
	last_V_sample = sample;
	last_CT_emulate_A = cur_CT_emulate_A;

	V_numSamples++;
}

void new_ADS_AC_I_sample(int32_t sample, uint16_t sampleTime) // handles I current values when ADS1220_read_both_CHs = 1, values alternate with V time wise
{
	cur_sample_time_elapsed = sampleTime - last_sample_time;
	last_sample_time = sampleTime;

	timeElapsed += cur_sample_time_elapsed;

	// Interpolate midpoint, so I sample time aligns with V (alternating sampling)
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

}


void analyze_AC_waveform(int32_t* V_buf,
							 int32_t* I_buf,
							 uint16_t N,
							 uint8_t zc_count,
							 uint32_t duration_us,
							 float* zc_buf)
{
    uint64_t Vsq_sum = 0;
    uint64_t Isq_sum = 0;
    int64_t  Psum_i64    = 0;

    for (uint16_t i = 0; i < N; i++) {
        int32_t V = V_buf[i];
        int32_t I = I_buf[i];

        Vsq_sum += (uint64_t)((int64_t)V * (int64_t)V);
        Isq_sum += (uint64_t)((int64_t)I * (int64_t)I);
        Psum_i64    += (int64_t)V * (int64_t)I;

    }

    // Convert to float only at the final step
    float invN = (N > 0) ? (1.0f / (float)N) : 0.0f;

    float VrmsRaw = sqrtf((float)Vsq_sum * invN);
    float IrmsRaw = sqrtf((float)Isq_sum * invN);

    float Vrms = AC_V_from_raw(VrmsRaw);
    float Irms = AC_I_from_raw(IrmsRaw);
    float RealPower = AC_I_from_raw(AC_V_from_raw((float)Psum_i64 * invN));
    float ApparentPower = Vrms * Irms;

    AC_EnergyRegister_Wh += (RealPower * (float)duration_us) / 3600000000.0f;

    float PF = 0.0f;
    if (ApparentPower != 0.0f) {
        PF = RealPower / ApparentPower;
    }
    float freq = 0.0f;
    if (zc_count < 1 || duration_us == 0)
    	freq = 0.0f;
    else
    	freq = ((float)zc_count * 1.0e6f) / (zc_buf[zc_count] - zc_buf[0]);

    float avgSampleTime = (float)duration_us * invN;

    // Populate results
    cur_AC_data.Vrms          = Vrms;
    cur_AC_data.Irms          = Irms;
    cur_AC_data.RealPower_W     = RealPower;
    cur_AC_data.ApparentPower_W = ApparentPower;

    cur_AC_data.VrmsRaw       = VrmsRaw;
    cur_AC_data.Vsq_sum_raw       = Vsq_sum;
    cur_AC_data.Isq_sum_raw       = Isq_sum;

    cur_AC_data.SampleNum     = N;
    cur_AC_data.Duration_us   = duration_us;

    cur_AC_data.PF            = PF;
    cur_AC_data.Frequency_Hz  = freq;

	cur_AC_data.avgSampleTime = avgSampleTime;
	cur_AC_data.zc_count = zc_count;

}

void analyze_AC_waveform_emulate(int32_t* V_buf,
							 float* I_buf,
							 uint16_t N,
							 uint8_t zc_count,
							 uint32_t duration_us,
							 float* zc_buf)
{
    uint64_t Vsq_sum = 0;
    float Isq_sum = 0;
    float  Psum    = 0;


    for (uint16_t i = 0; i < N; i++) {
        int32_t V = V_buf[i];
        float I = I_buf[i];

        Vsq_sum += (uint64_t)((int64_t)V * (int64_t)V);
        Isq_sum += I*I;
        Psum    += (float)V * I;

    }

    // Convert to float only at the final step
    float invN = (N > 0) ? (1.0f / (float)N) : 0.0f;

    float VrmsRaw = sqrtf((float)Vsq_sum * invN);

    float Irms = sqrtf((float)Isq_sum * invN);

    float Vrms = AC_V_from_raw(VrmsRaw);
    // float Irms = AC_I_from_raw(IrmsRaw);
    float RealPower = AC_V_from_raw(Psum * invN);
    float ApparentPower = Vrms * Irms;

    AC_EnergyRegister_Wh += (RealPower * (float)duration_us) / 3600000000.0f;

    float PF = 0.0f;
    if (ApparentPower != 0.0f) {
        PF = RealPower / ApparentPower;
    }
    float freq = 0.0f;
    if (zc_count < 1 || duration_us == 0)
    	freq = 0.0f;
    else
    	freq = ((float)zc_count * 1.0e6f) / (zc_buf[zc_count] - zc_buf[0]);

    float avgSampleTime = (float)duration_us * invN;

    // Populate results
    cur_AC_data.Vrms          = Vrms;
    cur_AC_data.Irms          = Irms;
    cur_AC_data.RealPower_W     = RealPower;
    cur_AC_data.ApparentPower_W = ApparentPower;

    cur_AC_data.VrmsRaw       = VrmsRaw;

    cur_AC_data.SampleNum     = N;
    cur_AC_data.Duration_us   = duration_us;

    cur_AC_data.PF            = PF;
    cur_AC_data.Frequency_Hz  = freq;

	cur_AC_data.avgSampleTime = avgSampleTime;
	cur_AC_data.zc_count = zc_count;

	memcpy(ref_V_buf, V_buf, N * sizeof(V_buf[0]));
	memcpy(ref_ZC_times_buf, zc_buf, (NUM_CYCLES + 1) * sizeof(zc_buf[0]));

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
	if (buf_rdy_to_process == 0) {
		return;
	}
	buf_rdy_to_process = 0;

	if (V_buf_to_write == 2) {
		if (sense_mode == 0) {
			analyze_AC_waveform_emulate(V_buf1, I_buf1_emul, V_buf1_head - 1, V_buf_to_process_zc_count, to_process_time, ZC_times_buf1);
		}
		else {
			analyze_AC_waveform(V_buf1, I_buf1, V_buf1_head - 1, V_buf_to_process_zc_count, to_process_time, ZC_times_buf1);
		}
	}
	else {
		if (sense_mode == 0) {
			analyze_AC_waveform_emulate(V_buf2, I_buf2_emul, V_buf2_head - 1, V_buf_to_process_zc_count, to_process_time, ZC_times_buf2);
		}
		else {
			analyze_AC_waveform(V_buf2, I_buf2, V_buf2_head - 1, V_buf_to_process_zc_count, to_process_time, ZC_times_buf2);
		}
	}
	snprintf(buf, sizeof(buf),
	         "%lu, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f\r\n",
	         HAL_GetTick(),
	         cur_AC_data.Vrms,           // Vrms in volts
	         cur_AC_data.Irms,           // Irms in amps
	         cur_AC_data.RealPower_W/1000.0f,      // kWs

	         cur_AC_data.PF,             // Power Factor
			 cur_AC_data.Frequency_Hz,
	         AC_EnergyRegister_Wh
	);


	if (data_mode == 7 || data_mode == 8) {
		CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
	}

	if (data_mode == 10 && V_buf_to_write == 1) {

		for (uint16_t i=0; i<V_buf2_head - 1; i++) {
			snprintf(buf, sizeof(buf),
			         "%u, %.3f, %.3f, %.3f\r\n",
			         i,
					 AC_V_from_raw(V_buf2[i]),
					 I_buf2_emul[i],
					 AC_I_from_raw(I_buf2[i])

			);
			CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
		}
	}
	if (data_mode == 10 && V_buf_to_write == 2) {

			for (uint16_t i=0; i<V_buf1_head - 1; i++) {
				snprintf(buf, sizeof(buf),
				         "%u, %.3f, %.3f, %.3f\r\n",
				         i,
						 AC_V_from_raw(V_buf1[i]),
						 I_buf1_emul[i],
						 AC_I_from_raw(I_buf1[i])
				);
				CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
			}
		}
}













/*
 * myScheduler.c
 *
 *  Created on: Jul 19, 2025
 *      Author: kevinj
 */


#include "AC_T_globals.h"

uint32_t canary1;
uint32_t currentTime;
uint32_t canary2;
uint32_t lastTime1s;
uint32_t canary3;
uint32_t lastTime100ms;
uint32_t lastTime200ms;
uint32_t lastTime30ms;
uint32_t lastTime10ms;
uint32_t lastTime1ms;
uint32_t canary4;
uint32_t nCycles30ms;
uint32_t canary5;


/**********************************************************/
/* The tasks */

/* This task runs each 1ms(but may be slower if other tasks are busy). */
void task1ms(void) {
	processACdata10ms();
	processCpADCdata10ms();
}

/* This task runs each 10ms. */
void task10ms(void) {

}


/* This task runs each 30ms. */
void task30ms(void) {

}

/* This task runs each 100ms. */
void task100ms(void) {

}

/* This task runs each 200ms. */
void task200ms(void) {

}

/* This task runs once a second. */
void task1s(void) {

	if (data_mode == 5) {
				  snprintf(buf, sizeof(buf),
						   "%lu, cp_H_L_D_F, %li, %li, %lu, %lu\r\n",
						   HAL_GetTick(),
						   cp_V_x100_from_raw(evse_cp_high),   // Raw ADC high value
						   cp_V_x100_from_raw(evse_cp_low),             // Raw ADC low value
						   evse_cp_duty_x100,       // Duty cycle ×100 (e.g., 8000 = 80.00%)
						   evse_cp_freq_x10);       // Frequency ×10 (e.g., 10000 = 1000.0 Hz)
				  CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
			  }

	  if (data_mode == 11) {
		  snprintf(buf, sizeof(buf), "SamplesPerSecond: %u, %u\r\n", V_numSamples, I_numSamples); // Convert int to string with newline
		  CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
	  }
	  V_numSamples = 0;
	  I_numSamples = 0;

  uint8_t blStop=0;
  if (canary1!=0x55AA55AA) { //magic number, checks for stack overflow
	  sprintf(strTmp, "canary1 corrupted");
	  addToTrace(strTmp);
	  blStop=1;
  }
  if (canary2!=0x55AA55AA) {
	  sprintf(strTmp, "canary2 corrupted");
	  addToTrace(strTmp);
	  blStop=1;
  }
  if (canary3!=0x55AA55AA) {
	  sprintf(strTmp, "canary3 corrupted");
	  addToTrace(strTmp);
	  blStop=1;
  }
  if (canary4!=0x55AA55AA) {
	  sprintf(strTmp, "canary4 corrupted");
	  addToTrace(strTmp);
	  blStop=1;
  }
  if (canary5!=0x55AA55AA) {
	  sprintf(strTmp, "canary5 corrupted");
	  addToTrace(strTmp);
	  blStop=1;
  }
  if (blStop) {
	  while (1) { }
  }
}

void initMyScheduler(void) {
	addToTrace("=== AC_Tester started. Version 2025-07-18 ===");
	canary1 = 0x55AA55AA;
	canary2 = 0x55AA55AA;
	canary3 = 0x55AA55AA;
	canary4 = 0x55AA55AA;
	canary5 = 0x55AA55AA;
}

void runMyScheduler(void) {
  /* a simple scheduler which calls the cyclic tasks depending on system time */
  currentTime = HAL_GetTick(); /* HAL_GetTick() returns the time since startup in milliseconds */
  if (currentTime<lastTime30ms) {
	    sprintf(strTmp, "Error: current time jumped back. current %ld, last %ld", currentTime, lastTime30ms);
	    addToTrace(strTmp);
	    while (1) { }
  }
  if ((currentTime - lastTime10ms)>10) {
    lastTime10ms += 10;
    task10ms();
  }
  if ((currentTime - lastTime1ms)>1) {
    lastTime1ms += 1;
    task1ms();
  }
  if ((currentTime - lastTime30ms)>30) {
    lastTime30ms += 30;
    task30ms();
  }
  if ((currentTime - lastTime100ms)>100) {
    lastTime100ms += 100;
    task100ms();
  }
  if ((currentTime - lastTime200ms)>200) {
    lastTime200ms += 200;
    task200ms();
  }
  if ((currentTime - lastTime1s)>1000) {
    lastTime1s += 1000;
    task1s();
  }
}

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
uint32_t lastTime500ms;
uint32_t lastTime30ms;
uint32_t lastTime20ms;
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
	processCpADCdata1ms();
}

/* This task runs each 10ms. */
void task10ms(void) {
	processEV_stateMachine10ms();
}


/* This task runs each 50ms. */
void task20ms(void) {

}
/* This task runs >30ms after last time ran, needed for AC detection logic
 * Otherwise, sometimes will run late, then next one will run on time
 * */
void task30msApart(void) {
	process_30ms_isMoreThan60Vac_onChargerOut();
}

/* This task runs each 100ms. */
void task100ms(void) {

}

/* This task runs each 200ms. */
void task200ms(void) {

}

void task500ms(void) {
	if (data_mode == 8) {
		snprintf(buf, sizeof(buf),
			 "EV CP %u, %u",
			 ev_cp_debounce_read_state,
			 ev_cp_cur_amp_limit_x100
			 );
		addToMsgQ(buf);

		snprintf(buf, sizeof(buf),
			 "AC %.3f, %.3f, %.3f, %.4f, %u",
			 cur_AC_data.Vrms,           // Vrms in volts
			 cur_AC_data.Irms,           // Irms in amps
			 cur_AC_data.RealPower_W/1000.0f,      // kWs
			 AC_EnergyRegister_Wh/1000.0f,
			 isMoreThan60Vac_onChargerOut
			 );
		addToMsgQ(buf);
	}

	if (data_mode == 9) {
		snprintf(buf, sizeof(buf),
				 "EVSE CP %u, %u, %u, %u, %li, %li, %lu, %lu, %u",
				 evse_cp_debounce_read_state,
				 evse_cp_cur_read_state,
				 evse_cp_cur_amp_limit_x100,
				 evse_isPLC,
				 evse_cp_high_Vx100,
				 evse_cp_low_Vx100,
				 evse_cp_duty_x100,
				 evse_cp_freq_x10,
				 isMoreThan60Vac_onChargerOut
		);
		addToMsgQ(buf);

		snprintf(buf, sizeof(buf),
				 "EV CP %u, %u, %u, %u, %li, %li, %lu, %lu",
				 ev_cp_debounce_read_state,
				 ev_cp_cur_read_state,
				 ev_cp_cur_amp_limit_x100,
				 ev_isPLC,
				 ev_cp_high_Vx100,
				 ev_cp_low_Vx100,
				 ev_cp_duty_x100,
				 ev_cp_freq_x10
		);
		addToMsgQ(buf);

		snprintf(buf, sizeof(buf),
			 "AC %.3f, %.3f, %.3f, %.3f, %.2f, %.4f",
			 cur_AC_data.Vrms,           // Vrms in volts
			 cur_AC_data.Irms,           // Irms in amps
			 cur_AC_data.RealPower_W/1000.0f,      // kWs
			 cur_AC_data.PF,             // Power Factor
			 cur_AC_data.Frequency_Hz,
			 AC_EnergyRegister_Wh/1000.0f
			 );
		addToMsgQ(buf);
	}
}

/* This task runs once a second. */
void task1s(void) {

	  if (data_mode == 102) {
		  snprintf(buf, sizeof(buf), "SamplesPerSecond: %u, %u\r\n", V_numSamples, I_numSamples); // Convert int to string with newline
		  addToMsgQ(buf);
	  }
	  V_numSamples = 0;
	  I_numSamples = 0;

  uint8_t blStop=0;
  if (canary1!=0x55AA55AA) { //magic number, checks for stack overflow
	  sprintf(buf, "canary1 corrupted");
	  addToMsgQ(buf);
	  blStop=1;
  }
  if (canary2!=0x55AA55AA) {
	  sprintf(buf, "canary2 corrupted");
	  addToMsgQ(buf);
	  blStop=1;
  }
  if (canary3!=0x55AA55AA) {
	  sprintf(buf, "canary3 corrupted");
	  addToMsgQ(buf);
	  blStop=1;
  }
  if (canary4!=0x55AA55AA) {
	  sprintf(buf, "canary4 corrupted");
	  addToMsgQ(buf);
	  blStop=1;
  }
  if (canary5!=0x55AA55AA) {
	  sprintf(buf, "canary5 corrupted");
	  addToMsgQ(buf);
	  blStop=1;
  }
  if (blStop) {
	  while (1) { }
  }
}

void initMyScheduler(void) {
	addToMsgQ("=== AC_Tester started. Version 2025-07-18 ===");
	canary1 = 0x55AA55AA;
	canary2 = 0x55AA55AA;
	canary3 = 0x55AA55AA;
	canary4 = 0x55AA55AA;
	canary5 = 0x55AA55AA;
}

void runMyScheduler(void) {
  /* a simple scheduler which calls the cyclic tasks depending on system time */
  currentTime = HAL_GetTick(); /* HAL_GetTick() returns the time since startup in milliseconds */
  if (currentTime<lastTime20ms) {
	    sprintf(buf, "Error: current time jumped back. current %ld, last %ld", currentTime, lastTime20ms);
	    addToMsgQ(buf);
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
  if ((currentTime - lastTime20ms)>20) {
    lastTime20ms += 20;
    task20ms();
  }

  if ((currentTime - lastTime30ms)>30) {
      lastTime30ms = currentTime;
      task30msApart();
  }

  if ((currentTime - lastTime100ms)>100) {
    lastTime100ms += 100;
    task100ms();
  }
  if ((currentTime - lastTime200ms)>200) {
    lastTime200ms += 200;
    task200ms();
  }
  if ((currentTime - lastTime500ms)>500) {
      lastTime500ms += 500;
      task500ms();
  }
  if ((currentTime - lastTime1s)>1000) {
    lastTime1s += 1000;
    task1s();
  }
}

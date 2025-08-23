/*
 * EV_StateMachine.c
 *
 *  Created on: Jul 29, 2025
 *      Author: kevinj
 */

#include "AC_T_globals.h"


// 0	idle    / unplugged        / target state: EVUnconnected
// 1	plug_in / EV can charge    / target state: EVCharging
// 2	plug_in / EV can't charge  / target state: EVSEready
volatile uint8_t vehicle_control = 0;
volatile uint8_t isVehicleControlNotManual = 0;
volatile uint8_t vehicle_amp_limit = 80;

uint8_t update_vehicle_amp_limit(uint8_t new_vehicle_amp_limit)
{
	vehicle_amp_limit = new_vehicle_amp_limit;
	return 1;
}


uint8_t cur_ev_state;
uint8_t cur_evse_state;

uint8_t last_ev_state;
uint8_t last_evse_state;

//last_ev_state = cur_ev_state;
//last_evse_state = cur_evse_state;

// cp_cur_set_state
void initEvStateMachine(void)
{
	set_cp_state(0);
	HAL_Delay(500);
	cur_ev_state = ev_cp_cur_read_state;
	cur_evse_state = evse_cp_debounce_read_state;
}

enum {
	Unplugged   = 0,
	PluggedIn = 1,
	DidNotUnplugFault = 2,
	DidNotPlugInFault = 3,
};
uint8_t PluggedInWrite_state; // set_cp_state(0) unplugged, if 1 or 2 pluggedIn
uint8_t PluggedInRead_state;

void plugIn(void) { // attempts to plug in
	set_cp_state(1);
	PluggedInWrite_state = PluggedIn;
}
void unPlug(void) { // attempts to unplug
	set_cp_state(0);
	PluggedInWrite_state = Unplugged;
}
void updatePluggedInRead_state(void)
{
	if (PluggedInWrite_state == 1) // 1 should be pluggedIn
	{
		if (cur_ev_state == CP_STATE_A && (cur_evse_state == CP_STATE_F || cur_evse_state == CP_STATE_E)) // only valid mismatch
			PluggedInRead_state = PluggedIn;
		else if (cur_ev_state == cur_evse_state) // should match if plugged in
			PluggedInRead_state = PluggedIn;
		else
			PluggedInRead_state = DidNotUnplugFault;
	}
	else // should be unplugged
	{
		if (cur_ev_state == CP_STATE_A)
			PluggedInRead_state = Unplugged;
		else
			PluggedInRead_state = DidNotPlugInFault;
	}
}
void evSetStateB(void) { // only if pluggedIn
	set_cp_state(1);
}
void evSetStateC(void) { // only if pluggedIn
	set_cp_state(2);
}

void runEvStateMachine(void)
{

	cur_ev_state = ev_cp_cur_read_state;
	cur_evse_state = evse_cp_debounce_read_state;




}








void processEV_stateMachine10ms(void)
{
	if (isVehicleControlNotManual)
	{
		runEvStateMachine();
	}

}



uint8_t update_vehicle_control(uint8_t new_vehicle_control)
{
	if (new_vehicle_control > 3)
			return 0;

	if (isVehicleControlNotManual == 0) {
		initEvStateMachine();
	}
	isVehicleControlNotManual = 1;
	vehicle_control = new_vehicle_control;
	return 1;
}




uint32_t chargeSessionStatIntervalMS = 1000; // log charging data every 1s
uint32_t chargeSessionStartTime;
float chargeSessionStartWh;
uint32_t lastTimePrint = 0;
void startChargeSession(void)
{
	chargeSessionStartTime = HAL_GetTick();
	chargeSessionStartWh = AC_EnergyRegister_Wh;
	lastTimePrint = HAL_GetTick();
}
void printChargeSessionEnded(void)
{
	float timeElapsedSeconds = (float)(HAL_GetTick() - chargeSessionStartTime) / 1000;
	sprintf(buf, "Session Energy:%.4fkWh, Time:%.1fs",
			(AC_EnergyRegister_Wh - chargeSessionStartWh)/1000.0f,
			timeElapsedSeconds);
	addToMsgQ(buf);
}
void printChargingStats(void)
{
	if ((HAL_GetTick() - lastTimePrint) > chargeSessionStatIntervalMS) {
		lastTimePrint += chargeSessionStatIntervalMS;

		sprintf(buf, "[%.0fs]EVcurrent/CPlimit: %.1f/%.1fA, Power:%.2fkW",
					(float)(HAL_GetTick() - chargeSessionStartTime) / 1000,
					cur_AC_data.Irms,
					(float)evse_cp_cur_amp_limit_x100/100.0f,
					cur_AC_data.RealPower_W/1000.0f
					);
		addToMsgQ(buf);
	  }
}





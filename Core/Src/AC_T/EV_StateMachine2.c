///*
// * EV_StateMachine.c
// *
// *  Created on: Jul 29, 2025
// *      Author: kevinj
// */
//
//#include "AC_T_globals.h"
//
//
//// 0	idle    / unplugged        / target state: EVUnconnected
//// 1	plug_in / EV can charge    / target state: EVCharging
//// 2	plug_in / EV can't charge  / target state: EVSEready
//volatile uint8_t vehicle_control = 0;
//volatile uint8_t isVehicleControlNotManual = 1;
//volatile uint8_t vehicle_amp_limit = 80;
//
//uint8_t update_vehicle_amp_limit(uint8_t new_vehicle_amp_limit)
//{
//	vehicle_amp_limit = new_vehicle_amp_limit;
//	return 1;
//}
//
//// main process happens at 10ms, so waits for 5 process ticks for a 50ms timeout
//
//// enumName, print text, timeout in ms (0 no timeout)
//
////WaitForEVChargeStopped timeout 150ms for AC power to be cut (!! important when unplugging quickly)
//
//// ===================== State List =====================
//#define STATE_LIST \
//   STATE_ENTRY(EvUnconnectedFaultEvse,       "EvUnconnectedFaultEvse",       0    ) \
//   STATE_ENTRY(EvUnconnectedEvseOff,         "EvUnconnectedEvseOff",         0    ) \
//   STATE_ENTRY(WaitForEvUnconnectedEvseOn,   "WaitForEvUnconnectedEvseOn, State:A",   1000 ) \
//   STATE_ENTRY(EvUnconnectedEvseOn,          "EvUnconnectedEvseOn",          0    ) \
//   STATE_ENTRY(WaitForEvConnectedIdle,       "WaitForEvConnectedIdle, State:B1/B2",       1000  ) \
//   STATE_ENTRY(EvConnectedIdle,              "EvConnectedIdle, State:B1/B2",              0    ) \
//   STATE_ENTRY(WaitForEvseReady,             "WaitForEvseReady, State:B2",             0    ) \
//   STATE_ENTRY(EvseReady,                    "EvseReady",                    0    ) \
//   STATE_ENTRY(WaitForEvReady,               "WaitForEvReady, State:C",               1000  ) \
//   STATE_ENTRY(EvReady,                      "EvReady, State:C",                      0    ) \
//   STATE_ENTRY(WaitForEvseACpowerPresent,    "WaitForEvseACpowerPresent",    10000) \
//   STATE_ENTRY(EvseACpowerPresent,           "EvseACpowerPresent",           0    ) \
//   STATE_ENTRY(EvCharging,                   "EvCharging, State:C",                   0    ) \
//   STATE_ENTRY(WaitForEvseChargeStopped,     "WaitForEvseChargeStopped, State:B",     150  ) \
//   STATE_ENTRY(EvseChargeStopped,            "EvseChargeStopped",            0    ) \
//   STATE_ENTRY(WaitForEvChargeStopped,       "WaitForEvChargeStopped, State:B",       150  ) \
//   STATE_ENTRY(EvChargeStopped,              "EvChargeStopped",              0    ) \
//   STATE_ENTRY(EvConnectedEvseOff,           "EvConnectedEvseOff, State:E",           0    ) \
//   STATE_ENTRY(EvConnnectedFaultEvOrEvse,    "EvConnnectedFaultEvOrEvse",    0    )
//
//
//
//// ===================== Enum =====================
//#define STATE_ENTRY(name, fname, timeout_ms) PEV_STATE_##name,
//typedef enum {
//   STATE_LIST
//   PEV_STATE_COUNT
//} pevstates;
//#undef STATE_ENTRY
//
//// ===================== Prototypes (one per state) =====================
//#define STATE_ENTRY(name, fname, timeout_ms) static void stateFunction##name(void);
//STATE_LIST
//#undef STATE_ENTRY
//
//// ===================== Function pointer table =====================
//typedef void (*StateFn)(void);
//#define STATE_ENTRY(name, fname, timeout_ms) stateFunction##name,
//static StateFn const stateFunctions[PEV_STATE_COUNT] = {
//   STATE_LIST
//};
//#undef STATE_ENTRY
//
//// ===================== Timeout table (store milliseconds) =====================
//#define STATE_ENTRY(name, fname, timeout_ms) ((uint32_t)(timeout_ms)),
//static const uint32_t timeouts_ms[PEV_STATE_COUNT] = {
//   STATE_LIST
//};
//#undef STATE_ENTRY
//
//// ===================== Print strings =====================
//#define STATE_ENTRY(name, fname, timeout_ms) fname,
//static const char * const pevSttLabels[PEV_STATE_COUNT] = {
//   STATE_LIST
//};
//#undef STATE_ENTRY
//
//
//
//// ===================== State & helpers =====================
//
//static uint16_t  pev_cyclesInState = 0;           // increments each 10 ms tick
//static pevstates pev_state         = PEV_STATE_WaitForEvUnconnectedEvseOn;
//static uint8_t cp_read_state_thisCycle;
//
//
//char faultOnStateBuf[64];
//char faultTypeBuf[64];
//char faultTimeOutBuf[64];
//
//uint32_t chargeSessionStatIntervalMS = 1000; // log charging data every 1s
//uint32_t chargeSessionStartTime;
//float chargeSessionStartWh;
//uint32_t lastTimePrint = 0;
//void startChargeSession(void)
//{
//	chargeSessionStartTime = HAL_GetTick();
//	chargeSessionStartWh = AC_EnergyRegister_Wh;
//	lastTimePrint = HAL_GetTick();
//}
//void printChargeSessionEnded(void)
//{
//	float timeElapsedSeconds = (float)(HAL_GetTick() - chargeSessionStartTime) / 1000;
//	sprintf(buf, "Session Energy:%.4fkWh, Time:%.1fs",
//			(AC_EnergyRegister_Wh - chargeSessionStartWh)/1000.0f,
//			timeElapsedSeconds);
//	addToMsgQ(buf);
//}
//void printChargingStats(void)
//{
//	if ((HAL_GetTick() - lastTimePrint) > chargeSessionStatIntervalMS) {
//		lastTimePrint += chargeSessionStatIntervalMS;
//
//		sprintf(buf, "[%.0fs]EVcurrent/CPlimit: %.1f/%.1fA, Power:%.2fkW",
//					(float)(HAL_GetTick() - chargeSessionStartTime) / 1000,
//					cur_AC_data.Irms,
//					(float)evse_cp_cur_amp_limit_x100/100.0f,
//					cur_AC_data.RealPower_W/1000.0f
//					);
//		addToMsgQ(buf);
//	  }
//}
//
//static inline void pev_enterState(pevstates n) {
//   pev_state = n;
//   pev_cyclesInState = 0;
//}
//
//static inline uint8_t pev_isTooLong(void) {
//   // main tick = 10 ms, so elapsed_ms = cycles * 10
//   uint32_t limit = timeouts_ms[pev_state];
//   return (limit > 0) && ((uint32_t)pev_cyclesInState * 10 > limit);
//}
//
//uint8_t isEvConnected(void) {
//    // EV is considered connected if in Idle → Charging range
//    return (pev_state >= PEV_STATE_EvConnectedIdle) ? 1 : 0;
//}
//
//
//
//void handleCpFault(void) {
//	sprintf(faultOnStateBuf, "FaultOn:%s", pevSttLabels[pev_state]);
//	sprintf(faultTypeBuf, "FaultType:CpRead:%u", cp_read_state_thisCycle);
//	sprintf(faultTimeOutBuf, "FaultTimeout:%lums", timeouts_ms[pev_state]);
//}
//
//void handleAcFault(void) {
//	sprintf(faultOnStateBuf, "FaultOn:%s", pevSttLabels[pev_state]);
//	sprintf(faultTypeBuf, "FaultType:AcOut:%u", cp_read_state_thisCycle);
//	sprintf(faultTimeOutBuf, "FaultTimeout:%lums", timeouts_ms[pev_state]);
//}
//
//
//uint8_t goToEvUnConnOrConnEvseOffIfStateE(void) {
//	if (evse_cp_debounce_read_state == CP_STATE_E) {
//		if (isEvConnected())
//			pev_enterState(PEV_STATE_EvConnectedEvseOff);
//		else {
//			pev_enterState(PEV_STATE_EvConnectedEvseOff);
//		}
//		return 1;
//	}
//	return 0;
//}
//
//uint8_t goToEvUnConnOrConnFaultIfStateDthruError(void) {
//	if (evse_cp_debounce_read_state >= CP_STATE_D && evse_cp_debounce_read_state <= CP_STATE_ERROR) {
//		if (isEvConnected())
//			pev_enterState(PEV_STATE_EvConnectedEvseOff);
//		else {
//			pev_enterState(PEV_STATE_EvConnectedEvseOff);
//		}
//		handleCpFault();
//		return 1;
//	}
//	return 0;
//}
//
//
//uint8_t printStateOnFirstCycle(void) {
//	if (pev_cyclesInState == 1) {
//		//addToMsgQ(pevSttLabels[pev_state]);
//		return 1;
//	}
//	return 0;
//}
//
//
//
//
//
//// ===================== State Functions =====================
//
//static void stateFunctionEvUnconnectedFaultEvse(void) {
//	if (printStateOnFirstCycle()) {
//		addToMsgQ(faultOnStateBuf);
//		addToMsgQ(faultTypeBuf);
//		addToMsgQ(faultTimeOutBuf);
//	}
//	update_CT_mA_setpoint(0);
//	set_cp_state(1);
//}
//
//static void stateFunctionEvUnconnectedEvseOff(void) {
//	if (cp_read_state_thisCycle != CP_STATE_E)
//		pev_enterState(PEV_STATE_WaitForEvUnconnectedEvseOn);
//}
//
//static void stateFunctionWaitForEvUnconnectedEvseOn(void) {
//
//	set_cp_state(0);
//	if (cp_read_state_thisCycle == CP_STATE_A)
//		pev_enterState(PEV_STATE_EvUnconnectedEvseOn);
//	handleCpFault();
//}
//
//static void stateFunctionEvUnconnectedEvseOn(void) {
//
//	printStateOnFirstCycle();
//
//	if (vehicle_control > 0)
//		pev_enterState(PEV_STATE_WaitForEvConnectedIdle);
//	else if (cp_read_state_thisCycle != CP_STATE_A) {
//		handleCpFault();
//		pev_enterState(PEV_STATE_EvUnconnectedFaultEvse);
//	}
//}
//
//static void stateFunctionWaitForEvConnectedIdle(void) {
//
//	set_cp_state(1);
//
//	if (cp_read_state_thisCycle == CP_STATE_B1 || cp_read_state_thisCycle == CP_STATE_B2)
//		pev_enterState(PEV_STATE_EvConnectedIdle);
//	handleCpFault();
//}
//
//static void stateFunctionEvConnectedIdle(void) {
//
//	printStateOnFirstCycle();
//
//	if (vehicle_control == 0)
//		pev_enterState(PEV_STATE_WaitForEvUnconnectedEvseOn);
//	else if (cp_read_state_thisCycle == CP_STATE_B1 || cp_read_state_thisCycle == CP_STATE_B2)
//		pev_enterState(PEV_STATE_WaitForEvseReady);
//	else {
//		handleCpFault();
//		pev_enterState(PEV_STATE_EvConnnectedFaultEvOrEvse);
//	}
//}
//
//static void stateFunctionWaitForEvseReady(void) {
//
//	printStateOnFirstCycle();
//
//	if (vehicle_control == 0)
//		pev_enterState(PEV_STATE_WaitForEvUnconnectedEvseOn);
//	else if (cp_read_state_thisCycle == CP_STATE_B2)
//		pev_enterState(PEV_STATE_EvseReady);
//	else {
//		handleCpFault();
//		pev_enterState(PEV_STATE_EvConnnectedFaultEvOrEvse);
//	}
//	handleCpFault();
//}
//
//static void stateFunctionEvseReady(void) {
//
//	printStateOnFirstCycle();
//
//	if (vehicle_control == 1)
//		pev_enterState(PEV_STATE_WaitForEvUnconnectedEvseOn);
//	else if (cp_read_state_thisCycle == CP_STATE_B2)
//		pev_enterState(PEV_STATE_EvseReady);
//	else {
//		handleCpFault();
//		pev_enterState(PEV_STATE_EvConnnectedFaultEvOrEvse);
//	}
//}
//
//static void stateFunctionWaitForEvReady(void) {
//
//	set_cp_state(2);
//
//	if (cp_read_state_thisCycle == CP_STATE_C)
//		pev_enterState(PEV_STATE_EvseReady);
//	else if (cp_read_state_thisCycle != CP_STATE_B2) {
//		handleCpFault();
//		pev_enterState(PEV_STATE_EvConnnectedFaultEvOrEvse);
//	}
//	handleCpFault();
//}
//
//static void stateFunctionEvReady(void) {
//
//	printStateOnFirstCycle();
//
//	pev_enterState(PEV_STATE_WaitForEvseACpowerPresent);
//}
//
//static void stateFunctionWaitForEvseACpowerPresent(void) {
//
//	if (isMoreThan60Vac_onChargerOut == 1)
//		pev_enterState(PEV_STATE_EvseACpowerPresent);
//	handleAcFault();
//}
//
//static void stateFunctionEvseACpowerPresent(void) {
//	printStateOnFirstCycle();
//	pev_enterState(PEV_STATE_EvCharging);
//	startChargeSession();
//}
//
//static void stateFunctionEvCharging(void) {
//	printStateOnFirstCycle();
//	if (vehicle_control == 0 || vehicle_control == 2) {
//		pev_enterState(PEV_STATE_WaitForEvChargeStopped);
//	}
//	else if (cp_read_state_thisCycle == CP_STATE_C && isMoreThan60Vac_onChargerOut == 1) {
//		uint32_t charging_milliAmps = vehicle_amp_limit*1000 < evse_cp_cur_amp_limit_x100*10
//				? vehicle_amp_limit*1000 : evse_cp_cur_amp_limit_x100*10;
//		update_CT_mA_setpoint(charging_milliAmps);
//		return;
//	}
//	printChargeSessionEnded();
//	if (cp_read_state_thisCycle == CP_STATE_C1) {
//		pev_enterState(PEV_STATE_WaitForEvseChargeStopped);
//	}
//	else if (cp_read_state_thisCycle != CP_STATE_C) {
//		update_CT_mA_setpoint(0);
//		handleCpFault();
//		pev_enterState(PEV_STATE_EvConnnectedFaultEvOrEvse);
//	}
//	else if (isMoreThan60Vac_onChargerOut == 1) {
//		update_CT_mA_setpoint(0);
//		handleAcFault();
//		pev_enterState(PEV_STATE_EvConnnectedFaultEvOrEvse);
//	}
//}
//
//static void stateFunctionWaitForEvseChargeStopped(void) {
//
//	update_CT_mA_setpoint(0);
//	set_cp_state(1);
//
//	if (isMoreThan60Vac_onChargerOut == 0 && (cp_read_state_thisCycle == CP_STATE_B1 || cp_read_state_thisCycle == CP_STATE_B2)){
//		pev_enterState(PEV_STATE_EvseChargeStopped);
//	}
//	if (isMoreThan60Vac_onChargerOut != 0)
//		handleAcFault();
//	else
//		handleCpFault();
//}
//
//static void stateFunctionEvseChargeStopped(void) {
//	printStateOnFirstCycle();
//	pev_enterState(PEV_STATE_EvConnectedIdle);
//}
//
//static void stateFunctionWaitForEvChargeStopped(void) {
//
//	update_CT_mA_setpoint(0);
//	set_cp_state(1);
//
//	if (isMoreThan60Vac_onChargerOut == 0 && (cp_read_state_thisCycle == CP_STATE_B1 || cp_read_state_thisCycle == CP_STATE_B2)){
//		pev_enterState(PEV_STATE_EvChargeStopped);
//	}
//	if (isMoreThan60Vac_onChargerOut != 0)
//		handleAcFault();
//	else
//		handleCpFault();
//}
//
//static void stateFunctionEvChargeStopped(void) {
//	printStateOnFirstCycle();
//	pev_enterState(PEV_STATE_EvConnectedIdle);
//}
//
//static void stateFunctionEvConnectedEvseOff(void) {
//	printStateOnFirstCycle();
//
//	update_CT_mA_setpoint(0);
//	set_cp_state(1);
//
//	if (cp_read_state_thisCycle != CP_STATE_E)
//		pev_enterState(PEV_STATE_EvConnectedIdle);
//
//}
//
//static void stateFunctionEvConnnectedFaultEvOrEvse(void) {
//	if (printStateOnFirstCycle()) {
//		addToMsgQ(faultOnStateBuf);
//		addToMsgQ(faultTypeBuf);
//		addToMsgQ(faultTimeOutBuf);
//	}
//	update_CT_mA_setpoint(0);
//	set_cp_state(1);
//}
//
//
//
//
///******* The statemachine dispatcher *******************/
//static void pev_runFsm(void)
//{
//	cp_read_state_thisCycle = evse_cp_debounce_read_state;
//
//
//	// check for state E or error states before running the state
//	if (pev_state != PEV_STATE_EvUnconnectedFaultEvse && pev_state != PEV_STATE_EvConnnectedFaultEvOrEvse) {
//		if (pev_state == PEV_STATE_EvCharging)
//			printChargeSessionEnded();
//		goToEvUnConnOrConnEvseOffIfStateE();
//		goToEvUnConnOrConnFaultIfStateDthruError();
//	}
//
//
//	// check for AC out when idle or unplugged
//	if (pev_state < PEV_STATE_WaitForEvReady && isMoreThan60Vac_onChargerOut == 1)
//	{
//		handleAcFault();
//		if (pev_state == PEV_STATE_EvCharging)
//			printChargeSessionEnded();
//		if (isEvConnected())
//			pev_enterState(PEV_STATE_EvConnnectedFaultEvOrEvse);
//		else
//			pev_enterState(PEV_STATE_EvUnconnectedFaultEvse);
//	}
//
//    if (pev_isTooLong())
//    {
//    	if (isEvConnected())
//			pev_enterState(PEV_STATE_EvConnnectedFaultEvOrEvse);
//		else
//			pev_enterState(PEV_STATE_EvUnconnectedFaultEvse);
//    }
//
//
//    pev_cyclesInState += 1;
//
//// addToMsgQ(pevSttLabels[pev_state]);
//    stateFunctions[pev_state](); //call state function
//
//}
//
//void processEV_stateMachine10ms(void)
//{
//	if (isVehicleControlNotManual)
//	{
//		pev_runFsm();
//	}
//
//}
//
//
//
//
//uint8_t update_vehicle_control(uint8_t new_vehicle_control)
//{
//	if (isVehicleControlNotManual == 0) {
//		set_cp_state(0);
//		pev_state = PEV_STATE_WaitForEvUnconnectedEvseOn;
//	}
//	isVehicleControlNotManual = 1;
//	if (new_vehicle_control > 3)
//		return 0;
//	vehicle_control = new_vehicle_control;
//	return 1;
//}
//
//

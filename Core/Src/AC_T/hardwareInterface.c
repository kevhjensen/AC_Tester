/*
 * Higher Level functions for ADS1220 ADC, MCP4725 DAC, J-1772 GPIOs, and internal ADCs (CP)
 */

#include "AC_T_globals.h"


volatile uint32_t _microsHigh = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        _microsHigh++;
    }
}

uint32_t micros() {
    uint32_t high1, low, high2;
    do {
        high1 = _microsHigh;
        low = __HAL_TIM_GET_COUNTER(&htim2);
        high2 = _microsHigh;
    } while (high1 != high2);  // avoid race condition

    return (high1 << 16) | low;
}

/*
 * ADS1220 higher level stuff
 */
volatile uint8_t ADS1220_new_data_CH = 0;
volatile uint8_t ADS1220_read_both_CHs = 0;

volatile uint16_t V_numSamples = 0;
volatile uint16_t I_numSamples = 0;

void ADS1220_init_AC_V_DRDYM(void)
{
	ADS1220_regs regs = { 0x04, 0xD4, 0x00, 0x02 }; // gain 4, 2KSPS, Mode=Turbo, no 50/60Hz filter, DRDYM enabled (DRDY on MISO)
	// ADS1220_regs regs = { 0x04, 0xD0, 0x00, 0x02 }; //single shot for reading 2 channels, might not be needed
	ADS1220_init(&hspi1, &regs); // Optionally check for failure

	HAL_NVIC_SetPriority(DRDY_IRQ_CH, 0, 0);
	HAL_NVIC_EnableIRQ(DRDY_IRQ_CH);
}

// ADS1220 new data ready interrupt
// 1930/1931 samples/s 1ch, 874s/s 2ch
void EXTI15_10_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(DRDY_PIN) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(DRDY_PIN);
        HAL_NVIC_DisableIRQ(DRDY_IRQ_CH);


        int32_t result = ADS1220_read_after_DRDYM(&hspi1);

		if (ADS1220_new_data_CH == 0)
		{
			if (ADS1220_read_both_CHs)
			{
				ADS1220_switch_CH_V_I(&hspi1, 1); //switch to I ch
				ADS1220_new_data_CH = 1;
			}
			new_ADS_AC_V_sample(result);
		}
		else
		{
			ADS1220_switch_CH_V_I(&hspi1, 0); // always switch back to primary CH (V)
			ADS1220_new_data_CH = 0;

			new_ADS_AC_I_sample(result);
		}

        __HAL_GPIO_EXTI_CLEAR_IT(DRDY_PIN); //Clear EXTI pending flag which catches the spi comms (still monitors when disabled)
        HAL_NVIC_ClearPendingIRQ(DRDY_IRQ_CH);
        HAL_NVIC_EnableIRQ(DRDY_IRQ_CH);
    }
}

/*
 * MCP4725 stuff
 */

static MCP4725 myMCP4725; // static so only in this file's scope

void MCP4725_DP_DN_init(void)
{
	myMCP4725 = MCP4725_init(&hi2c1, MCP4725A0_ADDR_A00, 3.30); // DP DN, A01 is D1


	uint16_t eeprom_val = MCP4725_readRegister(&myMCP4725, MCP4725_READ_EEPROM); // update eeprom to 0 current if not already set

	if (eeprom_val != 2048) { // not midscale or I2C error
		MCP4725_setValue(&myMCP4725, 2048, MCP4725_EEPROM_MODE, MCP4725_POWER_DOWN_OFF); // set DP/DN to 1.65V mid scale (= 0 current)
	}
}

// 90us i2c time, 6us settle time =~100us (2deg lag at 60Hz)
// 750us conversion time from ads1220 + <10us spi read (16.2deg)

void MCP4725_DP_DN_setValue(uint16_t value)
{
	MCP4725_setValue(&myMCP4725, value, MCP4725_FAST_MODE, MCP4725_POWER_DOWN_OFF);
}


/*
 * J-1772 relay/MOSFET stuff
 */

volatile uint8_t cp_cur_set_state = 0; //0:idle,1:plugIN(J-1772 state B), 2:C, 3:extEV, 4:F, 5:GND fault, 6:Diode Short

const GpioPin cp_gpio_map[CP_NUM_GPIO] = {
    { CP_PLUG_IN_PORT, CP_PLUG_IN_PIN },
    { CP_FAULT_PORT,   CP_FAULT_PIN },
    { CP_EXT_EV_PORT,  CP_EXT_EV_PIN },
    { CP_1k3_PORT,     CP_1k3_PIN },
    { CP_270_PORT,     CP_270_PIN }
};
uint8_t cp_gpio_states[CP_NUM_STATES][CP_NUM_GPIO] = {
	{0, 0, 0, 0, 0},
	{1, 0, 0, 0, 0},
	{1, 0, 0, 1, 0},
	{0, 0, 1, 0, 0},
	{1, 0, 0, 0, 1},
	{0, 1, 0, 0, 0},
	{1, 1, 0, 0, 0}
};

uint8_t set_cp_state(uint8_t new_state)
{
	if (new_state >= CP_NUM_STATES)
	{
		return 0;
	}
	cp_cur_set_state = new_state;

	for (int i=0; i < 5; i++) {
		HAL_GPIO_WritePin(cp_gpio_map[i].port, cp_gpio_map[i].pin, cp_gpio_states[new_state][i]);
	}
	return 1;
}


void hardwareInterfaceInit(void)
{
	ADS1220_init_AC_V_DRDYM();
	MCP4725_DP_DN_init();
	HAL_TIM_Base_Start_IT(&htim2);
}








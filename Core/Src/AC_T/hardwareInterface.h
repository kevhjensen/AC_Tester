/*
 * Higher Level functions for ADS1220 ADC, MCP4725 DAC, and J-1772 GPIOs
 */

#ifndef SRC_AC_T_HARDWAREINTERFACE_H_
#define SRC_AC_T_HARDWAREINTERFACE_H_


/*
 * microsecond time
 */
extern TIM_HandleTypeDef htim2; //1 Mhz counter, 65.6ms overflow

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
uint32_t micros();


/*
 * ADS1220 stuff
 */

extern SPI_HandleTypeDef hspi1;

#define DRDY_GPIO_PORT GPIOB // GPIOB, MISO line also connected to another GPIO pin set as interrupt
#define DRDY_PIN       GPIO_PIN_10 // GPIO_PIN_10
#define DRDY_IRQ_CH    EXTI15_10_IRQn // pin 10 on this

extern volatile uint8_t ADS1220_read_both_CHs; // 0=read only AC V, 1=read V & I (ADC in)
extern volatile uint8_t ADS1220_new_data_CH; // 0=last data AC V, 1=last data AC I

extern volatile uint16_t V_numSamples;
extern volatile uint16_t I_numSamples;

void ADS1220_init_AC_V_DRDYM(void);
void EXTI15_10_IRQHandler(void);


/*
 * MCP4725 stuff
 */

extern I2C_HandleTypeDef hi2c1;

void MCP4725_DP_DN_init(void);
void MCP4725_DP_DN_setValue(uint16_t value);


/*
 * J-1772 Relay and MOSFET Controls
 */
#define CP_NUM_GPIO 5
#define CP_NUM_STATES 7

#define CP_PLUG_IN_PORT    GPIOC
#define CP_FAULT_PORT      GPIOC
#define CP_EXT_EV_PORT     GPIOB
#define CP_1k3_PORT        GPIOA
#define CP_270_PORT        GPIOA

#define CP_PLUG_IN_PIN     GPIO_PIN_4
#define CP_FAULT_PIN       GPIO_PIN_5
#define CP_EXT_EV_PIN      GPIO_PIN_0
#define CP_1k3_PIN         GPIO_PIN_1
#define CP_270_PIN         GPIO_PIN_2

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} GpioPin;

extern const GpioPin cp_gpio_map[CP_NUM_GPIO];


extern volatile uint8_t cp_cur_set_state;
extern volatile uint8_t cp_new_set_state;



typedef enum {
    CP_IDLE = 0,
    CP_PLUG_IN = 1,
    CP_C = 2,
    CP_EXT_EV = 3,
    CP_F = 4,
    CP_GND_FAULT = 5,
    CP_DIODE_SHORT = 6
} CP_Set_State;

uint8_t set_cp_state(uint8_t new_state);

void hardwareInterfaceInit(void);





#endif /* SRC_AC_T_HARDWAREINTERFACE_H_ */

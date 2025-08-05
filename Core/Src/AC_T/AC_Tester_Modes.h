



#ifndef SRC_AC_T_AC_TESTER_MODES_H_
#define SRC_AC_T_AC_TESTER_MODES_H_






extern volatile uint16_t STDdataPeriodMS;

extern volatile uint8_t data_mode;
extern volatile uint8_t printSamples;


int ParseUSBCommand(char *line);



#endif /* SRC_AC_T_AC_TESTER_MODES_H_ */

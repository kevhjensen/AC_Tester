################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/AC_T/AC_Tester_Modes.c \
../Core/Src/AC_T/AC_processor.c \
../Core/Src/AC_T/ADS1220.c \
../Core/Src/AC_T/CpRead.c \
../Core/Src/AC_T/EV_StateMachine.c \
../Core/Src/AC_T/MCP4725.c \
../Core/Src/AC_T/hardwareInterface.c \
../Core/Src/AC_T/myHelpers.c \
../Core/Src/AC_T/myScheduler.c 

OBJS += \
./Core/Src/AC_T/AC_Tester_Modes.o \
./Core/Src/AC_T/AC_processor.o \
./Core/Src/AC_T/ADS1220.o \
./Core/Src/AC_T/CpRead.o \
./Core/Src/AC_T/EV_StateMachine.o \
./Core/Src/AC_T/MCP4725.o \
./Core/Src/AC_T/hardwareInterface.o \
./Core/Src/AC_T/myHelpers.o \
./Core/Src/AC_T/myScheduler.o 

C_DEPS += \
./Core/Src/AC_T/AC_Tester_Modes.d \
./Core/Src/AC_T/AC_processor.d \
./Core/Src/AC_T/ADS1220.d \
./Core/Src/AC_T/CpRead.d \
./Core/Src/AC_T/EV_StateMachine.d \
./Core/Src/AC_T/MCP4725.d \
./Core/Src/AC_T/hardwareInterface.d \
./Core/Src/AC_T/myHelpers.d \
./Core/Src/AC_T/myScheduler.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/AC_T/%.o Core/Src/AC_T/%.su Core/Src/AC_T/%.cyclo: ../Core/Src/AC_T/%.c Core/Src/AC_T/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/kevinj/STM32CubeIDE/workspace_1.16.1/AC_Tester/Core/Src/AC_T" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-AC_T

clean-Core-2f-Src-2f-AC_T:
	-$(RM) ./Core/Src/AC_T/AC_Tester_Modes.cyclo ./Core/Src/AC_T/AC_Tester_Modes.d ./Core/Src/AC_T/AC_Tester_Modes.o ./Core/Src/AC_T/AC_Tester_Modes.su ./Core/Src/AC_T/AC_processor.cyclo ./Core/Src/AC_T/AC_processor.d ./Core/Src/AC_T/AC_processor.o ./Core/Src/AC_T/AC_processor.su ./Core/Src/AC_T/ADS1220.cyclo ./Core/Src/AC_T/ADS1220.d ./Core/Src/AC_T/ADS1220.o ./Core/Src/AC_T/ADS1220.su ./Core/Src/AC_T/CpRead.cyclo ./Core/Src/AC_T/CpRead.d ./Core/Src/AC_T/CpRead.o ./Core/Src/AC_T/CpRead.su ./Core/Src/AC_T/EV_StateMachine.cyclo ./Core/Src/AC_T/EV_StateMachine.d ./Core/Src/AC_T/EV_StateMachine.o ./Core/Src/AC_T/EV_StateMachine.su ./Core/Src/AC_T/MCP4725.cyclo ./Core/Src/AC_T/MCP4725.d ./Core/Src/AC_T/MCP4725.o ./Core/Src/AC_T/MCP4725.su ./Core/Src/AC_T/hardwareInterface.cyclo ./Core/Src/AC_T/hardwareInterface.d ./Core/Src/AC_T/hardwareInterface.o ./Core/Src/AC_T/hardwareInterface.su ./Core/Src/AC_T/myHelpers.cyclo ./Core/Src/AC_T/myHelpers.d ./Core/Src/AC_T/myHelpers.o ./Core/Src/AC_T/myHelpers.su ./Core/Src/AC_T/myScheduler.cyclo ./Core/Src/AC_T/myScheduler.d ./Core/Src/AC_T/myScheduler.o ./Core/Src/AC_T/myScheduler.su

.PHONY: clean-Core-2f-Src-2f-AC_T


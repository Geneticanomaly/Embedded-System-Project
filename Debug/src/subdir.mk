################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/btn_handler.c \
../src/main.c \
../src/modulate_handler.c \
../src/timer_setup.c \
../src/uart_setup.c 

OBJS += \
./src/btn_handler.o \
./src/main.o \
./src/modulate_handler.o \
./src/timer_setup.o \
./src/uart_setup.o 

C_DEPS += \
./src/btn_handler.d \
./src/main.d \
./src/modulate_handler.d \
./src/timer_setup.d \
./src/uart_setup.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../ProjectRTOS_bsp/ps7_cortexa9_0/include -I"C:\Users\thoma\Xilink\zybo_hw" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



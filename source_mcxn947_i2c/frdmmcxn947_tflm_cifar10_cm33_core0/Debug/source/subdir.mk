################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../source/demo_info.cpp \
../source/eval_metrics.cpp \
../source/get_top_n.cpp \
../source/main.cpp \
../source/model.cpp \
../source/model_cifarnet_ops_npu.cpp \
../source/output_postproc.cpp 

C_SRCS += \
../source/image_decode_raw.c \
../source/image_load.c \
../source/semihost_hardfault.c \
../source/timer.c 

CPP_DEPS += \
./source/demo_info.d \
./source/eval_metrics.d \
./source/get_top_n.d \
./source/main.d \
./source/model.d \
./source/model_cifarnet_ops_npu.d \
./source/output_postproc.d 

C_DEPS += \
./source/image_decode_raw.d \
./source/image_load.d \
./source/semihost_hardfault.d \
./source/timer.d 

OBJS += \
./source/demo_info.o \
./source/eval_metrics.o \
./source/get_top_n.o \
./source/image_decode_raw.o \
./source/image_load.o \
./source/main.o \
./source/model.o \
./source/model_cifarnet_ops_npu.o \
./source/output_postproc.o \
./source/semihost_hardfault.o \
./source/timer.o 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.cpp source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C++ Compiler'
	arm-none-eabi-c++ -std=gnu++11 -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCPU_MCXN947VDF_cm33_core0 -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE_UART -DARM_MATH_CM33 -D__FPU_PRESENT=1 -DSDK_DEBUGCONSOLE=1 -DMCUX_META_BUILD -DMCXN947_cm33_core0_SERIES -DTF_LITE_STATIC_MEMORY -DKERNELS_OPTIMIZED_FOR_SPEED -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -DSDK_OS_BAREMETAL -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\drivers" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\CMSIS" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\CMSIS\m-profile" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\device" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\device\periph" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\component\lists" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities\str" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities\debug_console_lite" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\component\uart" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\flatbuffers\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\gemmlowp" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\kissfft" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\ruy" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\cmsis_nn" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\cmsis_nn\Include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\tensorflow\lite\micro\kernels\neutron" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\neutron\common\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\neutron\driver\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\source" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\board" -O0 -fno-common -g3 -gdwarf-4 -Wall -mcpu=cortex-m33 -c -ffunction-sections -fdata-sections -fno-builtin -fno-rtti -fno-exceptions -imacros "D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\source\mcux_config.h" -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/%.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__NEWLIB__ -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCPU_MCXN947VDF_cm33_core0 -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE_UART -DARM_MATH_CM33 -D__FPU_PRESENT=1 -DSDK_DEBUGCONSOLE=1 -DMCUX_META_BUILD -DMCXN947_cm33_core0_SERIES -DTF_LITE_STATIC_MEMORY -DKERNELS_OPTIMIZED_FOR_SPEED -DCR_INTEGER_PRINTF -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -DSDK_OS_BAREMETAL -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\drivers" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\CMSIS" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\CMSIS\m-profile" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\device" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\device\periph" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\component\lists" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities\str" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities\debug_console_lite" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\component\uart" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\flatbuffers\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\gemmlowp" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\kissfft" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\ruy" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\cmsis_nn" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\cmsis_nn\Include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\tensorflow\lite\micro\kernels\neutron" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\neutron\common\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\neutron\driver\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\source" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\board" -O0 -fno-common -g3 -gdwarf-4 -Wall -mcpu=cortex-m33 -c -ffunction-sections -fdata-sections -fno-builtin -imacros "D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\ESP32_I2C_MCXN947\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\source\mcux_config.h" -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source

clean-source:
	-$(RM) ./source/demo_info.d ./source/demo_info.o ./source/eval_metrics.d ./source/eval_metrics.o ./source/get_top_n.d ./source/get_top_n.o ./source/image_decode_raw.d ./source/image_decode_raw.o ./source/image_load.d ./source/image_load.o ./source/main.d ./source/main.o ./source/model.d ./source/model.o ./source/model_cifarnet_ops_npu.d ./source/model_cifarnet_ops_npu.o ./source/output_postproc.d ./source/output_postproc.o ./source/semihost_hardfault.d ./source/semihost_hardfault.o ./source/timer.d ./source/timer.o

.PHONY: clean-source


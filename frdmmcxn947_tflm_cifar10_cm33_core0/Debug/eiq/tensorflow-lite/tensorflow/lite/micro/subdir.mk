################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../eiq/tensorflow-lite/tensorflow/lite/micro/debug_log.cpp 

CPP_DEPS += \
./eiq/tensorflow-lite/tensorflow/lite/micro/debug_log.d 

OBJS += \
./eiq/tensorflow-lite/tensorflow/lite/micro/debug_log.o 


# Each subdirectory must supply rules for building sources it contributes
eiq/tensorflow-lite/tensorflow/lite/micro/%.o: ../eiq/tensorflow-lite/tensorflow/lite/micro/%.cpp eiq/tensorflow-lite/tensorflow/lite/micro/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C++ Compiler'
	arm-none-eabi-c++ -std=gnu++11 -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCPU_MCXN947VDF_cm33_core0 -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE_UART -DARM_MATH_CM33 -D__FPU_PRESENT=1 -DSDK_DEBUGCONSOLE=1 -DMCUX_META_BUILD -DMCXN947_cm33_core0_SERIES -DTF_LITE_STATIC_MEMORY -DKERNELS_OPTIMIZED_FOR_SPEED -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\source" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\drivers" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\CMSIS" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\CMSIS\m-profile" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\device" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\device\periph" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\component\lists" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities\str" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities\debug_console_lite" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\component\uart" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\flatbuffers\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\gemmlowp" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\kissfft" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\ruy" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\cmsis_nn" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\cmsis_nn\Include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\tensorflow\lite\micro\kernels\neutron" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\neutron\common\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\neutron\driver\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\board" -O0 -fno-common -g3 -gdwarf-4 -Wall -mcpu=cortex-m33 -c -ffunction-sections -fdata-sections -fno-builtin -fno-rtti -fno-exceptions -imacros "D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\source\mcux_config.h" -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-eiq-2f-tensorflow-2d-lite-2f-tensorflow-2f-lite-2f-micro

clean-eiq-2f-tensorflow-2d-lite-2f-tensorflow-2f-lite-2f-micro:
	-$(RM) ./eiq/tensorflow-lite/tensorflow/lite/micro/debug_log.d ./eiq/tensorflow-lite/tensorflow/lite/micro/debug_log.o

.PHONY: clean-eiq-2f-tensorflow-2d-lite-2f-tensorflow-2f-lite-2f-micro


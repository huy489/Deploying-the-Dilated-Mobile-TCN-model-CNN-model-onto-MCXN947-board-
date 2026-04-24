################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../utilities/fsl_assert.c 

S_UPPER_SRCS += \
../utilities/fsl_memcpy.S 

C_DEPS += \
./utilities/fsl_assert.d 

OBJS += \
./utilities/fsl_assert.o \
./utilities/fsl_memcpy.o 


# Each subdirectory must supply rules for building sources it contributes
utilities/%.o: ../utilities/%.c utilities/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__NEWLIB__ -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCPU_MCXN947VDF_cm33_core0 -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE_UART -DARM_MATH_CM33 -D__FPU_PRESENT=1 -DSDK_DEBUGCONSOLE=1 -DMCUX_META_BUILD -DMCXN947_cm33_core0_SERIES -DTF_LITE_STATIC_MEMORY -DKERNELS_OPTIMIZED_FOR_SPEED -DCR_INTEGER_PRINTF -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\source" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\drivers" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\CMSIS" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\CMSIS\m-profile" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\device" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\device\periph" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\component\lists" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities\str" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities\debug_console_lite" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\component\uart" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\flatbuffers\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\gemmlowp" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\kissfft" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\ruy" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\cmsis_nn" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\cmsis_nn\Include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\tensorflow\lite\micro\kernels\neutron" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\neutron\common\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\neutron\driver\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\board" -O0 -fno-common -g3 -gdwarf-4 -Wall -mcpu=cortex-m33 -c -ffunction-sections -fdata-sections -fno-builtin -imacros "D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\source\mcux_config.h" -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

utilities/%.o: ../utilities/%.S utilities/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU Assembler'
	arm-none-eabi-gcc -c -x assembler-with-cpp -D__NEWLIB__ -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\source" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\drivers" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\CMSIS" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\CMSIS\m-profile" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\device" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\device\periph" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\component\lists" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities\str" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\utilities\debug_console_lite" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\component\uart" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\flatbuffers\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\gemmlowp" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\kissfft" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\ruy" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\cmsis_nn" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\cmsis_nn\Include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\tensorflow\lite\micro\kernels\neutron" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\neutron\common\include" -I"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\IDE_mcxn947\frdmmcxn947_tflm_cifar10_cm33_core0\eiq\tensorflow-lite\third_party\neutron\driver\include" -g3 -gdwarf-4 -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -specs=nano.specs -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-utilities

clean-utilities:
	-$(RM) ./utilities/fsl_assert.d ./utilities/fsl_assert.o ./utilities/fsl_memcpy.o

.PHONY: clean-utilities


/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "output_postproc.h"
#include "demo_config.h"
#include "labels.h"
#ifdef EIQ_GUI_PRINTF
#include "chgui.h"
#endif

float MODEL_GetSigmoidScore(const uint8_t* data, tensor_type_t type)
{
    float value = 0.0f;

    switch (type)
    {
        case kTensorType_FLOAT32:
        {
            const float* predictions = reinterpret_cast<const float*>(data);
            value = predictions[0];
            break;
        }

        case kTensorType_UINT8:
        {
            const uint8_t* predictions = data;
            value = predictions[0] / 255.0f;
            break;
        }

        case kTensorType_INT8:
        {
            const int8_t* predictions = reinterpret_cast<const int8_t*>(data);
            value = ((int)predictions[0] + 128) / 255.0f;
            break;
        }
    }

    return value;
}

status_t MODEL_ProcessOutput(const uint8_t* data, const tensor_dims_t* dims,
                             tensor_type_t type, int inferenceTime)
{
    const float threshold = (float)DETECTION_TRESHOLD / 100.0f;

    int outputCount = dims->data[dims->size - 1];
    if (outputCount != 1)
    {
        PRINTF("Error: sigmoid model expected 1 output, got %d\r\n", outputCount);
        return kStatus_Fail;
    }

    float confidence = MODEL_GetSigmoidScore(data, type);
    const char* label = (confidence >= threshold) ? labels[1] : labels[0];
    int score = (int)(confidence * 100.0f);

    if (type == kTensorType_INT8)
    {
        const int8_t* out = reinterpret_cast<const int8_t*>(data);
        PRINTF("     Raw output int8: %d\r\n", out[0]);
    }

    PRINTF("----------------------------------------\r\n");
    PRINTF("     Inference time: %d us\r\n", inferenceTime);
    PRINTF("     Sigmoid score : %d%%\r\n", score);
    PRINTF("     Detected      : %s\r\n", label);
    PRINTF("----------------------------------------\r\n");

#ifdef EIQ_GUI_PRINTF
    GUI_PrintfToBuffer(GUI_X_POS, GUI_Y_POS, "Detected: %.20s (%d%%)", label, score);
#endif

    return kStatus_Success;
}

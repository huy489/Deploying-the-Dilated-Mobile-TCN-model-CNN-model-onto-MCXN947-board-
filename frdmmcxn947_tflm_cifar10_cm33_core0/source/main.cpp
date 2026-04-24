/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board_init.h"
#include "demo_config.h"
#include "demo_info.h"
#include "fsl_debug_console.h"
#include "model.h"
#include "output_postproc.h"
#include "timer.h"

#include <stdint.h>
#include <string.h>

#define MAGIC_WORD    (0x45434731u)   /* "ECG1" */
#define CMD_DATA      (1u)
#define CMD_END       (2u)
#define ECG_INPUT_LEN (2500)

static void UART_ReadExact(uint8_t* dst, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        dst[i] = (uint8_t)DbgConsole_Getchar();
    }
}

static uint32_t checksum_sum_u8(const uint8_t* data, uint32_t len)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return sum;
}

static uint32_t read_u32_le(void)
{
    uint8_t b[4];
    UART_ReadExact(b, 4);
    return ((uint32_t)b[0]) |
           ((uint32_t)b[1] << 8) |
           ((uint32_t)b[2] << 16) |
           ((uint32_t)b[3] << 24);
}

static uint16_t read_u16_le(void)
{
    uint8_t b[2];
    UART_ReadExact(b, 2);
    return (uint16_t)(b[0] | (b[1] << 8));
}

int main(void)
{
    BOARD_Init();
    TIMER_Init();

    DEMO_PrintInfo();

    if (MODEL_Init() != kStatus_Success)
    {
        PRINTF("Failed initializing model\r\n");
        for (;;) {}
    }

    tensor_dims_t inputDims;
    tensor_type_t inputType;
    uint8_t* inputData = MODEL_GetInputTensorData(&inputDims, &inputType);

    tensor_dims_t outputDims;
    tensor_type_t outputType;
    uint8_t* outputData = MODEL_GetOutputTensorData(&outputDims, &outputType);

    if (inputType != kTensorType_INT8)
    {
        PRINTF("ERROR,Unexpected input type. Need INT8.\r\n");
        for (;;) {}
    }

    int8_t* modelInput = reinterpret_cast<int8_t*>(inputData);

    PRINTF("READY\r\n");

    while (1)
    {
        uint32_t magic = read_u32_le();
        if (magic != MAGIC_WORD)
        {
            PRINTF("ERROR,Bad magic: 0x%08X\r\n", magic);
            continue;
        }

        uint8_t cmd = (uint8_t)DbgConsole_Getchar();

        if (cmd == CMD_DATA)
        {
            uint32_t sample_idx = read_u32_le();
            uint8_t true_label = (uint8_t)DbgConsole_Getchar();
            uint16_t payload_len = read_u16_le();

            if (payload_len != ECG_INPUT_LEN)
            {
                PRINTF("ERROR,Bad payload_len:%d\r\n", payload_len);

                /* bỏ qua phần còn lại của packet lỗi */
                for (uint32_t i = 0; i < payload_len + 4; i++)
                {
                    (void)DbgConsole_Getchar();
                }
                continue;
            }

            uint8_t rawBuf[ECG_INPUT_LEN];
            UART_ReadExact(rawBuf, payload_len);

            uint32_t recv_checksum = read_u32_le();
            uint32_t calc_checksum = checksum_sum_u8(rawBuf, payload_len);

            if (recv_checksum != calc_checksum)
            {
                PRINTF("ERROR,Checksum mismatch,%lu\r\n", (unsigned long)sample_idx);
                continue;
            }

            for (int i = 0; i < ECG_INPUT_LEN; i++)
            {
                modelInput[i] = (int8_t)rawBuf[i];
            }

            auto startTime = TIMER_GetTimeInUS();
            MODEL_RunInference();
            auto endTime = TIMER_GetTimeInUS();

            float score = MODEL_GetSigmoidScore(outputData, outputType);
            float threshold = (float)DETECTION_TRESHOLD / 100.0f;
            uint8_t pred_label = (score >= threshold) ? 1 : 0;

            int score_micro = (int)(score * 1000000.0f);

            PRINTF("RESULT,%d,%d,%d,%d,%d\r\n",
                   (int)sample_idx,
                   (int)true_label,
                   (int)pred_label,
                   score_micro,
                   (int)(endTime - startTime));

        }
        else if (cmd == CMD_END)
        {
            uint32_t total_sent = read_u32_le();
            PRINTF("END_ACK,%lu\r\n", (unsigned long)total_sent);
            for (;;) {}
        }
        else
        {
            PRINTF("ERROR,Unknown CMD:%u\r\n", (unsigned int)cmd);
        }
    }
}

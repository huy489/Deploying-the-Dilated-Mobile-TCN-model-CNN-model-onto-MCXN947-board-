#ifndef EVAL_METRICS_H
#define EVAL_METRICS_H

#include <stdint.h>
#include "fsl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_EVAL_SAMPLES
#define MAX_EVAL_SAMPLES 8000
#endif

typedef struct
{
    uint8_t trueLabel;
    uint8_t predLabel;
    float score;
} eval_record_t;

typedef struct
{
    eval_record_t records[MAX_EVAL_SAMPLES];
    int count;
} eval_buffer_t;

void EVAL_Reset(eval_buffer_t* evalBuf);

status_t EVAL_AddSample(eval_buffer_t* evalBuf,
                        uint8_t trueLabel,
                        float score,
                        float threshold);

void EVAL_PrintFinalReport(const eval_buffer_t* evalBuf);

#ifdef __cplusplus
}
#endif

#endif
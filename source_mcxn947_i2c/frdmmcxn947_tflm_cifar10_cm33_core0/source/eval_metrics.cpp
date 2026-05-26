#include "eval_metrics.h"
#include "fsl_debug_console.h"
#include <stdlib.h>

typedef struct
{
    float score;
    uint8_t label;
} auc_item_t;

static int auc_compare_desc(const void* a, const void* b)
{
    const auc_item_t* x = (const auc_item_t*)a;
    const auc_item_t* y = (const auc_item_t*)b;

    if (x->score < y->score) return 1;
    if (x->score > y->score) return -1;
    return 0;
}

void EVAL_Reset(eval_buffer_t* evalBuf)
{
    evalBuf->count = 0;
}

status_t EVAL_AddSample(eval_buffer_t* evalBuf,
                        uint8_t trueLabel,
                        float score,
                        float threshold)
{
    if (evalBuf->count >= MAX_EVAL_SAMPLES)
    {
        PRINTF("EVAL buffer full!\r\n");
        return kStatus_Fail;
    }

    eval_record_t* rec = &evalBuf->records[evalBuf->count];
    rec->trueLabel = trueLabel ? 1 : 0;
    rec->score = score;
    rec->predLabel = (score >= threshold) ? 1 : 0;

    evalBuf->count++;
    return kStatus_Success;
}

static float safe_div(float num, float den)
{
    if (den == 0.0f) return 0.0f;
    return num / den;
}

static float compute_auc(const eval_buffer_t* evalBuf)
{
    int n = evalBuf->count;
    if (n <= 1) return 0.0f;

    int posCount = 0;
    int negCount = 0;

    static auc_item_t items[MAX_EVAL_SAMPLES];

    for (int i = 0; i < n; i++)
    {
        items[i].score = evalBuf->records[i].score;
        items[i].label = evalBuf->records[i].trueLabel;

        if (items[i].label == 1) posCount++;
        else negCount++;
    }

    if (posCount == 0 || negCount == 0)
    {
        return 0.0f;
    }

    qsort(items, n, sizeof(auc_item_t), auc_compare_desc);

    float tp = 0.0f;
    float fp = 0.0f;
    float prev_tpr = 0.0f;
    float prev_fpr = 0.0f;
    float auc = 0.0f;

    for (int i = 0; i < n; i++)
    {
        if (items[i].label == 1) tp += 1.0f;
        else fp += 1.0f;

        float tpr = tp / (float)posCount;
        float fpr = fp / (float)negCount;

        auc += (fpr - prev_fpr) * (tpr + prev_tpr) * 0.5f;

        prev_tpr = tpr;
        prev_fpr = fpr;
    }

    return auc;
}

void EVAL_PrintFinalReport(const eval_buffer_t* evalBuf)
{
    int n = evalBuf->count;
    if (n <= 0)
    {
        PRINTF("No evaluation samples available.\r\n");
        return;
    }

    int TP = 0, TN = 0, FP = 0, FN = 0;
    int support_pos = 0;
    int support_neg = 0;

    for (int i = 0; i < n; i++)
    {
        uint8_t y = evalBuf->records[i].trueLabel;
        uint8_t p = evalBuf->records[i].predLabel;

        if (y == 1) support_pos++;
        else support_neg++;

        if (y == 1 && p == 1) TP++;
        else if (y == 0 && p == 0) TN++;
        else if (y == 0 && p == 1) FP++;
        else if (y == 1 && p == 0) FN++;
    }

    float precision_pos = safe_div((float)TP, (float)(TP + FP));
    float recall_pos    = safe_div((float)TP, (float)(TP + FN));
    float f1_pos        = safe_div(2.0f * precision_pos * recall_pos, precision_pos + recall_pos);

    float precision_neg = safe_div((float)TN, (float)(TN + FN));
    float recall_neg    = safe_div((float)TN, (float)(TN + FP));
    float f1_neg        = safe_div(2.0f * precision_neg * recall_neg, precision_neg + recall_neg);

    float accuracy      = safe_div((float)(TP + TN), (float)n);
    float sensitivity   = recall_pos;
    float specificity   = safe_div((float)TN, (float)(TN + FP));
    float auc           = compute_auc(evalBuf);

    PRINTF("\r\n========================================\r\n");
    PRINTF("FINAL EVALUATION REPORT\r\n");
    PRINTF("========================================\r\n");

    PRINTF("Confusion Matrix:\r\n");
    PRINTF("  TN = %d, FP = %d\r\n", TN, FP);
    PRINTF("  FN = %d, TP = %d\r\n", FN, TP);

    PRINTF("\r\nNormal (0):\r\n");
    PRINTF("  precision = %.4f\r\n", precision_neg);
    PRINTF("  recall    = %.4f\r\n", recall_neg);
    PRINTF("  f1-score  = %.4f\r\n", f1_neg);
    PRINTF("  support   = %d\r\n", support_neg);

    PRINTF("AFib (1):\r\n");
    PRINTF("  precision = %.4f\r\n", precision_pos);
    PRINTF("  recall    = %.4f\r\n", recall_pos);
    PRINTF("  f1-score  = %.4f\r\n", f1_pos);
    PRINTF("  support   = %d\r\n", support_pos);

    PRINTF("\r\nOverall:\r\n");
    PRINTF("  accuracy    = %.4f\r\n", accuracy);
    PRINTF("  sensitivity = %.4f\r\n", sensitivity);
    PRINTF("  specificity = %.4f\r\n", specificity);
    PRINTF("  AUC Score   = %.4f\r\n", auc);
    PRINTF("========================================\r\n");
}
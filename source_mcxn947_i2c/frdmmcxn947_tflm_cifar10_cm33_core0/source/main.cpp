/*
 * MCXN947 VERSION 3:
 * Receive full ECG sample from ESP32 via I2C.
 * Support grouped ACK by using RX packet queue.
 * Run inference and return result packet to ESP32.
 */

#include "board_init.h"
#include "demo_config.h"
#include "demo_info.h"
#include "fsl_debug_console.h"
#include "model.h"
#include "output_postproc.h"
#include "timer.h"

#include "fsl_clock.h"
#include "fsl_lpi2c.h"
#include "fsl_lpflexcomm.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*******************************************************************************
 * I2C config
 ******************************************************************************/

#define I2C_SLAVE_BASE      LPI2C2
#define I2C_FLEXCOMM_INDEX  2U
#define I2C_SLAVE_ADDR      0x55

/*******************************************************************************
 * Protocol
 ******************************************************************************/

#define CMD_PING            0x01
#define CMD_START_SAMPLE    0x10
#define CMD_WRITE_CHUNK     0x11
#define CMD_COMMIT_SAMPLE   0x12
#define CMD_RESET_SESSION   0x7F

#define ECG_INPUT_LEN       2500

/*
 * Must match ESP32 CHUNK_SIZE.
 * Packet size = 1 + 2 + 1 + CHUNK_SIZE + 2
 * For CHUNK_SIZE = 112, packet size = 118 bytes.
 */
#define CHUNK_SIZE          112

/*
 * Must be larger than max packet size.
 */
#define RX_BUF_SIZE         160

/*
 * Must be larger than ACK_EVERY_N_CHUNKS on ESP32.
 * If ESP32 sends 4 chunks then reads ACK, queue depth 8 is safe.
 */
#define RX_QUEUE_DEPTH      8

#define RSP_OK              0x00
#define RSP_ERR             0x01
#define RSP_BUSY            0x02
#define RSP_RESULT_READY    0x03

#define RSP_MAGIC           0x3354  /* 'T3' little-endian */

/*******************************************************************************
 * Response packet
 ******************************************************************************/

typedef struct __attribute__((packed))
{
    uint16_t magic;
    uint8_t  status;
    uint8_t  ack_cmd;
    uint16_t offset;
    uint16_t received_len;
    uint16_t chunk_count;
    uint16_t extra;

    uint16_t record_id;
    uint32_t sample_idx;
    uint8_t  true_label;
    uint8_t  pred_label;
    uint16_t score_q15;
    uint32_t inference_us;

    uint16_t checksum;
} t3_response_t;

/*******************************************************************************
 * RX queue packet
 ******************************************************************************/

typedef struct
{
    uint8_t data[RX_BUF_SIZE];
    uint32_t len;
} rx_packet_t;

/*******************************************************************************
 * Global state
 ******************************************************************************/

static lpi2c_slave_handle_t g_lpi2c_slave_handle;

static volatile bool g_rx_active = false;
static uint8_t g_rx_buf[RX_BUF_SIZE];

static rx_packet_t g_rx_queue[RX_QUEUE_DEPTH];
static volatile uint8_t g_rx_head = 0;
static volatile uint8_t g_rx_tail = 0;
static volatile uint8_t g_rx_count = 0;

static t3_response_t g_response;

static int8_t g_input_buffer[ECG_INPUT_LEN];

static uint16_t g_received_len = 0;
static uint16_t g_expected_len = 0;
static uint16_t g_expected_checksum = 0;

static uint16_t g_record_id = 0;
static uint32_t g_sample_idx = 0;
static uint8_t g_true_label = 0;

static uint16_t g_chunk_count = 0;
static bool g_session_started = false;

static volatile bool g_infer_request = false;

static int8_t *g_model_input = NULL;
static uint8_t *g_output_data = NULL;
static tensor_type_t g_output_type;

/*******************************************************************************
 * Utility
 ******************************************************************************/

static uint16_t checksum16(const uint8_t *data, uint32_t len)
{
    uint32_t sum = 0;

    for (uint32_t i = 0; i < len; i++)
    {
        sum += data[i];
    }

    return (uint16_t)(sum & 0xFFFF);
}

static uint16_t read_u16_le(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_u32_le(const uint8_t *p)
{
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void write_response(uint8_t status,
                           uint8_t ack_cmd,
                           uint16_t offset,
                           uint16_t received_len,
                           uint16_t chunk_count,
                           uint16_t extra)
{
    memset(&g_response, 0, sizeof(g_response));

    g_response.magic = RSP_MAGIC;
    g_response.status = status;
    g_response.ack_cmd = ack_cmd;
    g_response.offset = offset;
    g_response.received_len = received_len;
    g_response.chunk_count = chunk_count;
    g_response.extra = extra;

    g_response.record_id = g_record_id;
    g_response.sample_idx = g_sample_idx;
    g_response.true_label = g_true_label;

    g_response.checksum = 0;
    g_response.checksum = checksum16((const uint8_t *)&g_response, sizeof(g_response));
}

static void reset_session(void)
{
    g_received_len = 0;
    g_expected_len = 0;
    g_expected_checksum = 0;

    g_record_id = 0;
    g_sample_idx = 0;
    g_true_label = 0;

    g_chunk_count = 0;
    g_session_started = false;
    g_infer_request = false;

    g_rx_head = 0;
    g_rx_tail = 0;
    g_rx_count = 0;

    memset(g_input_buffer, 0, sizeof(g_input_buffer));

    write_response(RSP_OK, CMD_RESET_SESSION, 0, 0, 0, 0);

    PRINTF("T3V3: session reset\r\n");
}

/*******************************************************************************
 * Packet handlers
 ******************************************************************************/

static void handle_start_sample(const uint8_t *data, uint32_t len)
{
    if (len != 12)
    {
        PRINTF("T3V3 ERROR: START len=%u expected=12\r\n", (unsigned int)len);
        write_response(RSP_ERR, CMD_START_SAMPLE, 0, 0, 0, 1);
        return;
    }

    g_record_id = read_u16_le(&data[1]);
    g_sample_idx = read_u32_le(&data[3]);
    g_true_label = data[7];
    g_expected_len = read_u16_le(&data[8]);
    g_expected_checksum = read_u16_le(&data[10]);

    if (g_expected_len != ECG_INPUT_LEN)
    {
        PRINTF("T3V3 ERROR: bad expected_len=%u\r\n", g_expected_len);
        write_response(RSP_ERR, CMD_START_SAMPLE, 0, 0, 0, 2);
        return;
    }

    g_received_len = 0;
    g_chunk_count = 0;
    g_session_started = true;
    g_infer_request = false;

    memset(g_input_buffer, 0, sizeof(g_input_buffer));

    write_response(RSP_OK, CMD_START_SAMPLE, 0, g_received_len, g_chunk_count, g_expected_checksum);

    PRINTF("T3V3 START OK: record=%05u sample=%u true_label=%u len=%u expected_checksum=%u\r\n",
           g_record_id,
           (unsigned int)g_sample_idx,
           g_true_label,
           g_expected_len,
           g_expected_checksum);
}

static void handle_write_chunk(const uint8_t *data, uint32_t len)
{
    if (!g_session_started)
    {
        PRINTF("T3V3 ERROR: chunk before START\r\n");
        write_response(RSP_ERR, CMD_WRITE_CHUNK, 0, g_received_len, g_chunk_count, 3);
        return;
    }

    if (len < 6)
    {
        PRINTF("T3V3 ERROR: short chunk len=%u\r\n", (unsigned int)len);
        write_response(RSP_ERR, CMD_WRITE_CHUNK, 0, g_received_len, g_chunk_count, 4);
        return;
    }

    uint16_t offset = read_u16_le(&data[1]);
    uint8_t chunk_len = data[3];

    if (chunk_len > CHUNK_SIZE)
    {
        PRINTF("T3V3 ERROR: chunk_len too large=%u\r\n", chunk_len);
        write_response(RSP_ERR, CMD_WRITE_CHUNK, offset, g_received_len, g_chunk_count, 5);
        return;
    }

    uint32_t expected_packet_len = 1 + 2 + 1 + chunk_len + 2;

    if (len != expected_packet_len)
    {
        PRINTF("T3V3 ERROR: chunk packet len=%u expected=%u chunk_len=%u offset=%u\r\n",
               (unsigned int)len,
               (unsigned int)expected_packet_len,
               chunk_len,
               offset);

        write_response(RSP_ERR, CMD_WRITE_CHUNK, offset, g_received_len, g_chunk_count, 6);
        return;
    }

    if ((offset + chunk_len) > ECG_INPUT_LEN)
    {
        PRINTF("T3V3 ERROR: chunk overflow offset=%u len=%u\r\n",
               offset,
               chunk_len);

        write_response(RSP_ERR, CMD_WRITE_CHUNK, offset, g_received_len, g_chunk_count, 7);
        return;
    }

    const uint8_t *payload = &data[4];

    uint16_t recv_sum = read_u16_le(&data[4 + chunk_len]);
    uint16_t calc_sum = checksum16(payload, chunk_len);

    if (recv_sum != calc_sum)
    {
        PRINTF("T3V3 ERROR: chunk checksum offset=%u calc=%u recv=%u\r\n",
               offset,
               calc_sum,
               recv_sum);

        write_response(RSP_ERR, CMD_WRITE_CHUNK, offset, g_received_len, g_chunk_count, 8);
        return;
    }

    memcpy(&g_input_buffer[offset], payload, chunk_len);

    uint16_t end_pos = offset + chunk_len;
    if (end_pos > g_received_len)
    {
        g_received_len = end_pos;
    }

    g_chunk_count++;

    /*
     * Always update response with the latest chunk.
     * ESP32 may read ACK only after a group of chunks.
     */
    write_response(RSP_OK, CMD_WRITE_CHUNK, offset, g_received_len, g_chunk_count, calc_sum);

    if ((g_chunk_count <= 3) ||
        (g_chunk_count % 20 == 0) ||
        (g_received_len == ECG_INPUT_LEN))
    {
        PRINTF("T3V3 CHUNK OK: count=%u offset=%u len=%u received=%u\r\n",
               (unsigned int)g_chunk_count,
               offset,
               chunk_len,
               g_received_len);
    }
}

static void handle_commit_sample(void)
{
    if (!g_session_started)
    {
        PRINTF("T3V3 ERROR: COMMIT before START\r\n");
        write_response(RSP_ERR, CMD_COMMIT_SAMPLE, 0, g_received_len, g_chunk_count, 9);
        return;
    }

    if (g_received_len != ECG_INPUT_LEN)
    {
        PRINTF("T3V3 ERROR: commit received=%u expected=%u chunks=%u\r\n",
               g_received_len,
               ECG_INPUT_LEN,
               (unsigned int)g_chunk_count);

        write_response(RSP_ERR, CMD_COMMIT_SAMPLE, 0, g_received_len, g_chunk_count, 10);
        return;
    }

    uint16_t calc_total = checksum16((const uint8_t *)g_input_buffer, ECG_INPUT_LEN);

    int8_t first_byte = g_input_buffer[0];
    int8_t mid_byte = g_input_buffer[ECG_INPUT_LEN / 2];
    int8_t last_byte = g_input_buffer[ECG_INPUT_LEN - 1];

    PRINTF("T3V3 COMMIT CHECK:\r\n");
    PRINTF("  record=%05u sample=%u true_label=%u\r\n",
           g_record_id,
           (unsigned int)g_sample_idx,
           g_true_label);

    PRINTF("  received_len=%u chunks=%u\r\n",
           g_received_len,
           (unsigned int)g_chunk_count);

    PRINTF("  expected_checksum=%u calc_checksum=%u\r\n",
           g_expected_checksum,
           calc_total);

    PRINTF("  first=%d mid=%d last=%d\r\n",
           first_byte,
           mid_byte,
           last_byte);

    if (calc_total != g_expected_checksum)
    {
        PRINTF("T3V3 RESULT: FAIL checksum mismatch\r\n");
        write_response(RSP_ERR, CMD_COMMIT_SAMPLE, 0, g_received_len, g_chunk_count, calc_total);
        return;
    }

    PRINTF("T3V3 COMMIT OK. Start inference request.\r\n");

    write_response(RSP_OK, CMD_COMMIT_SAMPLE, 0, g_received_len, g_chunk_count, calc_total);

    g_infer_request = true;
}

static void handle_received_packet(const uint8_t *data, uint32_t len)
{
    if (len < 1)
    {
        return;
    }

    uint8_t cmd = data[0];

    switch (cmd)
    {
        case CMD_PING:
        {
            write_response(RSP_OK, CMD_PING, 0, g_received_len, g_chunk_count, 0);
            break;
        }

        case CMD_RESET_SESSION:
        {
            reset_session();
            break;
        }

        case CMD_START_SAMPLE:
        {
            handle_start_sample(data, len);
            break;
        }

        case CMD_WRITE_CHUNK:
        {
            handle_write_chunk(data, len);
            break;
        }

        case CMD_COMMIT_SAMPLE:
        {
            handle_commit_sample();
            break;
        }

        default:
        {
            PRINTF("T3V3 ERROR: unknown cmd=0x%02X len=%u\r\n",
                   cmd,
                   (unsigned int)len);

            write_response(RSP_ERR, cmd, 0, g_received_len, g_chunk_count, 11);
            break;
        }
    }
}

/*******************************************************************************
 * Inference
 ******************************************************************************/

static void run_inference_from_i2c_buffer(void)
{
    if (g_model_input == NULL || g_output_data == NULL)
    {
        PRINTF("T3V3 ERROR: tensor pointer NULL\r\n");
        write_response(RSP_ERR, CMD_COMMIT_SAMPLE, 0, g_received_len, g_chunk_count, 20);
        return;
    }

    write_response(RSP_BUSY, CMD_COMMIT_SAMPLE, 0, g_received_len, g_chunk_count, 0);

    memcpy(g_model_input, g_input_buffer, ECG_INPUT_LEN);

    int start_time = TIMER_GetTimeInUS();
    status_t infer_status = MODEL_RunInference();
    int end_time = TIMER_GetTimeInUS();

    if (infer_status != kStatus_Success)
    {
        PRINTF("T3V3 ERROR: MODEL_RunInference failed\r\n");
        write_response(RSP_ERR, CMD_COMMIT_SAMPLE, 0, g_received_len, g_chunk_count, 21);
        return;
    }

    float score = MODEL_GetSigmoidScore(g_output_data, g_output_type);

    if (score < 0.0f)
    {
        score = 0.0f;
    }

    if (score > 1.0f)
    {
        score = 1.0f;
    }

    float threshold = (float)DETECTION_TRESHOLD / 100.0f;
    uint8_t pred_label = (score >= threshold) ? 1 : 0;

    uint16_t score_q15 = (uint16_t)(score * 32767.0f);
    uint32_t inference_us = (uint32_t)(end_time - start_time);

    memset(&g_response, 0, sizeof(g_response));

    g_response.magic = RSP_MAGIC;
    g_response.status = RSP_RESULT_READY;
    g_response.ack_cmd = CMD_COMMIT_SAMPLE;
    g_response.offset = 0;
    g_response.received_len = g_received_len;
    g_response.chunk_count = g_chunk_count;
    g_response.extra = g_expected_checksum;

    g_response.record_id = g_record_id;
    g_response.sample_idx = g_sample_idx;
    g_response.true_label = g_true_label;
    g_response.pred_label = pred_label;
    g_response.score_q15 = score_q15;
    g_response.inference_us = inference_us;

    g_response.checksum = 0;
    g_response.checksum = checksum16((const uint8_t *)&g_response, sizeof(g_response));

    PRINTF("T3V3 PREDICT: record=%05u sample=%u true=%u pred=%u score_q15=%u inference_us=%u\r\n",
           g_record_id,
           (unsigned int)g_sample_idx,
           g_true_label,
           pred_label,
           score_q15,
           (unsigned int)inference_us);
}

/*******************************************************************************
 * I2C callback
 ******************************************************************************/

static void I2C_SlaveCallback(LPI2C_Type *base,
                              lpi2c_slave_transfer_t *transfer,
                              void *userData)
{
    (void)base;
    (void)userData;

    switch (transfer->event)
    {
        case kLPI2C_SlaveReceiveEvent:
        {
            g_rx_active = true;

            memset(g_rx_buf, 0, sizeof(g_rx_buf));

            transfer->data = g_rx_buf;
            transfer->dataSize = sizeof(g_rx_buf);
            break;
        }

        case kLPI2C_SlaveCompletionEvent:
        {
            if (g_rx_active &&
                transfer->completionStatus == kStatus_Success &&
                transfer->transferredCount > 0)
            {
                if (g_rx_count < RX_QUEUE_DEPTH)
                {
                    uint32_t n = transfer->transferredCount;

                    if (n > RX_BUF_SIZE)
                    {
                        n = RX_BUF_SIZE;
                    }

                    memcpy(g_rx_queue[g_rx_head].data, g_rx_buf, n);
                    g_rx_queue[g_rx_head].len = n;

                    g_rx_head = (uint8_t)((g_rx_head + 1) % RX_QUEUE_DEPTH);
                    g_rx_count++;
                }
                else
                {
                    write_response(RSP_ERR, 0xEE, 0, g_received_len, g_chunk_count, 12);
                }
            }

            g_rx_active = false;
            break;
        }

        case kLPI2C_SlaveTransmitEvent:
        {
            transfer->data = (uint8_t *)&g_response;
            transfer->dataSize = sizeof(g_response);
            break;
        }

        default:
        {
            break;
        }
    }
}

/*******************************************************************************
 * I2C init
 ******************************************************************************/

static void I2C_SlaveInit(void)
{
    lpi2c_slave_config_t slave_config;

    /*
     * P4_0 = FC2_P0 = SDA
     * P4_1 = FC2_P1 = SCL
     */
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);

    LP_FLEXCOMM_Init(I2C_FLEXCOMM_INDEX, LP_FLEXCOMM_PERIPH_LPI2C);

    LPI2C_SlaveGetDefaultConfig(&slave_config);

    slave_config.address0 = I2C_SLAVE_ADDR;

    LPI2C_SlaveInit(I2C_SLAVE_BASE,
                    &slave_config,
                    CLOCK_GetFreq(kCLOCK_Fro12M));

    LPI2C_SlaveTransferCreateHandle(I2C_SLAVE_BASE,
                                    &g_lpi2c_slave_handle,
                                    I2C_SlaveCallback,
                                    NULL);

    LPI2C_SlaveTransferNonBlocking(I2C_SLAVE_BASE,
                                   &g_lpi2c_slave_handle,
                                   kLPI2C_SlaveAddressMatchEvent |
                                   kLPI2C_SlaveReceiveEvent |
                                   kLPI2C_SlaveTransmitEvent |
                                   kLPI2C_SlaveCompletionEvent);

    write_response(RSP_OK, 0x00, 0, 0, 0, 0);

    PRINTF("T3V3 I2C SLAVE READY\r\n");
    PRINTF("ADDR = 0x%02X\r\n", I2C_SLAVE_ADDR);
}

/*******************************************************************************
 * main
 ******************************************************************************/

int main(void)
{
    BOARD_Init();
    TIMER_Init();

    PRINTF("\r\n=== MCXN947 VERSION 3: GROUP ACK I2C INFERENCE ===\r\n");

    DEMO_PrintInfo();

    if (MODEL_Init() != kStatus_Success)
    {
        PRINTF("Failed initializing model\r\n");
        for (;;) {}
    }

    tensor_dims_t input_dims;
    tensor_type_t input_type;
    uint8_t *input_data = MODEL_GetInputTensorData(&input_dims, &input_type);

    tensor_dims_t output_dims;
    g_output_data = MODEL_GetOutputTensorData(&output_dims, &g_output_type);

    if (input_type != kTensorType_INT8)
    {
        PRINTF("ERROR: Unexpected input type. Need INT8.\r\n");
        for (;;) {}
    }

    g_model_input = reinterpret_cast<int8_t *>(input_data);

    PRINTF("MODEL READY\r\n");
    PRINTF("Input dims size=%u type=%d\r\n", (unsigned int)input_dims.size, (int)input_type);
    PRINTF("Output dims size=%u type=%d\r\n", (unsigned int)output_dims.size, (int)g_output_type);

    reset_session();

    I2C_SlaveInit();

    PRINTF("READY FOR VERSION 3\r\n");

    while (1)
    {
        if (g_rx_count > 0)
        {
            uint8_t local_buf[RX_BUF_SIZE];
            uint32_t local_len = 0;

            local_len = g_rx_queue[g_rx_tail].len;

            if (local_len > RX_BUF_SIZE)
            {
                local_len = RX_BUF_SIZE;
            }

            memcpy(local_buf, g_rx_queue[g_rx_tail].data, local_len);

            g_rx_tail = (uint8_t)((g_rx_tail + 1) % RX_QUEUE_DEPTH);
            g_rx_count--;

            handle_received_packet(local_buf, local_len);
        }

        if (g_infer_request)
        {
            g_infer_request = false;
            run_inference_from_i2c_buffer();
        }
    }
}

#include <Wire.h>
#include <SPI.h>
#include <SD.h>

/*******************************************************************************
 * I2C / protocol config
 ******************************************************************************/

#define MCXN947_I2C_ADDR   0x55

#define CMD_PING           0x01
#define CMD_START_SAMPLE   0x10
#define CMD_WRITE_CHUNK    0x11
#define CMD_COMMIT_SAMPLE  0x12
#define CMD_RESET_SESSION  0x7F

#define RSP_OK             0x00
#define RSP_ERR            0x01
#define RSP_BUSY           0x02
#define RSP_RESULT_READY   0x03

#define RSP_MAGIC          0x3354

/*******************************************************************************
 * Data config
 ******************************************************************************/

#define INPUT_LEN          2500

/*
 * IMPORTANT:
 * CHUNK_SIZE phải giống bên MCXN947.
 *
 * Nếu MCXN947 đang dùng bản ổn định v2/v3 với CHUNK_SIZE = 112,
 * giữ nguyên 112.
 */
#define CHUNK_SIZE         112

/*
 * v4 ưu tiên ổn định.
 * Nếu muốn giống v2 tuyệt đối: ACK_EVERY_N_CHUNKS = 1
 * Nếu MCXN947 v3 group ACK đã pass: có thể đổi thành 4.
 */
#define ACK_EVERY_N_CHUNKS 1

/*******************************************************************************
 * ESP32 hardware config
 ******************************************************************************/

#define SD_CS              5

#define I2C_SDA            21
#define I2C_SCL            22
#define I2C_FREQ           400000

/*******************************************************************************
 * Record config
 ******************************************************************************/

#define RECORD_ID          8405

#define X_PATH             "/loaddata/X_08405.bin"
#define Y_PATH             "/loaddata/y_08405.bin"
#define RESULT_PATH        "/result/result_08405.csv"
#define PROGRESS_PATH      "/progress.txt"

/*
 * 1 = bỏ qua progress.txt, xóa result cũ, chạy lại từ đầu.
 * 0 = nếu có progress.txt thì resume từ đó.
 */
#define RESET_RUN          0

/*
 * Dùng khi không có progress.txt hoặc RESET_RUN = 1.
 */
#define DEFAULT_START_SAMPLE_IDX 0

/*
 * 0 = chạy hết record.
 * 10 = chỉ chạy 10 sample để test.
 */
#define MAX_SAMPLES_TO_RUN 0

/*
 * Retry cho mỗi sample nếu lỗi I2C/result.
 */
#define MAX_RETRY_PER_SAMPLE 3

/*
 * 1 = nếu sample fail sau retry thì dừng chương trình.
 * 0 = ghi ERROR vào CSV rồi chạy tiếp sample sau.
 */
#define STOP_ON_FAILED_SAMPLE 1

#define FLUSH_EVERY        10
#define PROGRESS_EVERY     10

/*******************************************************************************
 * Global buffer
 ******************************************************************************/

int8_t input_buf[INPUT_LEN];

/*******************************************************************************
 * Response packet from MCXN947
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
} T3Response;

/*******************************************************************************
 * Error code
 ******************************************************************************/

enum ErrorCode : uint16_t
{
  ERR_NONE = 0,
  ERR_SD_READ = 1,
  ERR_RESET_ACK = 2,
  ERR_START_ACK = 3,
  ERR_CHUNK_ACK = 4,
  ERR_COMMIT_SEND = 5,
  ERR_RESULT_TIMEOUT = 6,
  ERR_RESPONSE_BAD = 7,
  ERR_CSV_WRITE = 8,
  ERR_PROGRESS_WRITE = 9,
};

const char *error_name(uint16_t err)
{
  switch (err)
  {
    case ERR_NONE:           return "NONE";
    case ERR_SD_READ:        return "SD_READ";
    case ERR_RESET_ACK:      return "RESET_ACK";
    case ERR_START_ACK:      return "START_ACK";
    case ERR_CHUNK_ACK:      return "CHUNK_ACK";
    case ERR_COMMIT_SEND:    return "COMMIT_SEND";
    case ERR_RESULT_TIMEOUT: return "RESULT_TIMEOUT";
    case ERR_RESPONSE_BAD:   return "RESPONSE_BAD";
    case ERR_CSV_WRITE:      return "CSV_WRITE";
    case ERR_PROGRESS_WRITE: return "PROGRESS_WRITE";
    default:                 return "UNKNOWN";
  }
}

/*******************************************************************************
 * Progress state
 ******************************************************************************/

typedef struct
{
  uint32_t next_sample_idx;
  uint32_t ok_count;
  uint32_t fail_count;
} ProgressState;

/*******************************************************************************
 * Per-sample timing/result
 ******************************************************************************/

typedef struct
{
  uint32_t sd_read_ms;
  uint32_t tx_ms;
  uint32_t wait_result_ms;
  uint32_t sd_write_ms;
  uint32_t total_ms;
  uint16_t error_code;
  T3Response response;
} SampleResult;

/*******************************************************************************
 * Function prototypes to avoid Arduino auto-prototype errors
 ******************************************************************************/

bool load_progress(ProgressState *p);

bool save_progress(uint32_t next_sample_idx,
                   uint32_t ok_count,
                   uint32_t fail_count,
                   const char *last_status,
                   uint16_t last_error);

uint16_t run_sample_with_retry(File &xFile,
                               File &yFile,
                               File &resultFile,
                               uint32_t sample_idx,
                               uint8_t *out_true_label,
                               uint8_t *out_retry_used,
                               SampleResult *out_result);

/*******************************************************************************
 * Utility
 ******************************************************************************/

uint16_t checksum16(const uint8_t *data, size_t len)
{
  uint32_t sum = 0;

  for (size_t i = 0; i < len; i++)
  {
    sum += data[i];
  }

  return (uint16_t)(sum & 0xFFFF);
}

void write_u16_le(uint8_t *p, uint16_t v)
{
  p[0] = v & 0xFF;
  p[1] = (v >> 8) & 0xFF;
}

void write_u32_le(uint8_t *p, uint32_t v)
{
  p[0] = v & 0xFF;
  p[1] = (v >> 8) & 0xFF;
  p[2] = (v >> 16) & 0xFF;
  p[3] = (v >> 24) & 0xFF;
}

bool i2c_write_bytes(const uint8_t *data, size_t len)
{
  Wire.beginTransmission(MCXN947_I2C_ADDR);

  size_t written = Wire.write(data, len);

  if (written != len)
  {
    Serial.printf("I2C write truncated: written=%u expected=%u\n",
                  (unsigned int)written,
                  (unsigned int)len);
    Wire.endTransmission(true);
    return false;
  }

  uint8_t err = Wire.endTransmission(true);

  if (err != 0)
  {
    Serial.printf("I2C write error=%u len=%u\n",
                  err,
                  (unsigned int)len);
    return false;
  }

  return true;
}

bool read_response(T3Response *rsp)
{
  int n = Wire.requestFrom(MCXN947_I2C_ADDR, (uint8_t)sizeof(T3Response));

  if (n != (int)sizeof(T3Response))
  {
    return false;
  }

  uint8_t *p = (uint8_t *)rsp;
  int i = 0;

  while (Wire.available() && i < (int)sizeof(T3Response))
  {
    p[i++] = Wire.read();
  }

  if (i != (int)sizeof(T3Response))
  {
    return false;
  }

  if (rsp->magic != RSP_MAGIC)
  {
    return false;
  }

  uint16_t recv_checksum = rsp->checksum;
  rsp->checksum = 0;

  uint16_t calc_checksum = checksum16((const uint8_t *)rsp, sizeof(T3Response));

  rsp->checksum = recv_checksum;

  if (recv_checksum != calc_checksum)
  {
    return false;
  }

  return true;
}

void print_response(const T3Response &rsp)
{
  Serial.printf("RSP: status=%u cmd=0x%02X offset=%u received=%u chunks=%u extra=%u rec=%05u sample=%lu true=%u pred=%u score=%.6f infer_us=%lu\n",
                rsp.status,
                rsp.ack_cmd,
                rsp.offset,
                rsp.received_len,
                rsp.chunk_count,
                rsp.extra,
                rsp.record_id,
                (unsigned long)rsp.sample_idx,
                rsp.true_label,
                rsp.pred_label,
                rsp.score_q15 / 32767.0f,
                (unsigned long)rsp.inference_us);
}

/*******************************************************************************
 * Progress functions
 ******************************************************************************/

bool load_progress(ProgressState *p)
{
  if (!SD.exists(PROGRESS_PATH))
  {
    return false;
  }

  File f = SD.open(PROGRESS_PATH, FILE_READ);
  if (!f)
  {
    return false;
  }

  p->next_sample_idx = DEFAULT_START_SAMPLE_IDX;
  p->ok_count = 0;
  p->fail_count = 0;

  while (f.available())
  {
    String line = f.readStringUntil('\n');
    line.trim();

    if (line.startsWith("next_sample_idx="))
    {
      p->next_sample_idx = line.substring(String("next_sample_idx=").length()).toInt();
    }
    else if (line.startsWith("ok_count="))
    {
      p->ok_count = line.substring(String("ok_count=").length()).toInt();
    }
    else if (line.startsWith("fail_count="))
    {
      p->fail_count = line.substring(String("fail_count=").length()).toInt();
    }
  }

  f.close();
  return true;
}

bool save_progress(uint32_t next_sample_idx,
                   uint32_t ok_count,
                   uint32_t fail_count,
                   const char *last_status,
                   uint16_t last_error)
{
  if (SD.exists(PROGRESS_PATH))
  {
    SD.remove(PROGRESS_PATH);
  }

  File f = SD.open(PROGRESS_PATH, FILE_WRITE);
  if (!f)
  {
    return false;
  }

  f.printf("record_id=%05u\n", RECORD_ID);
  f.printf("next_sample_idx=%lu\n", (unsigned long)next_sample_idx);
  f.printf("ok_count=%lu\n", (unsigned long)ok_count);
  f.printf("fail_count=%lu\n", (unsigned long)fail_count);
  f.printf("last_status=%s\n", last_status);
  f.printf("last_error=%u\n", last_error);
  f.printf("last_error_name=%s\n", error_name(last_error));

  f.close();
  return true;
}

/*******************************************************************************
 * Response waiting
 ******************************************************************************/

bool wait_response(uint8_t expected_status,
                   uint8_t expected_cmd,
                   uint16_t expected_offset,
                   uint32_t timeout_ms,
                   T3Response *out_rsp = nullptr,
                   bool verbose = false)
{
  uint32_t t0 = millis();

  while (millis() - t0 < timeout_ms)
  {
    T3Response rsp;

    if (read_response(&rsp))
    {
      if (verbose)
      {
        print_response(rsp);
      }

      if (rsp.status == RSP_ERR)
      {
        Serial.println("Response reports error:");
        print_response(rsp);
        return false;
      }

      if (rsp.status == expected_status &&
          rsp.ack_cmd == expected_cmd &&
          rsp.offset == expected_offset)
      {
        if (out_rsp != nullptr)
        {
          *out_rsp = rsp;
        }

        return true;
      }
    }

    delay(1);
  }

  return false;
}

bool wait_group_ack(uint16_t expected_offset,
                    uint16_t expected_received_len,
                    uint16_t expected_chunk_count,
                    uint32_t timeout_ms)
{
  uint32_t t0 = millis();

  while (millis() - t0 < timeout_ms)
  {
    T3Response rsp;

    if (read_response(&rsp))
    {
      if (rsp.status == RSP_ERR)
      {
        Serial.println("Group ACK reports error:");
        print_response(rsp);
        return false;
      }

      if (rsp.status == RSP_OK &&
          rsp.ack_cmd == CMD_WRITE_CHUNK &&
          rsp.offset == expected_offset)
      {
        if (rsp.received_len != expected_received_len ||
            rsp.chunk_count != expected_chunk_count)
        {
          Serial.println("Group ACK mismatch:");
          print_response(rsp);

          Serial.printf("Expected received=%u chunks=%u\n",
                        expected_received_len,
                        expected_chunk_count);

          return false;
        }

        return true;
      }
    }

    delay(1);
  }

  Serial.printf("Group ACK timeout at offset=%u expected_received=%u expected_chunks=%u\n",
                expected_offset,
                expected_received_len,
                expected_chunk_count);

  return false;
}

/*******************************************************************************
 * I2C command senders
 ******************************************************************************/

bool send_reset()
{
  uint8_t cmd = CMD_RESET_SESSION;

  if (!i2c_write_bytes(&cmd, 1))
  {
    return false;
  }

  delay(2);
  return wait_response(RSP_OK, CMD_RESET_SESSION, 0, 2000);
}

bool send_start_sample(uint16_t record_id,
                       uint32_t sample_idx,
                       uint8_t true_label,
                       uint16_t total_len,
                       uint16_t total_checksum)
{
  uint8_t pkt[12];

  pkt[0] = CMD_START_SAMPLE;

  write_u16_le(&pkt[1], record_id);
  write_u32_le(&pkt[3], sample_idx);

  pkt[7] = true_label;

  write_u16_le(&pkt[8], total_len);
  write_u16_le(&pkt[10], total_checksum);

  if (!i2c_write_bytes(pkt, sizeof(pkt)))
  {
    return false;
  }

  delay(2);
  return wait_response(RSP_OK, CMD_START_SAMPLE, 0, 2000);
}

bool send_chunk_no_ack(uint16_t offset, const uint8_t *payload, uint8_t chunk_len)
{
  uint8_t pkt[1 + 2 + 1 + CHUNK_SIZE + 2];

  pkt[0] = CMD_WRITE_CHUNK;

  write_u16_le(&pkt[1], offset);

  pkt[3] = chunk_len;

  memcpy(&pkt[4], payload, chunk_len);

  uint16_t chunk_sum = checksum16(payload, chunk_len);

  write_u16_le(&pkt[4 + chunk_len], chunk_sum);

  size_t packet_len = 1 + 2 + 1 + chunk_len + 2;

  return i2c_write_bytes(pkt, packet_len);
}

bool send_commit()
{
  uint8_t cmd = CMD_COMMIT_SAMPLE;

  if (!i2c_write_bytes(&cmd, 1))
  {
    return false;
  }

  /*
   * Không chờ RSP_OK ở đây.
   * MCXN947 có thể chuyển rất nhanh:
   * RSP_OK -> RSP_BUSY -> RSP_RESULT_READY.
   */
  delay(2);
  return true;
}

bool wait_inference_result(T3Response *result)
{
  return wait_response(RSP_RESULT_READY,
                       CMD_COMMIT_SAMPLE,
                       0,
                       120000,
                       result,
                       false);
}

/*******************************************************************************
 * SD read
 ******************************************************************************/

bool read_sample_from_files(File &xFile,
                            File &yFile,
                            uint32_t sample_idx,
                            int8_t *x_buf,
                            uint8_t *true_label)
{
  uint32_t x_offset = sample_idx * INPUT_LEN;

  if (!xFile.seek(x_offset))
  {
    Serial.printf("ERROR: Cannot seek X file at sample %lu\n",
                  (unsigned long)sample_idx);
    return false;
  }

  int n = xFile.read((uint8_t *)x_buf, INPUT_LEN);

  if (n != INPUT_LEN)
  {
    Serial.printf("ERROR: X read length=%d expected=%d at sample %lu\n",
                  n,
                  INPUT_LEN,
                  (unsigned long)sample_idx);
    return false;
  }

  if (!yFile.seek(sample_idx))
  {
    Serial.printf("ERROR: Cannot seek y file at sample %lu\n",
                  (unsigned long)sample_idx);
    return false;
  }

  int m = yFile.read(true_label, 1);

  if (m != 1)
  {
    Serial.printf("ERROR: Cannot read true label at sample %lu\n",
                  (unsigned long)sample_idx);
    return false;
  }

  return true;
}

/*******************************************************************************
 * Send one sample
 ******************************************************************************/

uint16_t send_real_sample_v4(uint16_t record_id,
                             uint32_t sample_idx,
                             uint8_t true_label,
                             int8_t *x_buf)
{
  uint16_t total_sum = checksum16((const uint8_t *)x_buf, INPUT_LEN);

  if (!send_start_sample(record_id, sample_idx, true_label, INPUT_LEN, total_sum))
  {
    return ERR_START_ACK;
  }

  uint16_t chunks_sent = 0;
  uint16_t group_count = 0;
  uint16_t latest_offset = 0;
  uint16_t latest_received = 0;

  for (uint16_t offset = 0; offset < INPUT_LEN; offset += CHUNK_SIZE)
  {
    uint8_t len = CHUNK_SIZE;

    if (offset + len > INPUT_LEN)
    {
      len = INPUT_LEN - offset;
    }

    if (!send_chunk_no_ack(offset, (const uint8_t *)&x_buf[offset], len))
    {
      Serial.printf("ERROR: CHUNK send failed sample=%lu offset=%u len=%u\n",
                    (unsigned long)sample_idx,
                    offset,
                    len);
      return ERR_CHUNK_ACK;
    }

    chunks_sent++;
    group_count++;

    latest_offset = offset;
    latest_received = offset + len;

    bool is_last_chunk = (latest_received >= INPUT_LEN);
    bool need_ack = (group_count >= ACK_EVERY_N_CHUNKS) || is_last_chunk;

    if (need_ack)
    {
      if (!wait_group_ack(latest_offset, latest_received, chunks_sent, 2000))
      {
        Serial.printf("ERROR: GROUP ACK failed sample=%lu offset=%u\n",
                      (unsigned long)sample_idx,
                      latest_offset);
        return ERR_CHUNK_ACK;
      }

      group_count = 0;
    }
  }

  if (!send_commit())
  {
    return ERR_COMMIT_SEND;
  }

  return ERR_NONE;
}

/*******************************************************************************
 * CSV
 ******************************************************************************/

bool write_result_header(File &resultFile)
{
  resultFile.println("record_id,sample_idx,true_label,pred_label,score,inference_us,sd_read_ms,tx_ms,wait_result_ms,sd_write_ms,total_ms,retry_count,status,error_code,error_name");
  return true;
}

bool write_ok_line(File &resultFile,
                   const T3Response &r,
                   uint32_t sd_read_ms,
                   uint32_t tx_ms,
                   uint32_t wait_result_ms,
                   uint32_t total_ms,
                   uint8_t retry_count)
{
  uint32_t write_start = millis();

  float score = r.score_q15 / 32767.0f;

  resultFile.printf("%05u,%lu,%u,%u,%.6f,%lu,%lu,%lu,%lu,%lu,%lu,%u,%s,%u,%s\n",
                    r.record_id,
                    (unsigned long)r.sample_idx,
                    r.true_label,
                    r.pred_label,
                    score,
                    (unsigned long)r.inference_us,
                    (unsigned long)sd_read_ms,
                    (unsigned long)tx_ms,
                    (unsigned long)wait_result_ms,
                    (unsigned long)0,
                    (unsigned long)total_ms,
                    retry_count,
                    "OK",
                    ERR_NONE,
                    error_name(ERR_NONE));

  (void)write_start;
  return true;
}

bool write_error_line(File &resultFile,
                      uint16_t record_id,
                      uint32_t sample_idx,
                      uint8_t true_label,
                      uint32_t sd_read_ms,
                      uint32_t tx_ms,
                      uint32_t wait_result_ms,
                      uint32_t total_ms,
                      uint8_t retry_count,
                      uint16_t error_code)
{
  resultFile.printf("%05u,%lu,%u,%d,%.6f,%lu,%lu,%lu,%lu,%lu,%lu,%u,%s,%u,%s\n",
                    record_id,
                    (unsigned long)sample_idx,
                    true_label,
                    -1,
                    0.0f,
                    (unsigned long)0,
                    (unsigned long)sd_read_ms,
                    (unsigned long)tx_ms,
                    (unsigned long)wait_result_ms,
                    (unsigned long)0,
                    (unsigned long)total_ms,
                    retry_count,
                    "ERROR",
                    error_code,
                    error_name(error_code));

  return true;
}

/*******************************************************************************
 * Run one sample with retry
 ******************************************************************************/

uint16_t run_sample_with_retry(File &xFile,
                               File &yFile,
                               File &resultFile,
                               uint32_t sample_idx,
                               uint8_t *out_true_label,
                               uint8_t *out_retry_used,
                               SampleResult *out_result)
{
  memset(out_result, 0, sizeof(SampleResult));
  out_result->error_code = ERR_NONE;

  uint32_t sample_start = millis();

  uint8_t true_label = 0;

  uint32_t t_read0 = millis();

  if (!read_sample_from_files(xFile, yFile, sample_idx, input_buf, &true_label))
  {
    out_result->sd_read_ms = millis() - t_read0;
    out_result->total_ms = millis() - sample_start;
    out_result->error_code = ERR_SD_READ;
    *out_true_label = true_label;
    *out_retry_used = 0;
    return ERR_SD_READ;
  }

  uint32_t t_read1 = millis();

  *out_true_label = true_label;

  out_result->sd_read_ms = t_read1 - t_read0;

  uint16_t last_error = ERR_NONE;
  uint8_t retry_used = 0;

  for (uint8_t attempt = 0; attempt <= MAX_RETRY_PER_SAMPLE; attempt++)
  {
    retry_used = attempt;

    if (!send_reset())
    {
      last_error = ERR_RESET_ACK;
      delay(100);
      continue;
    }

    uint32_t t_tx0 = millis();

    last_error = send_real_sample_v4(RECORD_ID, sample_idx, true_label, input_buf);

    uint32_t t_tx1 = millis();

    out_result->tx_ms = t_tx1 - t_tx0;

    if (last_error != ERR_NONE)
    {
      delay(100);
      continue;
    }

    uint32_t t_wait0 = millis();

    T3Response rsp;

    if (!wait_inference_result(&rsp))
    {
      last_error = ERR_RESULT_TIMEOUT;
      out_result->wait_result_ms = millis() - t_wait0;
      delay(100);
      continue;
    }

    uint32_t t_wait1 = millis();

    out_result->wait_result_ms = t_wait1 - t_wait0;
    out_result->response = rsp;
    out_result->total_ms = millis() - sample_start;
    out_result->error_code = ERR_NONE;

    *out_retry_used = retry_used;
    return ERR_NONE;
  }

  out_result->total_ms = millis() - sample_start;
  out_result->error_code = last_error;

  *out_retry_used = retry_used;
  return last_error;
}

/*******************************************************************************
 * setup
 ******************************************************************************/

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== ESP32 VERSION 4: ONE RECORD WITH RESUME + RETRY ===");

  Wire.begin(I2C_SDA, I2C_SCL);

#if defined(ARDUINO_ARCH_ESP32)
  Wire.setBufferSize(256);
#endif

  Wire.setClock(I2C_FREQ);

  SPI.begin(18, 19, 23, SD_CS);

  if (!SD.begin(SD_CS))
  {
    Serial.println("ERROR: SD mount failed");
    while (1)
    {
      delay(1000);
    }
  }

  Serial.println("SD OK");
  Serial.println("I2C OK");

  File xFile = SD.open(X_PATH, FILE_READ);
  if (!xFile)
  {
    Serial.println("ERROR: Cannot open X file");
    return;
  }

  File yFile = SD.open(Y_PATH, FILE_READ);
  if (!yFile)
  {
    Serial.println("ERROR: Cannot open y file");
    xFile.close();
    return;
  }

  uint32_t x_size = xFile.size();
  uint32_t y_size = yFile.size();

  uint32_t x_samples = x_size / INPUT_LEN;
  uint32_t y_samples = y_size;

  uint32_t total_samples = x_samples < y_samples ? x_samples : y_samples;

  if (MAX_SAMPLES_TO_RUN > 0 && MAX_SAMPLES_TO_RUN < total_samples)
  {
    total_samples = MAX_SAMPLES_TO_RUN;
  }

  Serial.printf("X file size=%lu bytes\n", (unsigned long)x_size);
  Serial.printf("y file size=%lu bytes\n", (unsigned long)y_size);
  Serial.printf("x_samples=%lu y_samples=%lu total_samples=%lu\n",
                (unsigned long)x_samples,
                (unsigned long)y_samples,
                (unsigned long)total_samples);

  if (x_size % INPUT_LEN != 0)
  {
    Serial.println("WARNING: X file size is not divisible by INPUT_LEN");
  }

  ProgressState progress;
  bool has_progress = false;

#if RESET_RUN
  Serial.println("RESET_RUN = 1, remove old progress/result");
  if (SD.exists(PROGRESS_PATH))
  {
    SD.remove(PROGRESS_PATH);
  }
  if (SD.exists(RESULT_PATH))
  {
    SD.remove(RESULT_PATH);
  }

  progress.next_sample_idx = DEFAULT_START_SAMPLE_IDX;
  progress.ok_count = 0;
  progress.fail_count = 0;
#else
  has_progress = load_progress(&progress);

  if (has_progress)
  {
    Serial.println("Progress loaded:");
    Serial.printf("  next_sample_idx=%lu\n", (unsigned long)progress.next_sample_idx);
    Serial.printf("  ok_count=%lu\n", (unsigned long)progress.ok_count);
    Serial.printf("  fail_count=%lu\n", (unsigned long)progress.fail_count);
  }
  else
  {
    progress.next_sample_idx = DEFAULT_START_SAMPLE_IDX;
    progress.ok_count = 0;
    progress.fail_count = 0;

    Serial.println("No progress found, start from DEFAULT_START_SAMPLE_IDX");

    if (SD.exists(RESULT_PATH))
    {
      Serial.println("Remove old result file because this is a fresh run");
      SD.remove(RESULT_PATH);
    }
  }
#endif

  if (progress.next_sample_idx >= total_samples)
  {
    Serial.println("All samples already completed or invalid progress.");
    xFile.close();
    yFile.close();
    return;
  }

  File resultFile = SD.open(RESULT_PATH, FILE_APPEND);
  if (!resultFile)
  {
    Serial.println("ERROR: Cannot open result CSV");
    xFile.close();
    yFile.close();
    return;
  }

  if (!has_progress || progress.next_sample_idx == 0)
  {
    write_result_header(resultFile);
    resultFile.flush();
  }

  if (!save_progress(progress.next_sample_idx,
                     progress.ok_count,
                     progress.fail_count,
                     "START",
                     ERR_NONE))
  {
    Serial.println("WARNING: Cannot write initial progress");
  }

  uint32_t global_start_ms = millis();

  uint32_t ok_count = progress.ok_count;
  uint32_t fail_count = progress.fail_count;

  for (uint32_t sample_idx = progress.next_sample_idx; sample_idx < total_samples; sample_idx++)
  {
    uint8_t true_label = 0;
    uint8_t retry_used = 0;
    SampleResult sr;

    uint16_t err = run_sample_with_retry(xFile,
                                         yFile,
                                         resultFile,
                                         sample_idx,
                                         &true_label,
                                         &retry_used,
                                         &sr);

    if (err == ERR_NONE)
    {
      write_ok_line(resultFile,
                    sr.response,
                    sr.sd_read_ms,
                    sr.tx_ms,
                    sr.wait_result_ms,
                    sr.total_ms,
                    retry_used);

      ok_count++;

      if (!save_progress(sample_idx + 1,
                         ok_count,
                         fail_count,
                         "OK",
                         ERR_NONE))
      {
        Serial.println("WARNING: Cannot save progress after OK");
      }

      if (ok_count % FLUSH_EVERY == 0)
      {
        resultFile.flush();
      }

      if ((sample_idx == progress.next_sample_idx) ||
          (ok_count % PROGRESS_EVERY == 0) ||
          (sample_idx + 1 == total_samples))
      {
        float score = sr.response.score_q15 / 32767.0f;

        Serial.printf("[OK] sample=%lu/%lu true=%u pred=%u score=%.6f infer_us=%lu read=%lu tx=%lu wait=%lu total=%lu retry=%u\n",
                      (unsigned long)sample_idx,
                      (unsigned long)(total_samples - 1),
                      sr.response.true_label,
                      sr.response.pred_label,
                      score,
                      (unsigned long)sr.response.inference_us,
                      (unsigned long)sr.sd_read_ms,
                      (unsigned long)sr.tx_ms,
                      (unsigned long)sr.wait_result_ms,
                      (unsigned long)sr.total_ms,
                      retry_used);
      }
    }
    else
    {
      write_error_line(resultFile,
                       RECORD_ID,
                       sample_idx,
                       true_label,
                       sr.sd_read_ms,
                       sr.tx_ms,
                       sr.wait_result_ms,
                       sr.total_ms,
                       retry_used,
                       err);

      resultFile.flush();

      fail_count++;

      if (!save_progress(sample_idx,
                         ok_count,
                         fail_count,
                         "ERROR",
                         err))
      {
        Serial.println("WARNING: Cannot save progress after ERROR");
      }

      Serial.printf("[ERROR] sample=%lu error=%s retry=%u\n",
                    (unsigned long)sample_idx,
                    error_name(err),
                    retry_used);

#if STOP_ON_FAILED_SAMPLE
      Serial.println("STOP_ON_FAILED_SAMPLE = 1, stop program.");
      break;
#else
      Serial.println("Continue to next sample.");
      save_progress(sample_idx + 1,
                    ok_count,
                    fail_count,
                    "SKIP_ERROR",
                    err);
#endif
    }
  }

  resultFile.flush();
  resultFile.close();
  xFile.close();
  yFile.close();

  uint32_t global_end_ms = millis();

  Serial.println();
  Serial.println("=== RECORD 08405 V4 DONE ===");
  Serial.printf("OK samples   = %lu\n", (unsigned long)ok_count);
  Serial.printf("Fail samples = %lu\n", (unsigned long)fail_count);
  Serial.printf("Elapsed ms   = %lu\n", (unsigned long)(global_end_ms - global_start_ms));
  Serial.printf("Result file  = %s\n", RESULT_PATH);
  Serial.printf("Progress file= %s\n", PROGRESS_PATH);
}

void loop()
{
}
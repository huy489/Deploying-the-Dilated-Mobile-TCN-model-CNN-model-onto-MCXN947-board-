#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#define I2C_SLAVE_ADDR 0x55
#define SDA_PIN 21
#define SCL_PIN 22
#define SD_CS_PIN 5 // Chân CS của module thẻ SD

#define TOTAL_LEN 2500
#define CHUNK_SIZE 64

uint8_t input_buffer[TOTAL_LEN];
int current_sample_idx = 8405; // Theo tên file bạn muốn test

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000); // Fast Mode 400kHz

  // Khởi tạo giao tiếp SD Card (Mặc định sử dụng chân 18, 19, 23)
  Serial.println("\nDang khoi tao the SD...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Loi: Khong the khoi tao the SD! Vui long kiem tra lai day cam.");
    while (1); // Dừng chương trình nếu không có SD
  }
  Serial.println("Khoi tao the SD thanh cong!");
}

void loop() {
  // --- BƯỚC 1: ĐỌC DỮ LIỆU TỪ FILE .BIN TRÊN THẺ SD ---
  // Mở file Data (X)
  File dataFile = SD.open("/loaddata/X_08405.bin", FILE_READ);
  // Mở file Label (y)
  File labelFile = SD.open("/loaddata/y_08405.bin", FILE_READ);

  if (!dataFile || !labelFile) {
    Serial.println("Loi: Khong tim thay file X_08405.bin hoac y_08405.bin trong thu muc /loaddata!");
    delay(5000);
    return;
  }

  // File .bin chứa dữ liệu nhị phân thô, ta đọc thẳng 2500 bytes vào buffer
  size_t bytesRead = dataFile.read(input_buffer, TOTAL_LEN);
  dataFile.close();

  // Đọc 1 byte Label
  uint8_t label = labelFile.read();
  labelFile.close();

  if (bytesRead < TOTAL_LEN) {
    Serial.println("Loi: File X_08405.bin khong du 2500 bytes!");
    delay(5000);
    return;
  }

  // --- BƯỚC 2: TÍNH CHECKSUM ---
  uint32_t checksum = 0;
  for (int i = 0; i < TOTAL_LEN; i++) {
    checksum += input_buffer[i];
  }
  checksum &= 0xFFFF; // Giới hạn ở 16-bit

  Serial.printf("\n============================================\n");
  Serial.printf("Send sample_idx=%d total_len=%d (Label: %d)\n", current_sample_idx, TOTAL_LEN, label);
  Serial.printf("Expected Checksum: 0x%04X\n", checksum);

  // --- BƯỚC 3: CHIA CHUNK VÀ GỬI QUA I2C ---
  int total_chunks = (TOTAL_LEN + CHUNK_SIZE - 1) / CHUNK_SIZE;

  for (int i = 0; i < total_chunks; i++) {
    int chunk_offset = i * CHUNK_SIZE;
    int len = (i == total_chunks - 1) ? (TOTAL_LEN % CHUNK_SIZE) : CHUNK_SIZE;
    if (len == 0) len = CHUNK_SIZE; // Xử lý trường hợp chia hết

    Wire.beginTransmission(I2C_SLAVE_ADDR);
    // Ép kiểu (const uint8_t*) như đã fix lỗi ở phần trước
    Wire.write((const uint8_t*)&input_buffer[chunk_offset], len);
    uint8_t err = Wire.endTransmission();

    if (err == 0) {
      Serial.printf("Send chunk %d OK\n", i);
    } else {
      Serial.printf("Send chunk %d FAILED (err=%d)\n", i, err);
      break; 
    }
    
    // Nghỉ 5ms để MCXN947 kịp xử lý ngắt lưu vào RAM
    delay(5); 
  }
  
  Serial.println("Commit OK");
  
  // --- BƯỚC 4: LẤY KẾT QUẢ TỪ MCXN947 ---
  delay(20); // Cho Slave thêm chút thời gian để tính toán Checksum
  
  Wire.requestFrom(I2C_SLAVE_ADDR, 16); // Yêu cầu 16 byte để đọc "RX_OK <idx> <checksum>"
  String resp = "";
  while (Wire.available()) {
    char c = Wire.read();
    if (c != 0) resp += c; // Bỏ qua ký tự NULL
  }
  Serial.printf("MCXN947 result: %s\n", resp.c_str());

  // Chờ 10 giây trước khi lặp lại bài test (để bạn dễ theo dõi log)
  delay(10000); 
}
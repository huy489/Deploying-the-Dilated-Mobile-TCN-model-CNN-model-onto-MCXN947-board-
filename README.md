# Accurate ECG Beat Detection on FRDM-MCXN947 & ESP32 Wroom

[![Platform: MCXN947](https://img.shields.io/badge/Platform-NXP%20FRDM--MCXN947-blue.svg)](https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/mcx-arm-cortex-m/mcx-n-series-microcontrollers/mcx-n94x-n54x-mcus:MCX-N94X-N54X)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32%20Wroom-red.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Framework: TensorFlow Lite Micro](https://img.shields.io/badge/Framework-TensorFlow%20Lite%20Micro-orange.svg)](https://www.tensorflow.org/lite/microcontrollers)
[![Accelerator: NXP eIQ Neutron NPU](https://img.shields.io/badge/Accelerator-NXP%20eIQ%20Neutron%20NPU-green.svg)](https://www.nxp.com/design/software/development-software/eiq-ml-software-development-environment:EIQ)

Dự án này triển khai mô hình học máy **DILATED-SE-FIRENET** để phân loại tín hiệu điện tâm đồ (**ECG Binary Classification**) thời gian thực. Dự án tận dụng bộ tăng tốc phần cứng **Neutron NPU** trên vi điều khiển **NXP MCXN947** kết hợp với **ESP32 Wroom** đóng vai trò bộ đọc dữ liệu từ thẻ nhớ SD và truyền qua giao tiếp I2C.

---

## 📋 Mục Lục
1. [Tổng Quan Bài Toán & Mô Hình AI](#-tổng-quan-bài-toán--mô-hình-ai)
2. [Kiến Trúc Hệ Thống](#-kiến-trúc-hệ-thống)
3. [Sơ Đồ Kết Nối Phần Cứng (Pin Mapping)](#-sơ-đồ-kết-nối-phần-cứng-pin-mapping)
4. [Cấu Trúc Thư Mục](#-cấu-trúc-thư-mục)
5. [Giao Thức Truyền Thông & Hướng Dẫn Chạy](#-giao-thức-truyền-thông--hướng-dẫn-chạy)
   - [Pipeline UART (PC ↔ MCXN947)](#1-pipeline-uart-pc--mcxn947)
   - [Pipeline I2C (ESP32 ↔ MCXN947)](#2-pipeline-i2c-esp32--mcxn947)
6. [Đánh Giá Hiệu Năng Mô Hình Tổng Thể](#-đánh-giá-hiệu-năng-mô-hình-tổng-thể)
7. [So Sánh Hiệu Năng Thực Tế: MCU vs PC (Record 08405)](#-so-sánh-hiệu-năng-thực-tế-mcu-vs-pc-record-08405)

---

## 🔍 Tổng Quan Bài Toán & Mô Hình AI

### 1. Bài toán phân loại
* **Đầu vào (Input):** Tín hiệu ECG 1 kênh (Single-channel ECG) có kích thước `(2500, 1)`, định dạng **Quantized INT8**.
* **Đầu ra (Output):** Phân loại nhị phân (Binary Classification):
  * `0`: Nhịp tim bình thường (**Normal**)
  * `1`: Rung nhĩ (**AFib** - Atrial Fibrillation)
* **Hàm kích hoạt lớp cuối (Last Layer):** Sử dụng hàm **Sigmoid** để trả về xác suất phân loại (Sigmoid Score).

### 2. Kiến trúc mô hình DILATED-SE-FIRENET
* **Đặc điểm kiến trúc:** Sử dụng các lớp tích chập giãn nở (**Dilated Convolution**) kết hợp cơ chế Squeeze-and-Excitation (SE) giúp nắm bắt tốt các đặc trưng phụ thuộc xa của tín hiệu ECG mà vẫn tối ưu số lượng tham số.
* **Số lượng tham số:** Cực kỳ nhỏ gọn với **68,546 tham số** (trong đó có 67,346 tham số có thể huấn luyện), rất thích hợp cho các thiết bị Edge AI tài nguyên hạn chế.
* **Tối ưu hóa phần cứng:** Biên dịch thông qua công cụ **NXP eIQ** để chuyển đổi sang toán tử tối ưu `NEUTRON_GRAPH`, chạy trực tiếp trên Neutron NPU tích hợp của MCXN947, mang lại tốc độ vượt trội so với chạy trên nhân CPU ARM Cortex-M33 đơn thuần.

---

## 🏗️ Kiến Trúc Hệ Thống

Dự án hỗ trợ song song hai luồng Pipeline chính phục vụ cho quá trình đánh giá (Evaluation) và triển khai thực tế (Edge Deployment):

### Luồng 1: PC ↔ MCXN947 (Giao tiếp UART)
Dùng để kiểm thử, đo lường độ chính xác và benchmark thời gian chạy của mô hình INT8 trên NPU của kit MCXN947 từ tập dữ liệu mẫu lưu trên PC.
```
[ PC: Dataset (.npy) ] 
       │  (Truyền từng mẫu 2500 điểm INT8 qua UART)
       ▼
[ MCXN947 Board (UART Mode) ] ──► [ TFLM + Neutron NPU Inference ] ──► [ Sigmoid Score ]
       │                                                                      │
       └────────────────◄ (Trả kết quả score & time về PC) ───────────────────┘
```

### Luồng 2: ESP32 ↔ MCXN947 (Giao tiếp I2C & SPI)
Mô phỏng một thiết bị IoT độc lập hoàn chỉnh ngoài thực tế.
* **ESP32 Wroom (I2C Master):** Đọc file ECG định dạng `.bin` từ thẻ nhớ SD qua giao tiếp SPI, sau đó cắt nhỏ dữ liệu thành từng chunk gửi qua I2C.
* **MCXN947 (I2C Slave):** Nhận đủ dữ liệu, thực hiện kiểm tra checksum, chạy suy luận NPU và trả kết quả nhãn dự đoán cùng thời gian suy luận về cho ESP32. ESP32 lưu lại kết quả kiểm tra vào thẻ nhớ SD dưới dạng file `.csv`.
```
[ SD Card ] ──(SPI)──► [ ESP32 (I2C Master) ]
                              │
                              │ (Truyền các chunk 112 bytes qua I2C - Addr: 0x55)
                              ▼
                       [ MCXN947 (I2C Slave) ] ──► [ TFLM + NPU Inference ]
                              │                              │
                              ◄── (Trả kết quả cấu trúc T3Response)──┘
```

---

## 🔌 Sơ Đồ Kết Nối Phần Cứng (Pin Mapping)

Chi tiết kết nối giữa các board mạch được cấu hình như sau (Tham khảo thêm từ file [map_pin.md](file:///media/pd/data/Biosignal/demo_model_tinyML/AI_model_DK_Deployment/Deploying-the-Dilated-Mobile-TCN-model-CNN-model-onto-MCXN947-board-/map_pin.md)):

### 1. Kết nối giữa ESP32 (Master) và MCXN947 (Slave)
Kết nối bus I2C (sử dụng module LPI2C2 trên MCXN947):
* **ESP32 Pin 21 (SDA)** ↔ **MCXN947 Pin P4_0 (LPI2C2_SDA)**
* **ESP32 Pin 22 (SCL)** ↔ **MCXN947 Pin P4_1 (LPI2C2_SCL)**
* **ESP32 GND** ↔ **MCXN947 GND**

> [!NOTE]
> Kết nối I2C cần đảm bảo có điện trở kéo lên (Pull-up resistors 4.7kΩ) nếu đường truyền không ổn định hoặc các chân GPIO không được cấu hình pull-up nội bộ.

### 2. Kết nối giữa ESP32 và Module Thẻ nhớ SD (SPI Mode)
* **GND** ↔ **GND**
* **VCC** ↔ **+3V/3.3V**
* **MISO** ↔ **Pin 19**
* **MOSI** ↔ **Pin 23**
* **SCK** ↔ **Pin 18**
* **CS** ↔ **Pin 5**

---

## 📂 Cấu Trúc Thư Mục

```text
.
├── esp32/
│   ├── esp32_i2c_mcxn947/    # Bản nháp truyền I2C cơ bản (Chunk size 128)
│   ├── i2c_test2/            # Bản thử nghiệm I2C với chunk size 64
│   └── i2c_test3/            # Bản chính thức v4 ổn định (Chunk size 112, Group ACK, Checksum 16-bit)
├── frdmmcxn947_tflm_cifar10_cm33_core0/ # Mã nguồn MCXN947 chạy chế độ UART (PC-driven)
│   ├── source/               # Chứa main.cpp giao tiếp UART
│   └── ...
├── source_mcxn947_i2c/       # Mã nguồn MCXN947 chạy chế độ I2C Slave
│   └── frdmmcxn947_tflm_cifar10_cm33_core0/
│       ├── source/           # Chứa main.cpp cấu hình LPI2C2 Slave (Địa chỉ 0x55)
│       └── ...
├── model/
│   ├── main model/           # File mô hình gốc Keras (.keras)
│   ├── model_float32/        # File mô hình TFLite dạng số thực 32-bit và file header C/C++
│   ├── model_int8/           # File mô hình TFLite lượng tử hóa INT8 chạy trên NPU và file header C/C++
│   └── info/                 # Tài liệu nghiên cứu chi tiết, file preprocessing và train model
├── deploy_model/             # Tài liệu chi tiết quá trình deploy từ float32 sang int8 lên board
│   └── deploy_model.pdf
├── individual_records/       # Dữ liệu ECG mẫu dạng NumPy (.npy) và file header để test offline
├── script_UART_PC/           # Script Python chạy trên PC để gửi dữ liệu kiểm thử qua UART
│   └── send_ecg_uart.py
├── map_pin.md                # Sơ đồ cấu hình chân kết nối phần cứng
├── OVERVIEW.md               # Mô tả chi tiết tổng quan bài toán và pipeline UART
├── TREE.md                   # Cây thư mục chi tiết của toàn bộ dự án
└── README.md                 # Tài liệu hướng dẫn này
```

---

## ⚙️ Giao Thức Truyền Thông & Hướng Dẫn Chạy

### 1. Pipeline UART (PC ↔ MCXN947)

#### A. Định dạng gói tin UART gửi từ PC
Mỗi gói dữ liệu ECG truyền qua UART gồm các trường sau (định dạng Little-Endian):
1. `MAGIC_WORD`: 4 bytes (`0x45434731` / `"ECG1"`)
2. `CMD`: 1 byte (`0x01` = CMD_DATA, `0x02` = CMD_END)
3. *Nếu là CMD_DATA (0x01):*
   - `sample_idx`: 4 bytes (vị trí mẫu dữ liệu)
   - `true_label`: 1 byte (nhãn thực tế: `0` hoặc `1`)
   - `payload_len`: 2 bytes (độ dài dữ liệu: Bắt buộc là `2500` bytes)
   - `payload`: 2500 bytes (tín hiệu ECG định dạng INT8)
   - `checksum`: 4 bytes (tổng đại số của các phần tử trong payload)

#### B. Phản hồi từ MCXN947
Sau khi thực hiện suy luận, board gửi trả chuỗi ký tự kết quả có định dạng:
`RESULT,<sample_idx>,<true_label>,<pred_label>,<score_micro>,<inference_time_us>`
*(Ví dụ: `RESULT,5,0,0,12345,6420` nghĩa là mẫu thứ 5, nhãn thực tế là 0, dự đoán là 0, sigmoid score là 0.012345, thời gian suy luận là 6.42 ms).*

#### C. Hướng dẫn chạy
1. Nạp chương trình UART cho board MCXN947 bằng cách nạp project nằm trong thư mục [frdmmcxn947_tflm_cifar10_cm33_core0](file:///media/pd/data/Biosignal/demo_model_tinyML/AI_model_DK_Deployment/Deploying-the-Dilated-Mobile-TCN-model-CNN-model-onto-MCXN947-board-/frdmmcxn947_tflm_cifar10_cm33_core0) qua IDE **MCUXpresso**.
2. Cắm cáp USB kết nối kit MCXN947 với PC để nhận cổng COM ảo.
3. Cài đặt các thư viện Python trên PC:
   ```bash
   pip install numpy pyserial tensorflow scikit-learn
   ```
4. Mở file [send_ecg_uart.py](file:///media/pd/data/Biosignal/demo_model_tinyML/AI_model_DK_Deployment/Deploying-the-Dilated-Mobile-TCN-model-CNN-model-onto-MCXN947-board-/script_UART_PC/send_ecg_uart.py), sửa lại tham số `PORT` trùng với cổng COM ảo của kit (ví dụ `COM6` trên Windows hoặc `/dev/ttyACM0` trên Linux) và kiểm tra chính xác các đường dẫn tệp.
5. Khởi chạy script Python:
   ```bash
   python script_UART_PC/send_ecg_uart.py
   ```
6. Script sẽ truyền dữ liệu từ file `individual_records/X_08405.npy` qua cổng serial, nhận kết quả phân tích từ board mạch và in bảng báo cáo chi tiết về độ chính xác (Accuracy, Sensitivity, Specificity, AUC Score, Confusion Matrix).

---

### 2. Pipeline I2C (ESP32 ↔ MCXN947)

#### A. Định dạng giao thức I2C v4 (Group ACK)
Giao thức I2C hoạt động với cơ chế Master-Slave tin cậy, ESP32 điều khiển quá trình gửi và nhận kết quả:
* **CMD_RESET_SESSION (0x7F):** Reset phiên truyền nhận trên Slave.
* **CMD_START_SAMPLE (0x10):** Gửi gói tin bắt đầu mẫu (12 bytes) chứa `record_id`, `sample_idx`, `true_label`, `total_len` và `checksum` tổng.
* **CMD_WRITE_CHUNK (0x11):** Chia nhỏ 2500 bytes tín hiệu thành các gói `112 bytes` kèm theo thông tin offset và checksum cục bộ của chunk.
* **Group ACK:** Board MCXN947 sẽ tích lũy gói tin trong hàng đợi ngắt (RX Queue) và trả về mã ACK sau khi nhận đủ mỗi `N` chunk (mặc định `N=1` để đạt độ tin cậy tuyệt đối) để ESP32 tiếp tục gửi.
* **CMD_COMMIT_SAMPLE (0x12):** Kích hoạt board chạy suy luận TFLite Micro trên NPU. Board chuyển trạng thái sang `RSP_BUSY`.
* **Đọc kết quả:** ESP32 đọc kết quả dưới dạng cấu trúc nhị phân `T3Response` (32 bytes) chứa nhãn dự đoán (`pred_label`), xác suất dạng Q15 (`score_q15`) và thời gian suy luận (`inference_us`).

#### B. Hướng dẫn chạy
1. Mở project [source_mcxn947_i2c/frdmmcxn947_tflm_cifar10_cm33_core0](file:///media/pd/data/Biosignal/demo_model_tinyML/AI_model_DK_Deployment/Deploying-the-Dilated-Mobile-TCN-model-CNN-model-onto-MCXN947-board-/source_mcxn947_i2c/frdmmcxn947_tflm_cifar10_cm33_core0) trong MCUXpresso IDE, biên dịch và nạp xuống board MCXN947.
2. Dùng Arduino IDE nạp chương trình [i2c_test3.ino](file:///media/pd/data/Biosignal/demo_model_tinyML/AI_model_DK_Deployment/Deploying-the-Dilated-Mobile-TCN-model-CNN-model-onto-MCXN947-board-/esp32/i2c_test3/i2c_test3.ino) xuống ESP32.
3. Định dạng thẻ nhớ SD thành FAT32. Tạo thư mục `/loaddata` trên thẻ nhớ và chép dữ liệu các file ECG dạng nhị phân thô (`X_08405.bin`, `y_08405.bin`) vào thư mục này.
4. Cắm thẻ nhớ SD vào module đọc thẻ trên ESP32.
5. Tiến hành nối dây giữa ESP32 và kit MCXN947 theo [Sơ đồ kết nối phần cứng](#-sơ-đồ-kết-nối-phần-cứng-pin-mapping).
6. Cấp nguồn cho cả 2 board. ESP32 sẽ tự động thực hiện quá trình đọc tệp từ thẻ SD, gửi dữ liệu qua I2C đến MCXN947, nhận phản hồi kết quả suy luận AI và ghi log kết quả chi tiết thành tệp CSV trong thư mục `/result` trên thẻ nhớ SD.

---

## 📈 Đánh Giá Hiệu Năng Mô Hình Tổng Thể

Báo cáo kết quả huấn luyện mô hình **DILATED-SE-FIRENET** trên tập dữ liệu kiểm thử (Test Set) gồm **62,205 mẫu** cho thấy độ tin cậy cực kỳ cao, đặc biệt là khả năng phát hiện bệnh không bỏ sót nhịp (Độ nhạy - Sensitivity đạt 100%):

### Bảng chỉ số chi tiết (Classification Report)

| Nhãn phân loại | Precision | Recall (Độ nhạy) | F1-Score | Số lượng mẫu (Support) |
|:---:|:---:|:---:|:---:|:---:|
| **Normal (0)** | 1.0000 | 0.8914 | 0.9426 | 33,990 |
| **AFib (1)** | 0.8843 | 1.0000 | 0.9386 | 28,215 |
| **Trung bình chung** | **0.9475** | **0.9406** | **0.9408** | **62,205** |

* **Độ chính xác tổng thể (Overall Accuracy):** **94.06%**
* **Độ nhạy phân loại AFib (Sensitivity):** **100.00%** (Cực kỳ quan trọng trong y tế để tránh bỏ sót bệnh nhân mắc Rung nhĩ)
* **Độ đặc hiệu (Specificity):** **89.14%** (Tỷ lệ phân loại đúng các mẫu nhịp tim bình thường)

---

## 📊 So Sánh Hiệu Năng Thực Tế: MCU vs PC (Record 08405)

Dưới đây là so sánh chi tiết hiệu năng của mô hình **DILATED-SE-FIRENET** lượng tử hóa INT8 khi chạy thực tế trên vi điều khiển **MCXN947** (Neutron NPU) so với khi chạy giả lập trên **PC** (TensorFlow Lite Interpreter). Phép thử được thực hiện trên toàn bộ **7,301 mẫu** của bản ghi `08405` thuộc tập dữ liệu MIT-BIH:

### 1. Ma trận nhầm lẫn (Confusion Matrix)

| Thiết bị | True Negative (TN) | True Positive (TP) | False Negative (FN) | False Positive (FP) |
|:---:|:---:|:---:|:---:|:---:|
| **MCU (NPU)** | 2041 | 5254 | 5 | 1 |
| **PC (TFLite)** | 2041 | 5258 | 1 | 1 |

### 2. Chi tiết theo từng lớp nhãn

#### Lớp Normal (0) - Support: 2042 mẫu
* **MCU:** Precision = 0.9976 | Recall = 0.9995 | F1-Score = 0.9985
* **PC:** Precision = 0.9995 | Recall = 0.9995 | F1-Score = 0.9995

#### Lớp AFib (1) - Support: 5259 mẫu
* **MCU:** Precision = 0.9998 | Recall = 0.9990 | F1-Score = 0.9994
* **PC:** Precision = 0.9998 | Recall = 0.9998 | F1-Score = 0.9998

### 3. Các chỉ số tổng quát (Overall Metrics)

| Chỉ số đánh giá | MCU (eIQ Neutron NPU) | PC (TFLite Float/Int8) |
|:---|:---:|:---:|
| **Accuracy (Độ chính xác)** | **99.92%** (7295/7301) | **99.97%** (7299/7301) |
| **Sensitivity (Recall AFib)** | **99.90%** | **99.98%** |
| **Specificity (Recall Normal)** | **99.95%** | **99.95%** |
| **AUC Score** | **1.0000** | **1.0000** |

> [!TIP]
> **Nhận xét:** Sự sai biệt về độ chính xác giữa MCU (Neutron NPU) và PC là cực kỳ nhỏ (chỉ lệch vỏn vẹn **4 mẫu** trên tổng số **7,301 mẫu** thử nghiệm, tương đương **0.05%**). Điều này chứng minh quá trình lượng tử hóa mô hình sang INT8 và biên dịch tối ưu hóa qua eIQ Compiler sang toán tử Neutron NPU diễn ra vô cùng thành công và chính xác, giữ nguyên vẹn chất lượng chẩn đoán y tế ban đầu của mô hình.

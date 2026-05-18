# Deploying Dilated Mobile TCN / CNN ECG Model onto MCXN947 Board

Dự án này tập trung vào việc triển khai (deploy) mô hình học máy phân loại tín hiệu điện tâm đồ (ECG Binary Classification) phiên bản định dạng **Quantized INT8** lên vi điều khiển **NXP MCXN947**, tận dụng bộ tăng tốc NPU phần cứng. 

Pipeline truyền nhận dữ liệu được thực hiện thời gian thực thông qua giao tiếp **UART** giữa Máy tính (PC) và Board mạch.

---

## 1. Tổng Quan Bài Toán & Mục Tiêu

### Bài toán (Problem Statement)
* **Input:** Tín hiệu ECG 1 kênh (Single-channel ECG signal).
* **Output:** Phân loại nhị phân (Binary Classification):
  * `0`: Bình thường (Normal)
  * `1`: Rung nhĩ (AFib - Atrial Fibrillation)
* **Kích thước dữ liệu mẫu:** Mỗi sample đầu vào có kích thước `(2500, 1)`.
* **Layer cuối cùng:** Sử dụng hàm **Sigmoid** (1 node output), không phải Softmax.

### Kiến trúc Pipeline Triển Khai

```text
[ PC: Dataset (NPY) ] 
       │  (Gửi từng sample 2500 điểm INT8 qua UART)
       ▼
[ MCXN947 Board ] ──► [ NPU Inference ] ──► [ Tính toán Sigmoid Score ]
       │                                                   │
       └────────────────◄ (Gửi kết quả score về) ──────────┘
       ▼
[ PC: Evaluation ] ──► Tính toán Metrics: Accuracy, F1-Score, AUC, Sensitivity, Specificity...
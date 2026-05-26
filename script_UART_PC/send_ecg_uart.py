import struct
import time
import numpy as np
import serial
import serial.tools.list_ports
import tensorflow as tf

# =========================================================
# Cấu hình
# =========================================================
PORT = "COM6"          # đổi đúng theo board của bạn
BAUDRATE = 115200
TIMEOUT = 1.0

MAGIC_WORD = 0x45434731   # "ECG1"
CMD_DATA = 1
CMD_END = 2
ECG_INPUT_LEN = 2500

x_path = r"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\individual_records\X_08405.npy"
y_path = r"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\individual_records\y_08405.npy"
tflite_model_path = r"D:\Biosignal\demo_model_tinyML\AI_model_DK_Deployment\exe\model_AI\model_int8\19k_best_model_fixed_split_recalibrated_int8.tflite"

# timeout chờ board trả RESULT cho mỗi sample
RESULT_TIMEOUT_SEC = 5.0
READY_TIMEOUT_SEC = 10.0
END_ACK_TIMEOUT_SEC = 10.0


# =========================================================
# Helper: liệt kê COM ports
# =========================================================
def list_com_ports():
    print("Available COM ports:")
    for p in serial.tools.list_ports.comports():
        print(f"  {p.device} - {p.description}")


# =========================================================
# Quantization
# =========================================================
def get_tflite_input_quant_params(model_path: str):
    interpreter = tf.lite.Interpreter(model_path=model_path)
    interpreter.allocate_tensors()
    input_details = interpreter.get_input_details()

    scale, zero_point = input_details[0]["quantization"]
    dtype = input_details[0]["dtype"]
    shape = input_details[0]["shape"]

    print("Input shape:", shape)
    print("Input dtype:", dtype)
    print("Input quantization:", (scale, zero_point))

    if dtype != np.int8:
        raise ValueError(f"Model input dtype phải là int8, hiện là {dtype}")
    if scale == 0:
        raise ValueError("Input scale = 0")

    return scale, zero_point


def quantize_to_int8(x_float: np.ndarray, scale: float, zero_point: int) -> np.ndarray:
    x_q = x_float / scale + zero_point
    x_q = np.round(x_q)
    x_q = np.clip(x_q, -128, 127).astype(np.int8)
    return x_q


# =========================================================
# Packet helpers
# =========================================================
def checksum_sum_u8(data_bytes: bytes) -> int:
    return sum(data_bytes) & 0xFFFFFFFF


def build_data_packet(sample_idx: int, true_label: int, payload_int8: np.ndarray) -> bytes:
    payload = payload_int8.reshape(-1)

    if len(payload) != ECG_INPUT_LEN:
        raise ValueError(f"payload length phải là {ECG_INPUT_LEN}, hiện là {len(payload)}")

    payload_bytes = payload.astype(np.int8).tobytes()
    checksum = checksum_sum_u8(payload_bytes)

    pkt = bytearray()
    pkt += struct.pack("<I", MAGIC_WORD)
    pkt += struct.pack("<B", CMD_DATA)
    pkt += struct.pack("<I", sample_idx)
    pkt += struct.pack("<B", int(true_label))
    pkt += struct.pack("<H", ECG_INPUT_LEN)
    pkt += payload_bytes
    pkt += struct.pack("<I", checksum)
    return bytes(pkt)


def build_end_packet(total_sent: int) -> bytes:
    pkt = bytearray()
    pkt += struct.pack("<I", MAGIC_WORD)
    pkt += struct.pack("<B", CMD_END)
    pkt += struct.pack("<I", total_sent)
    return bytes(pkt)


# =========================================================
# Metrics
# =========================================================
def safe_div(a, b):
    return 0.0 if b == 0 else a / b


def compute_auc(y_true, y_score):
    y_true = np.asarray(y_true, dtype=np.int32)
    y_score = np.asarray(y_score, dtype=np.float32)

    pos = np.sum(y_true == 1)
    neg = np.sum(y_true == 0)
    if pos == 0 or neg == 0:
        return 0.0

    order = np.argsort(-y_score)
    y_true_sorted = y_true[order]

    tp = 0.0
    fp = 0.0
    prev_tpr = 0.0
    prev_fpr = 0.0
    auc = 0.0

    for label in y_true_sorted:
        if label == 1:
            tp += 1.0
        else:
            fp += 1.0

        tpr = tp / pos
        fpr = fp / neg
        auc += (fpr - prev_fpr) * (tpr + prev_tpr) * 0.5
        prev_tpr = tpr
        prev_fpr = fpr

    return float(auc)


def compute_and_print_metrics(results):
    if len(results) == 0:
        print("Không có kết quả nào để evaluate.")
        return

    y_true = np.array([r["true_label"] for r in results], dtype=np.int32)
    y_pred = np.array([r["pred_label"] for r in results], dtype=np.int32)
    y_score = np.array([r["score"] for r in results], dtype=np.float32)

    tp = int(np.sum((y_true == 1) & (y_pred == 1)))
    tn = int(np.sum((y_true == 0) & (y_pred == 0)))
    fp = int(np.sum((y_true == 0) & (y_pred == 1)))
    fn = int(np.sum((y_true == 1) & (y_pred == 0)))

    support_neg = int(np.sum(y_true == 0))
    support_pos = int(np.sum(y_true == 1))

    precision_pos = safe_div(tp, tp + fp)
    recall_pos = safe_div(tp, tp + fn)
    f1_pos = safe_div(2 * precision_pos * recall_pos, precision_pos + recall_pos)

    precision_neg = safe_div(tn, tn + fn)
    recall_neg = safe_div(tn, tn + fp)
    f1_neg = safe_div(2 * precision_neg * recall_neg, precision_neg + recall_neg)

    accuracy = safe_div(tp + tn, len(y_true))
    sensitivity = recall_pos
    specificity = safe_div(tn, tn + fp)
    auc = compute_auc(y_true, y_score)

    avg_inference_us = np.mean([r["inference_us"] for r in results])

    print("\n========================================")
    print("FINAL EVALUATION REPORT (PC SIDE)")
    print("========================================")
    print("Confusion Matrix:")
    print(f"  TN = {tn}, FP = {fp}")
    print(f"  FN = {fn}, TP = {tp}")

    print("\nNormal (0):")
    print(f"  precision = {precision_neg:.4f}")
    print(f"  recall    = {recall_neg:.4f}")
    print(f"  f1-score  = {f1_neg:.4f}")
    print(f"  support   = {support_neg}")

    print("\nAFib (1):")
    print(f"  precision = {precision_pos:.4f}")
    print(f"  recall    = {recall_pos:.4f}")
    print(f"  f1-score  = {f1_pos:.4f}")
    print(f"  support   = {support_pos}")

    print("\nOverall:")
    print(f"  accuracy    = {accuracy:.4f}")
    print(f"  sensitivity = {sensitivity:.4f}")
    print(f"  specificity = {specificity:.4f}")
    print(f"  AUC Score   = {auc:.4f}")
    print(f"  Avg infer   = {avg_inference_us:.2f} us")
    print("========================================")


# =========================================================
# UART line helpers
# =========================================================
def read_line_from_board(ser, timeout_sec):
    t0 = time.time()
    while time.time() - t0 < timeout_sec:
        line = ser.readline().decode(errors="ignore").strip()
        if line:
            return line
    return None


def wait_for_ready(ser):
    print("Waiting for READY from board...")
    t0 = time.time()
    while time.time() - t0 < READY_TIMEOUT_SEC:
        line = ser.readline().decode(errors="ignore").strip()
        if line:
            print("[BOARD]", line)
        if line == "READY":
            return True
    return False


def wait_for_result(ser, idx):
    t0 = time.time()

    while time.time() - t0 < RESULT_TIMEOUT_SEC:
        line = ser.readline().decode(errors="ignore").strip()

        if not line:
            continue

        print("[BOARD]", line)

        if line.startswith("RESULT,"):
            print("RAW RESULT LINE =", repr(line))
            parts = line.split(",")

            if len(parts) != 6:
                print("Malformed RESULT line, skip:", parts)
                continue

            try:
                sample_idx = int(parts[1])
                true_label = int(parts[2])
                pred_label = int(parts[3])
                score = int(parts[4]) / 1_000_000.0
                inference_us = int(parts[5])
            except ValueError:
                print("Parse RESULT failed, skip:", parts)
                continue

            return {
                "sample_idx": sample_idx,
                "true_label": true_label,
                "pred_label": pred_label,
                "score": score,
                "inference_us": inference_us,
            }

        elif line.startswith("ERROR,"):
            print(f"Board báo lỗi khi xử lý sample {idx}: {line}")
            return None

    raise RuntimeError(f"Timeout chờ RESULT cho sample {idx}")


def wait_for_end_ack(ser):
    print("\nWaiting END_ACK...")
    t0 = time.time()
    while time.time() - t0 < END_ACK_TIMEOUT_SEC:
        line = ser.readline().decode(errors="ignore").strip()
        if not line:
            continue

        print("[BOARD]", line)
        if line.startswith("END_ACK,"):
            return True
    return False


# =========================================================
# Main
# =========================================================
def main():
    print("===== ECG UART Sender =====")
    list_com_ports()

    x_data = np.load(x_path).astype(np.float32)
    y_data = np.load(y_path).astype(np.int8)

    # chọn range sample muốn test
    start_idx = 0
    end_idx = len(x_data) - 1

    print("x shape:", x_data.shape)
    print("y shape:", y_data.shape)

    if x_data.shape[0] != y_data.shape[0]:
        raise ValueError("Số sample X và y không khớp")

    if start_idx < 0 or end_idx >= x_data.shape[0] or start_idx > end_idx:
        raise ValueError("Range start/end không hợp lệ")

    scale, zero_point = get_tflite_input_quant_params(tflite_model_path)

    ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)
    time.sleep(2.0)

    try:
        if not wait_for_ready(ser):
            raise RuntimeError("Không nhận được READY từ board.")

        results = []
        total_sent = 0

        for idx in range(start_idx, end_idx + 1):
            x_sample = x_data[idx]      # shape (2500, 1)
            y_true = int(y_data[idx])

            x_q = quantize_to_int8(x_sample, scale, zero_point)

            pkt = build_data_packet(idx, y_true, x_q)
            ser.write(pkt)
            ser.flush()

            result = wait_for_result(ser, idx)
            if result is not None:
                results.append(result)
                total_sent += 1

        end_pkt = build_end_packet(total_sent)
        ser.write(end_pkt)
        ser.flush()

        if not wait_for_end_ack(ser):
            print("Warning: không nhận được END_ACK từ board.")

        compute_and_print_metrics(results)

    finally:
        ser.close()


if __name__ == "__main__":
    main()
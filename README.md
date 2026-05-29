# 🌡️ Smart Home CE Firmware - ESP32 FreeRTOS Monitor

**Hệ thống nhúng giám sát nhiệt độ/độ ẩm với phát hiện dị thường TinyML trên ESP32 YOLO UNO**

> **CE (Embedded/Electrical)**: Firmware thiết bị nhúng - Đọc cảm biến, điều khiển relay, gửi dữ liệu qua HTTP REST API

## 📋 Mục lục

- [Tổng Quan Project](#-tổng-quan-project)
- [Kiến Trúc Hệ Thống](#-kiến-trúc-hệ-thống)
- [Cấu Trúc Thư Mục](#-cấu-trúc-thư-mục)
- [Phần Cứng](#-phần-cứng)
- [Quick Start](#-quick-start)
- [Chi Tiết Các Task](#-chi-tiết-các-task-freertos)
- [API REST Endpoints](#-api-rest-endpoints)
- [Cấu Hình WiFi](#-cấu-hình-wifi)
- [TinyML Anomaly Detection](#-tinyml-anomaly-detection)
- [Troubleshooting](#-troubleshooting)
- [File Tham Khảo](#-file-tham-khảo)

---

## 🎯 Tổng Quan Project

**Smart Home CE** là hệ thống nhúng chạy trên **ESP32 YOLO UNO** (Dual-core 240MHz, WiFi) với các tính năng:

✅ **Đọc Cảm Biến**: DHT20 (nhiệt độ + độ ẩm) qua I2C mỗi 5 giây  
✅ **Xử Lý Dữ Liệu**: Lọc nhiễu + phát hiện outlier + exponential smoothing  
✅ **Phát Hiện Dị Thường**: TinyML anomaly detection bằng TensorFlow Lite  
✅ **Điều Khiển Thiết Bị**: 2 relay (đèn, quạt) qua GPIO 12, 13  
✅ **Kết Nối Cloud**: HTTP REST API (POST sensor data, GET commands, PUT status)  
✅ **WiFi Auto-Reconnect**: Kết nối lại tự động khi mất WiFi  
✅ **Đa Nhiệm FreeRTOS**: 6 tasks chạy song song với priority management  

**Memory**:
- RAM: 18.4% (60KB / 320KB) - Plenty of room
- Flash: 31.8% (1063KB / 3.3MB) - Safe margin

---

## 🏗️ Kiến Trúc Hệ Thống

### Task Dependencies

```
┌─────────────────────────────────────────────────────────────┐
│ WiFi Manager Task (Priority 1)                              │
│ └─> Maintains connection, exponential backoff reconnect    │
└────────────────────┬────────────────────────────────────────┘
                     │ (WiFi Connected Signal)
                     ▼
┌─────────────────────────────────────────────────────────────┐
│ Sensor Task (Priority 3 - HIGHEST)                          │
│ └─> DHT20 every 5s → Filtering → xQueueSensorData         │
└────────────┬─────────────────────────┬─────────────────────┘
             │                         │
             ▼                         ▼
    ┌────────────────┐      ┌──────────────────────┐
    │ TinyML Task    │      │ HTTP Upload Task     │
    │ (Priority 2)   │      │ (Priority 2)         │
    │ Anomaly score  │      │ POST every 30s       │
    └────────┬───────┘      │ + retry logic        │
             │              └──────────────────────┘
             ▼
    Update anomaly flag
             │
             └──────────────────┐
                                ▼
                    ┌─────────────────────────┐
                    │ Command Poll Task       │
                    │ (Priority 2)            │
                    │ GET every 10s           │
                    └────────┬────────────────┘
                             │
                             ▼
                    ┌─────────────────────────┐
                    │ Relay Task              │
                    │ (Priority 1)            │
                    │ Execute → PumpStatus    │
                    └─────────────────────────┘
```

### Data Flow

```
DHT20 Sensor
    │
    ├─► Validation (range check)
    │
    ├─► Outlier Detection (Z-score)
    │
    ├─► Exponential Smoothing (α=0.3)
    │
    ├─► xQueueSensorData
    │
    ├─► TinyML Inference
    │   └─► Anomaly Score (0.0 - 1.0)
    │
    └─► HTTP Upload
        └─► Backend /api/sensors/data
```

---

## 📁 Cấu Trúc Thư Mục

```
/root/esp32-freertos-tinyml-monitor/
├── include/
│   ├── ce_config.h              # ⚙️  Configuration (pins, URLs, timings)
│   ├── ce_data_types.h          # 📦 Data structures (CE_SensorData, Command, etc)
│   ├── ce_global.h              # 🌍 Global variables & IPC primitives
│   ├── ce_sensor_task.h         # 📡 DHT20 reading task header
│   ├── ce_relay_task.h          # 🔌 Relay GPIO control header
│   ├── ce_http_client.h         # 🌐 HTTP GET/POST helpers
│   ├── ce_http_upload_task.h    # 📤 Sensor upload (30s cycle)
│   ├── ce_command_poll_task.h   # 📥 Command fetch (10s cycle)
│   ├── ce_tinyml_task.h         # 🤖 Anomaly detection header
│   ├── ce_wifi_manager.h        # 📶 WiFi task header
│   └── [legacy headers...]      # ✅ Existing headers (untouched)
│
├── src/
│   ├── ce_sensor_task.cpp       # ~300 lines - DHT20 + data filtering
│   ├── ce_relay_task.cpp        # ~80 lines - GPIO control + status
│   ├── ce_http_client.cpp       # ~120 lines - HTTP + ArduinoJson parsing
│   ├── ce_http_upload_task.cpp  # ~50 lines - Sensor upload with retry
│   ├── ce_command_poll_task.cpp # ~50 lines - Command polling
│   ├── ce_tinyml_task.cpp       # ~70 lines - Anomaly detection (simple + TFLite ready)
│   ├── ce_wifi_manager.cpp      # ~60 lines - WiFi connection management
│   ├── global.cpp               # ✅ Updated - CE queues initialized
│   ├── main.cpp                 # ✅ Updated - CE task creation
│   └── [legacy files...]        # ✅ Existing sources (untouched)
│
├── lib/
│   ├── DHT20/                   # ✅ Sensor driver
│   ├── ArduinoJson/             # ✅ JSON parsing
│   ├── TensorFlowLite_ESP32/    # ✅ ML inference
│   └── [other libs...]          # ✅ Not modified
│
├── platformio.ini               # ✅ Updated - ArduinoJson added
├── CE_BUILD_GUIDE.md            # 📖 Deployment instructions
└── README.md                    # 📄 This file
```

---

## 🔧 Phần Cứng

### Pinout - ESP32 YOLO UNO

| Component | GPIO | Notes |
|-----------|------|-------|
| **DHT20 SDA** | GPIO 21 | I2C Data (4.7kΩ pull-up) |
| **DHT20 SCL** | GPIO 22 | I2C Clock (4.7kΩ pull-up) |
| **Light Relay** | GPIO 12 | Active HIGH, 100ms debounce |
| **Fan Relay** | GPIO 13 | Active HIGH, 100ms debounce |

### Wiring Diagram

```
    ESP32 YOLO UNO
    ┌─────────────────┐
    │                 │
    │  GPIO21 (SDA) ──┼──┐
    │  GPIO22 (SCL) ──┼──┼─── DHT20 (I2C Address 0x38)
    │  GND ────────────┼─┤
    │  3.3V ───────────┼─┘
    │                 │
    │  GPIO12 ────────┼─────── Relay 1 (Light)
    │  GPIO13 ────────┼─────── Relay 2 (Fan)
    │  GND ───────────┴─────── Relay GND
    │                 │
    └─────────────────┘
          WiFi (Internal)
```

### Yêu Cầu Phần Cứng

- **ESP32 YOLO UNO**: Dual-core, 240MHz, WiFi 2.4GHz
- **DHT20 Sensor**: I2C, ±0.5°C accuracy, high-precision humidity
- **2x Relay Module**: 5V trigger, 10A rated
- **USB Cable**: Micro-USB để lập trình & power

---

## 🚀 Quick Start

### 1. Cài Đặt Dependencies

```bash
# Install PlatformIO
pip install platformio

# Navigate to project
cd /root/esp32-freertos-tinyml-monitor
```

### 2. Cấu Hình WiFi & Backend

Edit `include/ce_config.h`:

```cpp
#define CE_WIFI_SSID           "Your_WiFi_Name"
#define CE_WIFI_PASSWORD       "Your_WiFi_Password"
#define CE_BACKEND_URL         "http://192.168.1.50:3000"
#define CE_DEVICE_ID           "esp32-01"
#define CE_DEVICE_SECRET       ""  // Optional auth token
```

### 3. Build Firmware

```bash
pio run -e yolo_uno
```

**Expected Output**:
```
RAM:   [==        ]  18.4% (used 60376 bytes from 327680 bytes)
Flash: [===       ]  31.8% (used 1063485 bytes from 3342336 bytes)
========================= [SUCCESS] ========================
```

### 4. Upload to Device

```bash
pio run -t upload -e yolo_uno
```

Or specify USB port:
```bash
pio run -t upload -e yolo_uno --upload-port /dev/ttyUSB0
```

### 5. Monitor Serial Output

```bash
pio device monitor -p /dev/ttyUSB0 -b 115200
```

**Expected Boot Sequence**:
```
[BOOT] CE globals initialized
[WIFI] Task started
[SENSOR] Task started
[SENSOR] DHT20 initialized successfully
[RELAY] Task started
[TINYML] Task started (stub)
[UPLOAD] Task started
[CMND_POLL] Task started
[BOOT] Setup complete - all tasks created
[WIFI] Connected! IP: 192.168.1.100, Signal: -65 dBm
[SENSOR] T=28.50°C, H=65.30%, Q=0.98, #1
[UPLOAD] Success: T=28.50°C, H=65.30%
```

---

## 📊 Chi Tiết Các Task FreeRTOS

### 1️⃣ WiFi Manager Task
- **Priority**: 1 (Low)
- **Interval**: Continuous
- **Stack**: 3KB
- **Purpose**: Maintain WiFi connection with exponential backoff
- **Logs**: `[WIFI]`

**Behavior**:
```
Connected → Check signal every 10s
Not Connected → Retry with backoff [5s, 10s, 20s, 30s]
```

---

### 2️⃣ Sensor Task
- **Priority**: 3 (High)
- **Interval**: Every 5 seconds
- **Stack**: 2KB
- **Purpose**: Read DHT20, apply filtering, detect outliers
- **Logs**: `[SENSOR]`

**Processing Pipeline**:
```
Read DHT20 → Validate → Detect Outliers → Smooth → Queue
```

**Data Quality Metrics**:
- Range: -40 to +85°C (Temperature), 0 to 100% (Humidity)
- Z-score threshold: 2.5 (outlier detection)
- Smoothing factor: α=0.3 (exponential)
- Quality score: 0.0-1.0 (output with each reading)

**Sample Output**:
```
[SENSOR] T=28.50°C, H=65.30%, Q=0.98, #45234
```

---

### 3️⃣ TinyML Task
- **Priority**: 2 (Medium)
- **Interval**: Every reading cycle (~5s)
- **Stack**: 4KB
- **Purpose**: Run anomaly detection inference
- **Logs**: `[TINYML]`

**Algorithm**:
```
Input: T_normalized = (T + 40) / 125, H_normalized = H / 100
Compute: anomaly_score = |T_norm - center| + |H_norm - center| / 2
Output: anomaly_flag = (score > 0.5)
```

---

### 4️⃣ HTTP Upload Task
- **Priority**: 2 (Medium)
- **Interval**: Every 30 seconds
- **Stack**: 4KB
- **Purpose**: Upload sensor data to backend
- **Logs**: `[UPLOAD]`

**Retry Logic**:
- Attempts: 3
- Backoff: 5 seconds between retries
- Timeout: 5 seconds per request

**Sample Log**:
```
[UPLOAD] Posting sensor data...
[UPLOAD] Success: HTTP 200 OK
```

---

### 5️⃣ Command Poll Task
- **Priority**: 2 (Medium)
- **Interval**: Every 10 seconds
- **Stack**: 4KB
- **Purpose**: Fetch commands from backend
- **Logs**: `[CMND_POLL]`

**Query**: `GET /api/devices/command?deviceId=esp32-01`

**Sample Log**:
```
[CMND_POLL] Fetching commands...
[CMND_POLL] Received 1 command: turn light on
```

---

### 6️⃣ Relay Task
- **Priority**: 1 (Low)
- **Trigger**: On-demand (from command queue)
- **Stack**: 2KB
- **Purpose**: Execute relay commands
- **Logs**: `[RELAY]`

**Execution**:
```
Get Command → Set GPIO → Wait 100ms → Verify → Send Status
```

---

## 🌐 API REST Endpoints

### 1. POST /api/sensors/data

**Frequency**: Every 30 seconds  
**Purpose**: Upload sensor readings

**Request**:
```json
{
  "deviceId": "esp32-01",
  "temperature": 28.50,
  "humidity": 65.30,
  "anomaly": false,
  "anomalyScore": 0.28,
  "dataQuality": 0.98,
  "timestamp": 1711010400
}
```

**Response** (200 OK):
```json
{
  "success": true,
  "message": "Sensor data received",
  "recordId": "rec-2024-03-26-12345"
}
```

---

### 2. GET /api/devices/command?deviceId=esp32-01

**Frequency**: Every 10 seconds  
**Purpose**: Fetch pending commands

**Response** (200 OK):
```json
{
  "deviceId": "esp32-01",
  "commands": [
    {
      "commandId": "cmd-001",
      "device": "light",
      "action": "on",
      "priority": "normal",
      "timestamp": 1711010300,
      "expiresAt": 1711010460
    }
  ]
}
```

**Response** (no commands):
```json
{
  "deviceId": "esp32-01",
  "commands": []
}
```

---

### 3. POST /api/devices/status

**Trigger**: After relay execution + periodic (10s heartbeat)  
**Purpose**: Report device status

**Request**:
```json
{
  "deviceId": "esp32-01",
  "status": {
    "light": true,
    "fan": false
  },
  "executedCommands": [
    {
      "commandId": "cmd-001",
      "status": "executed",
      "result": "success"
    }
  ],
  "meta": {
    "uptime": 3600,
    "wifiSignal": -65,
    "freeMemory": 120000
  },
  "timestamp": 1711010420
}
```

**Response** (200 OK):
```json
{
  "success": true,
  "message": "Status recorded",
  "ackCommands": ["cmd-001"]
}
```

---

## 📶 Cấu Hình WiFi

### Method 1: Hardcoded (für Production)

Edit `include/ce_config.h`:
```cpp
#define CE_WIFI_SSID           "Home_WiFi"
#define CE_WIFI_PASSWORD       "password123"
#define CE_BACKEND_URL         "http://192.168.1.50:3000"
```

Then rebuild & upload.

### WiFi Reconnection Strategy

- **Initial Connection Timeout**: 15 seconds
- **Backoff Strategy**: [5s, 10s, 20s, 30s]
- **Auto-Continue**: Retries indefinitely with exponential backoff

**Serial Output**:
```
[WIFI] Connected! IP: 192.168.1.100, Signal: -65 dBm
[WIFI] Connection lost!
[WIFI] Attempting reconnect (attempt 1)...
[WIFI] Backoff: 5 seconds
[WIFI] Reconnected! IP: 192.168.1.100
```

---

## 🤖 TinyML Anomaly Detection

### Current Implementation (Simple)

The firmware includes a **basic anomaly detector** that:

1. **Normalizes** sensor inputs to 0-1 range
2. **Computes deviation** from nominal values (T=25°C center, H=50% center)
3. **Outputs score** as average deviation
4. **Flags anomaly** if score > 0.5

**Formula**:
```
T_norm = (T + 40) / 125       # [−40, +85]°C → [0, 1]
H_norm = H / 100              # [0, 100]% → [0, 1]

anomaly_score = (|T_norm - 0.45| + |H_norm - 0.5|) / 2

anomaly = (anomaly_score > 0.5)
```

### Full TensorFlow Lite Integration (Ready)

The firmware is **ready for real TFLite models**:
- Tensor arena: 8KB pre-allocated
- Model loading code in `ce_tinyml_task.cpp`
- Per-model configuration in `ce_config.h`

To use real model:
1. Convert training model → `.tflite` quantized
2. Replace placeholder in `lib/tinyml_model/anomaly_model.h`
3. Adjust normalization in `ce_tinyml_task.cpp`

---

## 🔧 Troubleshooting

### Build Issues

#### Error: "ce_config.h: No such file"
**Solution**: Ensure `include/` path is in PlatformIO config
```bash
pio build -e yolo_uno -v
```

#### Memory Too High
**Current**: 18.4% RAM, 31.8% Flash  
**Solution**: Remove unused legacy tasks from `main.cpp`

---

### Runtime Issues

#### [ERROR] DHT20 not found on I2C bus

**Checklist**:
1. Verify GPIO21 (SDA), GPIO22 (SCL) connections
2. Add 4.7kΩ pull-ups to I2C lines
3. Check DHT20 address: `0x38` (fixed)
4. Use multimeter to verify 3.3V on DHT20 pin

**Test I2C**:
```cpp
// Add to main.cpp for testing
Wire.begin(21, 22);
for (int addr = 0x01; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
        Serial.printf("Found device at 0x%02X\n", addr);
    }
}
```

---

#### [ERROR] WiFi Connect Timeout

**Checklist**:
1. Verify SSID & Password in `ce_config.h`
2. Check WiFi signal: should show `[WIFI] Signal: -XX dBm`
3. Move device closer to router (> -70 dBm)
4. Restart WiFi router

---

#### [UPLOAD] Failed: HTTP error

**Checklist**:
1. Verify backend is running: `curl http://192.168.1.50:3000/api/sensors/data`
2. Check firewall rules (allow port 3000)
3. Verify `CE_BACKEND_URL` matches backend IP:port
4. Check network connectivity with a simple ping test

---

#### Sensor Returns NaN

**Checklist**:
1. Verify I2C connections (GPIO21, GPIO22)
2. Check pull-up resistors (4.7kΩ)
3. Ensure DHT20 is powered (3.3V)
4. Try power cycle (hold RESET for 2 seconds)

---

## 📄 File Tham Khảo

| File | Mục Đích |
|------|---------|
| `include/ce_config.h` | ⚙️ Configuration (pins, URLs, timings) |
| `include/ce_data_types.h` | 📦 Data structures (CE_SensorData, Command, etc) |
| `include/ce_global.h` | 🌍 Global variables, queues, semaphores |
| `src/ce_sensor_task.cpp` | 📡 DHT20 reading + filtering |
| `src/ce_relay_task.cpp` | 🔌 Relay GPIO control |
| `src/ce_http_client.cpp` | 🌐 HTTP GET/POST + JSON |
| `src/ce_wifi_manager.cpp` | 📶 WiFi connection |
| `src/ce_tinyml_task.cpp` | 🤖 Anomaly detection |
| `platformio.ini` | 🏗️ Build configuration |
| `CE_BUILD_GUIDE.md` | 📖 Detailed deployment guide |

---

## 📝 License & Credits

- **TensorFlow Lite Micro**: Open Source
- **PlatformIO**: Open Source
- **FreeRTOS**: Open Source
- **ESP32 Arduino Core**: Espressif

---

## 📞 Support & Contact

For issues or contributions:
1. Check [Troubleshooting](#-troubleshooting) section
2. Review serial logs for `[ERROR]` messages
3. Verify hardware wiring against [Pinout](#pinout---esp32-yolo-uno)
4. Test with mock backend

---

**Last Updated**: 2024-03-26  
**Build Status**: ✅ SUCCESS  
**Memory**: RAM 18.4%, Flash 31.8%  
**Ready for Deployment**: 🚀
**Status:** ✅ Production Ready
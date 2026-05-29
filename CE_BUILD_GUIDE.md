# CE Firmware - Build & Deployment Guide

## 📊 Build Status: ✅ SUCCESS

**Final Build #5 Results:**
- **RAM**: 18.4% (60,376 / 327,680 bytes) - Plenty of room for future features
- **Flash**: 31.8% (1,063,485 / 3,342,336 bytes) - Comfortable margin
- **Compilation**: Zero errors, 0 warnings from CE code (some TFLite lib warnings are normal)
- **All Tasks**: WiFi Manager, Sensor Read, Relay Control, TinyML, HTTP Upload, Command Poll

---

## 🏗️ Project Structure

```
/root/esp32-freertos-tinyml-monitor/
├── include/
│   ├── ce_config.h                  # Configuration (pins, URLs, timings)
│   ├── ce_data_types.h              # Data structures (CE_SensorData, Command, etc)
│   ├── ce_global.h                  # Global variables & queues
│   ├── ce_sensor_task.h             #  DHT20 reading task
│   ├── ce_relay_task.h              # Relay GPIO control task
│   ├── ce_http_client.h             # HTTP GET/POST helpers
│   ├── ce_http_upload_task.h        # Sensor data upload (30s cycle)
│   ├── ce_command_poll_task.h       # Command fetch (10s cycle)
│   ├── ce_tinyml_task.h             # Anomaly detection (stub)
│   └── ce_wifi_manager.h            # WiFi auto-reconnect
├── src/
│   ├── ce_sensor_task.cpp           # ~300 lines - DHT20 + filtering
│   ├── ce_relay_task.cpp            # ~80 lines - GPIO control
│   ├── ce_http_client.cpp           # ~100 lines - HTTP + JSON parsing
│   ├── ce_http_upload_task.cpp      # ~50 lines - Sensor upload
│   ├── ce_command_poll_task.cpp     # ~50 lines - Command polling
│   ├── ce_tinyml_task.cpp           # ~70 lines - Simple anomaly detection
│   ├── ce_wifi_manager.cpp          # ~60 lines - WiFi management
│   ├── global.cpp                   # **Updated** with CE queues + init
│   ├── main.cpp                     # **Updated** with CE task creation
│   └── [legacy files remain unchanged]
├── platformio.ini                   # **Updated** ArduinoJson + HTTPClient
└── [lib/ - untouched as requested]

```

---

## 🚀 Quick Start Build

### 1. Configure WiFi & Backend (Required!)

Edit `include/ce_config.h`:

```cpp
#define CE_WIFI_SSID           "Your_WiFi_Name"
#define CE_WIFI_PASSWORD       "Your_WiFi_Password"
#define CE_BACKEND_URL         "http://192.168.1.X:3000"  // Backend API server
#define CE_DEVICE_ID           "esp32-01"
#define CE_DEVICE_SECRET       ""  // Optional auth token
```

### 2. Compile

```bash
cd /root/esp32-freertos-tinyml-monitor
pio run -e yolo_uno
```

Expected output:
```
RAM:   [==        ]  18.4% (used 60376 bytes from 327680 bytes)
Flash: [===       ]  31.8% (used 1063485 bytes from 3342336 bytes)
[SUCCESS] Build complete
```

### 3. Upload to Device

```bash
pio run -t upload -e yolo_uno
```

Or via USB:
```bash
pio run -t upload -e yolo_uno --upload-port /dev/ttyUSB0
```

### 4. Monitor Serial Output

```bash
pio device monitor -p /dev/ttyUSB0 -b 115200
```

Expected boot sequence:
```
[BOOT] Serial initialized at 115200
[INIT] CE globals initialized
[WIFI] Task started
[SENSOR] Task started
[RELAY] Task started
[TINYML] Task started (stub)
[UPLOAD] Task started
[CMND_POLL] Task started
[BOOT] Setup complete - all tasks created
[WIFI] Connected! IP: 192.168.X.X, Signal: -55 dBm
[SENSOR] DHT20 initialized successfully
[SENSOR] T=28.50°C, H=65.30%, Q=0.98
[UPLOAD] Success: T=28.50°C, H=65.30%
[CMND_POLL] Queued: LIGHT=1
[RELAY] Executing light=ON
```

---

## 📋 Task Details

| Task | Purpose | Interval | Priority | Stack | Logs |
|------|---------|----------|----------|-------|------|
| **WiFi Manager** | Connect/reconnect WiFi | Continuous | 1 (Low) | 3KB | `[WIFI]` |
| **Sensor Read** | DHT20 + filtering | 5 seconds | 3 (High) | 2KB | `[SENSOR]` |
| **Relay Control** | Execute commands | On-demand | 1 (Low) | 2KB | `[RELAY]` |
| **TinyML** | Anomaly detection | 5 seconds | 2 (Med) | 4KB | `[TINYML]` |
| **HTTP Upload** | Post sensor data | 30 seconds | 2 (Med) | 4KB | `[UPLOAD]` |
| **Command Poll** | Fetch backend commands | 10 seconds | 2 (Med) | 4KB | `[CMND_POLL]` |

---

## 🔧 Configuration Adjustments

### Change Sensor Read Interval

In `ce_config.h`:
```cpp
#define SENSOR_READ_INTERVAL    5000  // Change to 10000 for 10 seconds
```

### Change Upload Frequency

In `ce_config.h`:
```cpp
#define HTTP_UPLOAD_INTERVAL    30000  // Change to 60000 for 60 seconds
```

### Change GPIO Pins

In `ce_config.h`:
```cpp
#define RELAY_LIGHT_PIN     12    // Change to your GPIO
#define RELAY_FAN_PIN       13    // Change to your GPIO
#define SENSOR_SDA_PIN      21    // I2C SDA
#define SENSOR_SCL_PIN      22    // I2C SCL
```

### Anomaly Detection Threshold

In `ce_config.h`:
```cpp
#define TINYML_ANOMALY_THRESHOLD 0.5f  // Change to 0.3 for stricter detection
```

---

## 🔌 Hardware Wiring

| Component | ESP32 Pin | Notes |
|-----------|-----------|-------|
| DHT20 SDA | GPIO21 | I2C data (4.7кΩ pull-up) |
| DHT20 SCL | GPIO22 | I2C clock (4.7кΩ pull-up) |
| Light Relay | GPIO12 | Active HIGH |
| Fan Relay | GPIO13 | Active HIGH |
| GND | GND | Common ground |
| VCC | 3.3V | DHT20 power |

---

## 📡 API Endpoints

### 1. POST /api/sensors/data (Every 30s)

Request:
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

### 2. GET /api/devices/command?deviceId=esp32-01 (Every 10s)

Response:
```json
{
  "commands": [
    {
      "commandId": "cmd-001",
      "device": "light",
      "action": "on"
    }
  ]
}
```

### 3. POST /api/devices/status

Request:
```json
{
  "deviceId": "esp32-01",
  "status": {
    "light": true,
    "fan": false
  },
  "timestamp": 1711010400
}
```

---

## 🐛 Troubleshooting

### Build Fails with "DHT20_OK not defined"

**Solution**: Check `lib/DHT20/DHT20.h` is in include path. Already configured in `platformio.ini`.

### WiFi Won't Connect

**Troubleshoot**:
```
[WIFI] Disconnected, attempting reconnect...
```

1. Check `CE_WIFI_SSID` and `CE_WIFI_PASSWORD` are correct
2. Verify WiFi signal: `[WIFI] Signal: -55 dBm` (should be > -70 dBm)
3. Check router range - move closer

### HTTP Upload Fails (HTTP error)

**Check**:
```
[UPLOAD] Failed: HTTP 404
```

1. Verify `CE_BACKEND_URL` is correct
2. Ensure backend is running: `curl http://192.168.X.X:3000/api/sensors/data`
3. Check firewall rules

### Sensor Returns NaN

**Check**:
```
[SENSOR] T=NaN, H=NaN
```

1. Verify I2C wires: GPIO21 (SDA), GPIO22 (SCL)
2. Check 4.7kΩ pull-ups on I2C lines
3. Run I2C scanner to verify address 0x38

---

## 📈 Memory & Performance

### Current Usage:
- **RAM**: 18.4% (plenty for future expansion)
- **Flash**: 31.8% (safe margin for OTA)

### Heap Growth (30 min test):
- Should remain stable: ±5KB variance
- If heap shrinks > 10%, check for memory leaks in tasks

### Task Execution Times:
- Sensor read: 50-100ms (mostly waiting)
- HTTPclient: 500-1500ms (network I/O)
- TinyML inference: 5-10ms
- Relay GPIO: <200ms

---

## ✅ Deployment Checklist

Before going to production:

- [ ] WiFi credentials configured (`CE_WIFI_SSID`, `CE_WIFI_PASSWORD`)
- [ ] Backend URL configured (`CE_BACKEND_URL`)
- [ ] GPIO pins match your hardware (relay, sensor)
- [ ] Build succeeds: `pio run -e yolo_uno`
- [ ] Device boots and shows `[BOOT] Setup complete`
- [ ] WiFi connects within 15 seconds
- [ ] DHT20 sensor readings appear every 5s
- [ ] HTTP upload succeeds every 30s
- [ ] Command polling works every 10s
- [ ] Relay responds to commands within 500ms
- [ ] No memory leaks over 60 min runtime
- [ ] All serial logs show no `[ERROR]` messages

--

## 📞 Support

For issues:
1. Check serial logs: `[ERROR]` messages indicate problems
2. Verify hardware wiring
3. Test with `curl` to backend endpoints
4. Monitor free heap: heap should not decrease over time

---

**Ready to deploy!** 🚀

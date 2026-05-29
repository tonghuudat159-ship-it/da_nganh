#ifndef CE_CONFIG_GLOBAL_H
#define CE_CONFIG_GLOBAL_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "ce_data_types.h"

/*============================================================
  SMART HOME CE FIRMWARE - UNIFIED CONFIG & GLOBALS
  Platform: ESP32 YOLO UNO
  Author: CE Team (Khang + Dat)

  ============================================================*/

// ============================================================
// HARDWARE PINS
// ============================================================
// LIGHT_PIN is analog input from the light sensor AO/SIG.
#define LIGHT_PIN             10
// PIR_PIN is digital input from the PIR motion sensor.
#define PIR_PIN               38
// GPIO48 is D13 on this board.
// D13/GPIO48 is reserved for the auto-light LED.
// Fan must not use GPIO48.
// GPIO12 is I2C SCL and must not be used as relay/LED output.
#define RELAY_LIGHT_PIN       48
// Fan relay uses D12/GPIO47.
#define RELAY_FAN_PIN         47

// ============================================================
// RELAY CONFIGURATION
// ============================================================
#define RELAY_ON_STATE      HIGH   // Active HIGH
#define RELAY_OFF_STATE     LOW
#define PIR_ACTIVE_STATE          HIGH
#define PIR_WARMUP_TIME_MS        30000UL
#define PIR_STABLE_READS_REQUIRED 3
#define PIR_SAMPLE_DELAY_MS       50
#define FAN_TEMP_THRESHOLD    27.0f
#define LIGHT_THRESHOLD       1500

#if RELAY_FAN_PIN == RELAY_LIGHT_PIN
#error "RELAY_FAN_PIN must not be the same as RELAY_LIGHT_PIN"
#endif

// ============================================================
// FREERTOS TASK CONFIGURATION
// ============================================================
#define WIFI_TASK_STACK     3072
#define WIFI_TASK_PRIO      1

#define RELAY_TASK_STACK    2048
#define RELAY_TASK_PRIO     1

#define AUTO_RELAY_TASK_STACK   2048
#define AUTO_RELAY_TASK_PRIO    2

#define HTTP_UPLOAD_TASK_STACK  4096
#define HTTP_UPLOAD_TASK_PRIO   2
#define HTTP_UPLOAD_INTERVAL    30000 // ms (30 seconds)

#define HTTP_COMMAND_TASK_STACK 4096
#define HTTP_COMMAND_TASK_PRIO  2
#define HTTP_COMMAND_INTERVAL   10000 // ms (10 seconds)

#define TINYML_TASK_STACK   4096
#define TINYML_TASK_PRIO    2

#define SENSOR_TASK_STACK   2048
#define SENSOR_TASK_PRIO    3     // High priority
#define SENSOR_READ_INTERVAL 5000 // ms (5 seconds)

// ============================================================
// QUEUE SIZES
// ============================================================
#define QUEUE_SENSOR_SIZE   16
#define QUEUE_RELAY_SIZE    8
#define QUEUE_STATUS_SIZE   4

// ============================================================
// WIFI CONFIGURATION
// ============================================================
#define CE_WIFI_SSID           "Your_WiFi_SSID"
#define CE_WIFI_PASSWORD       "Your_Password"
#define CE_WIFI_RECONNECT_INTERVALS {5000, 10000, 20000, 30000} // Exponential backoff

// ============================================================
// BACKEND API CONFIGURATION
// ============================================================
#define CE_BACKEND_URL         "http://192.168.1.50:5000"
#define CE_DEVICE_ID           "esp32-01"
#define CE_DEVICE_SECRET       "my_esp32_secret_123"

// API Endpoints
#define API_SENSORS_DATA    "/api/sensors/data"
#define API_DEVICES_COMMAND "/api/devices/command"

// ============================================================
// SERIAL DEBUG
// ============================================================
#define SERIAL_BAUD         115200
#define HEALTH_CHECK_INTERVAL 60000 // Print health metrics every 60s

// ============================================================
// CE QUEUES FOR INTER-TASK COMMUNICATION
// ============================================================

// Sensor readings: SensorTask -> TinyMLTask, HttpUploadTask
extern QueueHandle_t xQueueSensorData;

// Relay commands: CommandPollTask -> RelayTask
extern QueueHandle_t xQueueCommand;

// Device status: RelayTask -> HttpUploadTask
extern QueueHandle_t xQueueStatus;

// ============================================================
// CE SEMAPHORES & MUTEXES
// ============================================================

// HTTP Mutex: Ensure only one HTTP request at a time
extern SemaphoreHandle_t xSemaphoreHTTP;

// WiFi connection binary semaphore
extern SemaphoreHandle_t xSemaphoreWiFi;

// ============================================================
// CE GLOBAL STATE VARIABLES
// ============================================================

// WiFi state
extern volatile bool g_wifiConnected;
extern volatile int8_t g_wifiSignal;     // RSSI (dBm)

// Device state
extern volatile bool g_lightOn;
extern volatile bool g_fanOn;

// Sensor state
extern volatile float g_lastTemperature;
extern volatile float g_lastHumidity;
extern volatile float g_lastAnomalyScore;

// System state
extern volatile uint32_t g_systemUptime; // seconds
extern volatile uint32_t g_freeHeap;    // bytes

// ============================================================
// LEGACY GLOBALS (from old project) - KEEP FOR COMPATIBILITY
// ============================================================

// Legacy queues (old project)
extern QueueHandle_t xQueueForTinyML;
extern QueueHandle_t xQueueTempHumiForMain;
extern QueueHandle_t xQueueforAuto;

// Legacy semaphores (old project)
extern SemaphoreHandle_t xSemaphoreMutex;

// ============================================================
// LEGACY DATA TYPES (for backward compatibility)
// ============================================================

// manual mode or auto mode

extern bool IsLight_Auto;
extern bool IsFan_Auto;

typedef struct {
    float temperature;
    float humidity;
    float light;
    // PIR detects motion. human_inside is true only after PIR warmup
    // and only when the input is stably active across repeated samples.
    bool human_inside;
} SensorData;

typedef struct 
{
    float temperature;
    float humidity;
    float inference_result;
    bool anomaly_detected;
    String anomaly_type;
} MLResult;

// ============================================================
// INITIALIZATION FUNCTIONS
// ============================================================

/**
 * @brief Initialize all CE global queues and semaphores
 * Called from setup() before task creation
 */
void ce_globals_init(void);

#endif // CE_CONFIG_GLOBAL_H

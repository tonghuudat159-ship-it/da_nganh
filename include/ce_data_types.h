#ifndef CE_DATA_TYPES_H
#define CE_DATA_TYPES_H

#include <stdint.h>
#include <stdbool.h>




// Filtered sensor reading with TinyML results
typedef struct {
    float temperature;
    float humidity;
    float anomalyScore;     // 0.0 - 1.0
    bool anomaly;           // true if anomalyScore > threshold
    float dataQuality;      // 0.0 - 1.0 (confidence metric)
    uint32_t timestamp;
    bool valid;
} CE_SensorData;

// ============================================================
// RELAY COMMANDS
// ============================================================

typedef enum {
    RELAY_ACTION_ON = 1,
    RELAY_ACTION_OFF = 0,
    RELAY_ACTION_TOGGLE = 2
} RelayAction;

typedef enum {
    RELAY_DEVICE_LIGHT = 0,
    RELAY_DEVICE_FAN = 1
} RelayDevice;

typedef struct {
    char commandId[32];     // Backend command ID
    RelayDevice device;     // light or fan
    RelayAction action;     // on/off/toggle
    uint8_t priority;       // 0-255 (higher = more priority)
    uint32_t timestamp;
    uint32_t expiresAt;
} Command;

// ============================================================
// DEVICE STATUS
// ============================================================

typedef struct {
    bool lightOn;
    bool fanOn;
    char lastCommandId[32];
    uint32_t lastCommandTime;
    bool executionSuccess;
    uint32_t timestamp;
} DeviceStatus;

typedef struct {
    DeviceStatus status;
    Command executedCommand;
    uint32_t executionTime;  // ms
} DeviceStatusReport;

// ============================================================
// HTTP RESPONSE DATA
// ============================================================

typedef struct {
    int httpCode;
    char buffer[1024];      // Response body
    uint16_t bufferLen;
    uint32_t timestamp;
} HTTPResponse;

typedef struct {
    bool success;
    char message[256];
    uint32_t timestamp;
} APIResponse;

// ============================================================
// WIFI STATUS
// ============================================================

typedef enum {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_FAILED
} WiFiState;

typedef struct {
    WiFiState state;
    char ssid[32];
    int8_t rssi;            // Signal strength (dBm)
    uint32_t uptime;        // Connected duration (ms)
    uint32_t disconnects;   // Number of disconnections
} WiFiStatus;

// ============================================================
// DEVICE METADATA
// ============================================================

typedef struct {
    char deviceId[32];
    char fwVersion[16];
    uint32_t uptime;        // System uptime (seconds)
    uint32_t freeHeap;      // Free memory (bytes)
    int8_t wifiSignal;      // WiFi RSSI (dBm)
    uint8_t cpuTemp;        // CPU temperature (°C, if available)
    uint32_t timestamp;
} DeviceMetadata;





typedef enum {
    CE_ERR_OK = 0,
    
    // Sensor errors
    CE_ERR_SENSOR_I2C = 101,
    CE_ERR_SENSOR_CRC = 102,
    CE_ERR_SENSOR_TIMEOUT = 103,
    CE_ERR_SENSOR_OUT_OF_RANGE = 104,
    
    // WiFi errors
    CE_ERR_WIFI_CONNECT = 201,
    CE_ERR_WIFI_TIMEOUT = 202,
    CE_ERR_WIFI_NO_SSID = 203,
    
    // HTTP errors
    CE_ERR_HTTP_SOCKET = 301,
    CE_ERR_HTTP_TIMEOUT = 302,
    CE_ERR_HTTP_PARSE = 303,
    CE_ERR_HTTP_INVALID_RESPONSE = 304,
    
    // TinyML errors
    CE_ERR_TINYML_MODEL = 401,
    CE_ERR_TINYML_INFERENCE = 402,
    CE_ERR_TINYML_NO_MEMORY = 403,
    
    // Relay errors
    CE_ERR_RELAY_GPIO = 501,
    CE_ERR_RELAY_FEEDBACK = 502,
    
    // Memory errors
    CE_ERR_QUEUE_FULL = 601,
    CE_ERR_OUT_OF_MEMORY = 602,
    
    // Generic errors
    CE_ERR_UNKNOWN = 999
} CE_ErrorCode;

#endif // CE_DATA_TYPES_H

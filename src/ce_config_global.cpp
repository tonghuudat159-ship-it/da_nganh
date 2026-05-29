#include "ce_config_global.h"
#include "ce_data_types.h"

String ssid = "ESP32-YOUR NETWORK HERE!!!";
String password = "12345678";
String wifi_ssid = "Hudahadu";
String wifi_password = "khotinhv";
boolean isWifiConnected = false;


SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();
SemaphoreHandle_t xSemaphoreMutex = xSemaphoreCreateMutex();

QueueHandle_t xQueueForLedBlink = xQueueCreate(5, sizeof(SensorData));
QueueHandle_t xQueueForNeoPixel = xQueueCreate(5, sizeof(SensorData));
QueueHandle_t xQueueForTinyML   = xQueueCreate(5, sizeof(SensorData));
QueueHandle_t xQueueForMainServer = xQueueCreate(5, sizeof(MLResult));
QueueHandle_t xQueueTempHumiForMain = xQueueCreate(5, sizeof(SensorData));
QueueHandle_t xQueueLatestState = xQueueCreate(1, sizeof(MLResult));
QueueHandle_t xQueueForIoT = xQueueCreate(5, sizeof(SensorData));

// Unified config & global file + legacy support

    

// CE Queues
QueueHandle_t xQueueSensorData = NULL;
QueueHandle_t xQueueCommand = NULL;
QueueHandle_t xQueueStatus = NULL;

// CE Semaphores
SemaphoreHandle_t xSemaphoreHTTP = NULL;
SemaphoreHandle_t xSemaphoreWiFi = NULL;

// CE Global State
volatile bool g_wifiConnected = false;
volatile int8_t g_wifiSignal = -100;
volatile uint32_t g_wifiUptime = 0;

volatile bool g_lightOn = false;
volatile bool g_fanOn = false;
volatile char g_lastCommandId[32] = {0};

// Auto relay mode flags
bool IsLight_Auto = true;   // Default: Light auto mode enabled
bool IsFan_Auto = true;     // Default: Fan auto mode enabled

volatile float g_lastTemperature = 0.0f;
volatile float g_lastHumidity = 0.0f;
volatile bool g_sensorValid = false;
volatile float g_lastAnomalyScore = 0.0f;

volatile uint32_t g_systemUptime = 0;
volatile uint32_t g_freeHeap = 0;
volatile uint8_t g_errorCount = 0;

/**
 * @brief Initialize CE queues and semaphores
 */
void ce_globals_init(void) {
    // Create queues
    xQueueSensorData = xQueueCreate(QUEUE_SENSOR_SIZE, sizeof(CE_SensorData));
    xQueueCommand = xQueueCreate(QUEUE_RELAY_SIZE, sizeof(Command));
    xQueueStatus = xQueueCreate(QUEUE_STATUS_SIZE, sizeof(DeviceStatusReport));

    // Create semaphores/mutexes
    xSemaphoreHTTP = xSemaphoreCreateMutex();
    xSemaphoreWiFi = xSemaphoreCreateBinary();

    // Validate initialization
    if (xQueueSensorData == NULL || xQueueCommand == NULL || xQueueStatus == NULL) {
        Serial.println("[ERROR] Failed to create CE queues!");
        while (1);
    }
    if (xSemaphoreHTTP == NULL || xSemaphoreWiFi == NULL) {
        Serial.println("[ERROR] Failed to create CE semaphores!");
        while (1);
    }

    Serial.println("[INIT] CE globals initialized successfully");
}

/**
 * @brief Print health metrics every CHECK_INTERVAL seconds
 */
void ce_print_health_metrics(void) {
    static uint32_t lastPrint = 0;
    uint32_t now = millis();
    
    if ((now - lastPrint) >= HEALTH_CHECK_INTERVAL) {
        Serial.printf("[HEALTH] Uptime: %lu s, FreeHeap: %lu B, WiFi: %s (%d dBm), "
                      "Temp: %.1f°C, Humidity: %.1f%%, Light: %d, Fan: %d\n",
                      g_systemUptime, g_freeHeap, 
                      g_wifiConnected ? "OK" : "DOWN", g_wifiSignal,
                      g_lastTemperature, g_lastHumidity,
                      g_lightOn, g_fanOn);
        lastPrint = now;
    }
}


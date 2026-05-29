#include "ce_http_upload_task.h"
#include "ce_config_global.h"
#include "ce_http_client.h"
#include <ArduinoJson.h>

void http_upload_task(void *parameter) {
    Serial.println("[UPLOAD] Task started");
    TickType_t last_wake = xTaskGetTickCount();
    
    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(HTTP_UPLOAD_INTERVAL));
        
        if (!g_wifiConnected) continue;
        
        // Build JSON payload
        DynamicJsonDocument doc(512);
        doc["deviceId"] = CE_DEVICE_ID;
        doc["temperature"] = g_lastTemperature;
        doc["humidity"] = g_lastHumidity;
        
        String json_str;
        serializeJson(doc, json_str);
        
        // Take HTTP semaphore
        if (xSemaphoreTake(xSemaphoreHTTP, pdMS_TO_TICKS(5000))) {
            char response[256] = {0};
            int code = http_post(API_SENSORS_DATA, json_str.c_str(), response, sizeof(response));
            xSemaphoreGive(xSemaphoreHTTP);
            
            if (code == 200) {
                Serial.printf("[UPLOAD] Success: T=%.1f°C, H=%.1f%%\n", g_lastTemperature, g_lastHumidity);
            } else {
                Serial.printf("[UPLOAD] Failed: HTTP %d\n", code);
            }
        }
    }
}

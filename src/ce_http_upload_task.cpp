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
        doc["anomalyScore"] = g_lastAnomalyScore;
        doc["dataQuality"] = 1.0;

        String json_str;
        serializeJson(doc, json_str);

        Serial.printf("[UPLOAD] Target base: %s\n", CE_BACKEND_URL);
        Serial.printf("[UPLOAD] Payload: %s\n", json_str.c_str());

        // Take HTTP semaphore
        if (xSemaphoreTake(xSemaphoreHTTP, pdMS_TO_TICKS(5000))) {
            char response[256] = {0};
            int code = http_post(API_SENSORS_DATA, json_str.c_str(), response, sizeof(response));
            xSemaphoreGive(xSemaphoreHTTP);

            if (code == 200 || code == 201) {
                Serial.printf("[UPLOAD] Success: HTTP %d, T=%.1fC, H=%.1f%%\n",
                              code, g_lastTemperature, g_lastHumidity);
            } else {
                Serial.printf("[UPLOAD] Failed: HTTP %d, response=%s\n", code, response);
            }
        }
    }
}

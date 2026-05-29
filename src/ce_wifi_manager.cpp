#include "ce_wifi_manager.h"
#include "ce_config_global.h"
#include <WiFi.h>

void wifi_manager_task(void *parameter) {
    Serial.println("[WIFI] Task started");
    
    uint8_t backoff_idx = 0;
    uint32_t backoff_times[] = CE_WIFI_RECONNECT_INTERVALS;
    uint8_t backoff_count = sizeof(backoff_times) / sizeof(backoff_times[0]);
    
    while (1) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!g_wifiConnected) {
                g_wifiConnected = true;
                g_wifiSignal = WiFi.RSSI();
                backoff_idx = 0;
                Serial.printf("[WIFI] Connected! IP: %s, Signal: %d dBm\n", 
                            WiFi.localIP().toString().c_str(), g_wifiSignal);
                xSemaphoreGive(xSemaphoreWiFi);
            }
            
            // Update signal periodically
            if (millis() % 10000 == 0) {
                g_wifiSignal = WiFi.RSSI();
            }
            
            delay(1000);
        } else {
            g_wifiConnected = false;
            
            Serial.printf("[WIFI] Disconnected, attempting reconnect...\n");
            WiFi.begin(CE_WIFI_SSID, CE_WIFI_PASSWORD);
            
            uint32_t backoff = backoff_times[backoff_idx];
            uint32_t start = millis();
            
            // Try to connect with timeout
            while (WiFi.status() != WL_CONNECTED && (millis() - start) < backoff) {
                delay(100);
            }
            
            if (backoff_idx < backoff_count - 1) {
                backoff_idx++;
            }
        }
    }
}

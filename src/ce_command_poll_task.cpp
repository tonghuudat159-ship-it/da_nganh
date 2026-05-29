#include "ce_command_poll_task.h"
#include "ce_config_global.h"
#include "ce_http_client.h"

void command_poll_task(void *parameter) {
    Serial.println("[CMND_POLL] Task started");
    TickType_t last_wake = xTaskGetTickCount();
    
    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(HTTP_COMMAND_INTERVAL));
        
        if (!g_wifiConnected) continue;
        
        // Build query URL
        String url = String(API_DEVICES_COMMAND) + "?deviceId=" + String(CE_DEVICE_ID);
        
        // Take HTTP semaphore
        if (xSemaphoreTake(xSemaphoreHTTP, pdMS_TO_TICKS(5000))) {
            char response[1024] = {0};
            int code = http_get(url.c_str(), response, sizeof(response));
            xSemaphoreGive(xSemaphoreHTTP);
            
            if (code == 200 && strlen(response) > 0) {
                Command cmds[5] = {0};
                int count = http_parse_commands(response, cmds, 5);
                
                for (int i = 0; i < count; i++) {
                    xQueueSend(xQueueCommand, &cmds[i], pdMS_TO_TICKS(100));
                    Serial.printf("[CMND_POLL] Queued: %s=%d\n", 
                                 cmds[i].device == RELAY_DEVICE_LIGHT ? "LIGHT" : "FAN", cmds[i].action);
                }
            }
        }
    }
}

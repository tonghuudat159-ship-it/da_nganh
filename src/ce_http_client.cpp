#include "ce_http_client.h"
#include "ce_config_global.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cstring>

static WiFiClient wifiClient;

int http_get(const char *path, char *response, uint16_t max_len) {
    if (!response || max_len == 0) return -1;
    response[0] = '\0';
    if (!path) return -1;

    HTTPClient http;

    String url = String(CE_BACKEND_URL) + String(path);
    http.begin(wifiClient, url);

    if (strlen(CE_DEVICE_SECRET) > 0) {
        http.addHeader("Authorization", String("Bearer ") + String(CE_DEVICE_SECRET));
    }

    http.setTimeout(5000);

    int httpCode = http.GET();

    if (httpCode > 0) {
        String payload = http.getString();
        strncpy(response, payload.c_str(), max_len - 1);
        response[max_len - 1] = '\0';
    }

    http.end();
    return httpCode;
}

int http_post(const char *path, const char *json_body, char *response, uint16_t max_len) {
    if (!response || max_len == 0) return -1;
    response[0] = '\0';
    if (!path || !json_body) return -1;

    HTTPClient http;

    String url = String(CE_BACKEND_URL) + String(path);
    http.begin(wifiClient, url);

    http.addHeader("Content-Type", "application/json");

    if (strlen(CE_DEVICE_SECRET) > 0) {
        http.addHeader("Authorization", String("Bearer ") + String(CE_DEVICE_SECRET));
    }

    http.setTimeout(5000);

    int httpCode = http.POST((uint8_t*)json_body, strlen(json_body));

    if (httpCode > 0) {
        String payload = http.getString();
        strncpy(response, payload.c_str(), max_len - 1);
        response[max_len - 1] = '\0';
    }

    http.end();
    return httpCode;
}

int http_parse_commands(const char *json, Command *commands, int max_count) {
    if (!json || !commands || max_count <= 0) return 0;
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.printf("[HTTP] JSON parse error: %s\n", error.c_str());
        return 0;
    }
    
    if (!doc["commands"]) return 0;
    JsonArray cmds = doc["commands"].as<JsonArray>();
    
    int count = 0;
    for (JsonObject cmd : cmds) {
        if (count >= max_count) break;
        
        strncpy(commands[count].commandId, cmd["commandId"] | "", sizeof(commands[count].commandId)-1);
        const char *device_str = cmd["device"] | "light";
        commands[count].device = (strcmp(device_str, "light") == 0) ? RELAY_DEVICE_LIGHT : RELAY_DEVICE_FAN;
        const char *action_str = cmd["action"] | "off";
        commands[count].action = (strcmp(action_str, "on") == 0) ? RELAY_ACTION_ON : RELAY_ACTION_OFF;
        commands[count].priority = cmd["priority"] | 0;
        commands[count].timestamp = cmd["timestamp"] | 0;
        commands[count].expiresAt = cmd["expiresAt"] | 0;
        count++;
    }
    return count;
}

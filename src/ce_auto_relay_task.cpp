#include "ce_auto_relay_task.h"
#include "ce_config_global.h"
#include <Arduino.h>

static void relay_set(uint8_t pin, bool state) {
    digitalWrite(pin, state ? RELAY_ON_STATE : RELAY_OFF_STATE);
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Debounce delay
}

static void relay_auto_init(void) {
    // Configure relay outputs
    pinMode(RELAY_LIGHT_PIN, OUTPUT);
    pinMode(RELAY_FAN_PIN, OUTPUT);

    // Configure sensor inputs
    pinMode(PIR_PIN, INPUT_PULLDOWN);
    pinMode(LIGHT_PIN, INPUT);

    // Set initial state: all OFF
    relay_set(RELAY_LIGHT_PIN, false);
    relay_set(RELAY_FAN_PIN, false);
    g_lightOn = false;
    g_fanOn = false;
    Serial.println("[AUTO_RELAY] Initialized");
}

static void control_fan_auto(float temperature, bool human_inside) {
    bool should_fan_be_on = (human_inside && temperature > FAN_TEMP_THRESHOLD);

    // If state changed, update relay
    if (should_fan_be_on != g_fanOn) {
        relay_set(RELAY_FAN_PIN, should_fan_be_on);
        g_fanOn = should_fan_be_on;

        Serial.printf("[AUTO_RELAY] FAN: %s (temp=%.1fC, human=%s)\n",
                      g_fanOn ? "ON" : "OFF", temperature, human_inside ? "YES" : "NO");
    }
}

static void control_light_auto(uint16_t light_level, bool human_inside) {
    // Light threshold: turn on light when light level is below threshold.
    // Assuming light sensor reads: HIGH value = BRIGHT, LOW value = DARK.
    bool should_light_be_on = (human_inside && light_level < LIGHT_THRESHOLD);
    
    // If state changed, update relay
    if (should_light_be_on != g_lightOn) {
        relay_set(RELAY_LIGHT_PIN, should_light_be_on);
        g_lightOn = should_light_be_on;

        Serial.printf("[AUTO_RELAY] LIGHT: %s (light_level=%u, human=%s)\n",
                      g_lightOn ? "ON" : "OFF", light_level, human_inside ? "YES" : "NO");
    }
}

void ce_auto_relay_task(void *parameter) {
    SensorData sensor_reading;
    uint32_t last_log_time = 0;
    const uint32_t LOG_INTERVAL_MS = 10000;  // Log every 10 seconds

    // Initialize relay pins
    relay_auto_init();

    Serial.println("[AUTO_RELAY] Task started - waiting for sensor data...");

    while (1) {
        // Try to receive sensor data from queue (5 second timeout)
        if (xQueueReceive(xQueueTempHumiForMain, &sensor_reading,
                          5000 / portTICK_PERIOD_MS) == pdTRUE) {
            uint16_t light_level = (uint16_t)sensor_reading.light;
            bool should_fan_be_on = (sensor_reading.human_inside &&
                                     sensor_reading.temperature > FAN_TEMP_THRESHOLD);
            bool should_light_be_on = (sensor_reading.human_inside &&
                                       light_level < LIGHT_THRESHOLD);

            Serial.printf("[AUTO] human=%s, light=%u/%u, D13_light=%s, temp=%.1f/%.1f, fan_D12=%s\n",
                          sensor_reading.human_inside ? "YES" : "NO",
                          light_level, LIGHT_THRESHOLD,
                          should_light_be_on ? "ON" : "OFF",
                          sensor_reading.temperature, FAN_TEMP_THRESHOLD,
                          should_fan_be_on ? "ON" : "OFF");

            // Control fan if auto mode is enabled
            if (IsFan_Auto) {
                control_fan_auto(sensor_reading.temperature, sensor_reading.human_inside);
            }

            // Control light if auto mode is enabled
            if (IsLight_Auto) {
                control_light_auto(light_level, sensor_reading.human_inside);
            }

            // Periodic logging (every 10 seconds)
            uint32_t now = millis();
            if (now - last_log_time >= LOG_INTERVAL_MS) {
                last_log_time = now;
                Serial.printf("[AUTO_RELAY] STATUS: Light=%s, Fan=%s | "
                              "Temp=%.1fC, Light=%u, Human=%s | "
                              "Auto(Light=%s, Fan=%s)\n",
                              g_lightOn ? "ON" : "OFF", g_fanOn ? "ON" : "OFF",
                              sensor_reading.temperature, light_level,
                              sensor_reading.human_inside ? "YES" : "NO",
                              IsLight_Auto ? "ON" : "OFF", IsFan_Auto ? "ON" : "OFF");
            }

        } else {
            // Timeout: no sensor data received
            // This could mean temp_humi_monitor task is not running or queue is not sending
            Serial.println("[AUTO_RELAY] TIMEOUT: No sensor data in queue");
        }
    }
}

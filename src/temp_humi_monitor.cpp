#include "temp_humi_monitor.h"
#include "ce_config_global.h"

DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);

static bool read_pir_motion_stable() {
    uint8_t active_count = 0;

    for (uint8_t i = 0; i < PIR_STABLE_READS_REQUIRED; i++) {
        int raw = digitalRead(PIR_PIN);
        if (raw == PIR_ACTIVE_STATE) {
            active_count++;
        }
        vTaskDelay(PIR_SAMPLE_DELAY_MS / portTICK_PERIOD_MS);
    }

    return active_count == PIR_STABLE_READS_REQUIRED;
}

static void delay_with_pir_poll(uint32_t duration_ms) {
    uint32_t start = millis();
    while (millis() - start < duration_ms) {
        (void)read_pir_motion_stable();
    }
}

void temp_humi_monitor(void *pvParameters) {
    pinMode(PIR_PIN, INPUT_PULLDOWN);
    pinMode(LIGHT_PIN, INPUT);

    Wire.begin(11, 12);
    dht20.begin();

    lcd.begin();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Task B-");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    uint32_t pir_start_ms = millis();

    while (1) {
        dht20.read();
        float temperature = dht20.getTemperature();
        float humidity = dht20.getHumidity();
        uint16_t light_level = analogRead(LIGHT_PIN);
        int pir_raw = digitalRead(PIR_PIN);
        bool pir_motion = read_pir_motion_stable();
        bool pir_ready = (millis() - pir_start_ms) >= PIR_WARMUP_TIME_MS;
        bool human_inside = pir_ready && pir_motion;

        Serial.printf("[PIR] raw=%d, stable_motion=%s, ready=%s, human_inside=%s\n",
                      pir_raw,
                      pir_motion ? "true" : "false",
                      pir_ready ? "true" : "false",
                      human_inside ? "true" : "false");

        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Sensor Error!");
            delay_with_pir_poll(SENSOR_READ_INTERVAL);
            continue;
        }

        SensorData sensordata;
        sensordata.temperature = temperature;
        sensordata.humidity = humidity;
        sensordata.light = light_level;
        sensordata.human_inside = human_inside;

        xQueueSend(xQueueForTinyML, &sensordata, 0);
        xQueueSend(xQueueTempHumiForMain, &sensordata, 0);

        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperature, 1);
        lcd.print((char)223);
        lcd.print("C");

        lcd.setCursor(0, 1);
        lcd.print("Humi: ");
        lcd.print(humidity, 1);
        lcd.print("%");

        lcd.setCursor(11, 0);
        if (temperature >= 35.0) {
            lcd.print("CRIT!");
        } else if (temperature >= 30.0) {
            lcd.print("WARN ");
        } else {
            lcd.print("NORM ");
        }

        delay_with_pir_poll(2500);

        // Screen 2: PIR motion after warmup and stable sampling.
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("PIR: ");
        if (human_inside) {
            lcd.print("Motion!");
        } else {
            lcd.print("Clear  ");
        }

        lcd.setCursor(0, 1);
        lcd.print("Light: ");
        lcd.print(light_level);

        delay_with_pir_poll(2500);
    }
}

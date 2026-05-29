#ifndef CE_AUTO_RELAY_TASK_H
#define CE_AUTO_RELAY_TASK_H

#include <Arduino.h>
#include "ce_config_global.h"
#include "ce_data_types.h"

/*============================================================
  SMART HOME CE AUTO RELAY TASK

  Purpose: Automatically control relays based on sensor data

  Logic:
    - FAN Auto: ON when temp > 30C AND occupancy is assumed true
               within PIR_HOLD_TIME_MS after the last PIR motion
    - LIGHT Auto: ON when occupancy is assumed true within
                 PIR_HOLD_TIME_MS after the last PIR motion and
                 light level < threshold

  Queue: Receives SensorData from xQueueTempHumiForMain
  External Control: IsLight_Auto, IsFan_Auto flags to enable/disable modes

  Author: Thai Khang & Dat
  ============================================================*/

/**
 * @brief Control fan based on temperature and PIR-backed occupancy hold
 * @param temperature Current temperature reading
 * @param human_inside Occupancy assumed true within PIR_HOLD_TIME_MS after motion
 */
static void control_fan_auto(float temperature, bool human_inside);

/**
 * @brief Control light based on PIR-backed occupancy hold and light level
 * @param light_level Current light sensor reading (0-4095)
 * @param human_inside Occupancy assumed true within PIR_HOLD_TIME_MS after motion
 */
static void control_light_auto(uint16_t light_level, bool human_inside);

/**
 * @brief Set relay state with debounce
 */
static void relay_set(uint8_t pin, bool state);

/**
 * @brief Initialize relay pins and sensors
 */
static void relay_auto_init(void);

/**
 * @brief FreeRTOS task for automatic relay control
 * Receives temperature/humidity from queue and controls relays accordingly
 * @param parameter FreeRTOS task parameter (unused)
 */
void ce_auto_relay_task(void *parameter);

#endif

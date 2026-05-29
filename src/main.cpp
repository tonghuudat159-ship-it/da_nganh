// Unified config & global file
#include "ce_config_global.h"
#include "temp_humi_monitor.h"
//#include "mainserver.h"
#include "tinyml.h"

// CE Firmware task headers
#include "ce_auto_relay_task.h"
#include "ce_http_upload_task.h"
#include "ce_command_poll_task.h"
#include "ce_wifi_manager.h"

void setup()
{
  Serial.begin(115200);
  Serial.println("[BOOT] Serial initialized at 115200");
  
  // Initialize CE globals (queues, semaphores)
  ce_globals_init();
  Serial.println("[BOOT] CE globals initialized");
  
  // Create legacy tasks (old project)
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 4096, NULL, 2, NULL);
  // xTaskCreate(tiny_ml_task, "Tiny ML Task" ,8192  ,NULL  ,2 , NULL);
   xTaskCreate(ce_auto_relay_task, "Auto Relay Task", 4096, NULL, 2, NULL);

  Serial.println("[BOOT] Setup complete - all tasks created");
}

void loop()
{

}
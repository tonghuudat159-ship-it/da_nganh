#include "tinyml.h"

// Globals, for the convenience of one-shot setup.
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024; // Adjust size based on your model
    uint8_t tensor_arena[kTensorArenaSize];

    float anomaly_threshold = 0.5;
    unsigned long last_adjustment = 0;
    int recent_anomalies = 0;
    int total_readings = 0;
    int readings = 10;

} 

void SetAnomalyThreshold()
{
    if (total_readings < readings)
    {
        return;
    }

    float anomaly_ratio = (float)recent_anomalies / (float)total_readings;
    
    // Biến để lưu trạng thái có cần in log hay không để tránh giữ Mutex quá lâu nếu không cần thiết
    bool threshold_changed = false;

    if (anomaly_ratio > 0.3)
    {
        anomaly_threshold += 0.05;
        // BẮT ĐẦU KHÓA MUTEX TRƯỚC KHI IN
        if (xSemaphoreTake(xSemaphoreMutex, portMAX_DELAY) == pdTRUE) {
            Serial.println("[TinyML] Too many anomalies, increasing threshold");
            xSemaphoreGive(xSemaphoreMutex); // TRẢ MUTEX NGAY
        }
        threshold_changed = true;
    }
    else if (anomaly_ratio < 0.05)
    {
        anomaly_threshold -= 0.05;
        // BẮT ĐẦU KHÓA MUTEX TRƯỚC KHI IN
        if (xSemaphoreTake(xSemaphoreMutex, portMAX_DELAY) == pdTRUE) {
            Serial.println("[TinyML] Too few anomalies, decreasing threshold");
            xSemaphoreGive(xSemaphoreMutex); // TRẢ MUTEX NGAY
        }
        threshold_changed = true;
    }

    // Kẹp giá trị ngưỡng trong khoảng an toàn
    if (anomaly_threshold < 0.3)
        anomaly_threshold = 0.3;
    if (anomaly_threshold > 0.8)
        anomaly_threshold = 0.8;

    // Chỉ in giá trị ngưỡng mới nếu có sự thay đổi
    if (threshold_changed) {
        if (xSemaphoreTake(xSemaphoreMutex, portMAX_DELAY) == pdTRUE) {
            Serial.print("[TinyML] New threshold: ");
            Serial.println(anomaly_threshold);
            xSemaphoreGive(xSemaphoreMutex);
        }
    }
    total_readings = 0;
    recent_anomalies = 0;
}

void setupTinyML()
{
    Serial.println("TensorFlow Lite Init....");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite); // g_model_data is from model_data.h
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model provided is schema version %d, not equal to supported version %d.",
                               model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
}

void tiny_ml_task(void *pvParameters)
{

    setupTinyML();

    while (1)
    {

        // Prepare input data (e.g., sensor readings)
        // For a simple example, let's assume a single float input
        SensorData data_receive;
        if (xQueueReceive(xQueueForTinyML, &data_receive, portMAX_DELAY) == pdTRUE)
        {

            total_readings++;

            if (total_readings >= readings)
            {
                SetAnomalyThreshold();
            }

            float temperature = data_receive.temperature;
            float humidity = data_receive.humidity;

            float temperature_normalized = (temperature - temperature_mean)/temperature_std;
            float humidity_normalized = (humidity - humidity_mean)/humidity_std;
                                               
            input->data.f[0] = temperature_normalized;
            input->data.f[1] = humidity_normalized;

            // Run inference
            TfLiteStatus invoke_status = interpreter->Invoke();
            if (invoke_status != kTfLiteOk)
            {
                error_reporter->Report("Invoke failed");
                return;
            }

            float result = output->data.f[0];

            String anomaly_type;
            bool anomaly_detected = false;

            if (result > anomaly_threshold + 0.2)
            {
                anomaly_type = "CRITICAL";
                anomaly_detected = true;
                recent_anomalies++;
            }
            else if (result >= anomaly_threshold)
            {
                anomaly_type = "WARNING";
                anomaly_detected = true;
                recent_anomalies++;
            }
            else
            {
                anomaly_type = "NORMAL";
                anomaly_detected = false;
            }

            MLResult ml_result;
            ml_result.temperature = data_receive.temperature;
            ml_result.humidity = data_receive.humidity;
            ml_result.inference_result = result;
            ml_result.anomaly_detected = anomaly_detected;
            ml_result.anomaly_type = anomaly_type;
            
            if (xSemaphoreTake(xSemaphoreMutex, portMAX_DELAY) == pdTRUE)
            {
                Serial.print("[TinyML] T:");
                Serial.print(ml_result.temperature);
                Serial.print(" H:");
                Serial.print(ml_result.humidity);
                Serial.print(" | Result:");
                Serial.print(ml_result.inference_result);
                Serial.print(" | Threshold:");
                Serial.print(anomaly_threshold, 2);
                Serial.print(" | Level:");
                Serial.println(ml_result.anomaly_type);
                xSemaphoreGive(xSemaphoreMutex);
            }
        }
    }
}
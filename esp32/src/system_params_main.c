#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/temperature_sensor.h"


#include <system_params.h>

static const char *TAG = "System params";


void system_params_task(void *pvParameters) {
    temperature_sensor_handle_t temp_handle;
    system_params_setup(&temp_handle);

    const char *TAG = "SYSTEM_PARAMS_TASK";

    ESP_LOGI(TAG, "System parameters task started.");
    
    while(1) {
        // Temperature readout
        read_temperature(temp_handle);
        // Time readout
        get_time();

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    vTaskDelete(NULL);
}

void app_main() {
    ESP_LOGI(TAG, "ESP32-S3 Boot completed successfully.");

    // Task to monitor system parameters
    xTaskCreate(
        (TaskFunction_t)system_params_task,
        "system_params_task",
        4096,
        NULL,
        5,
        NULL
    );
    
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
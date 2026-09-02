#include <time.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

#include <system_params.h>

void system_params_setup(temperature_sensor_handle_t *temp_handle) {
    // Temperature sensor
    temperature_sensor_config_t temp_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 50);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_config, temp_handle));
}

void read_temperature(temperature_sensor_handle_t temp_handle) {
    float temp_sensor_data = 0.0f;
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_handle));
    ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_handle, &temp_sensor_data));
    printf("Temperature: %.1f °C\n", temp_sensor_data);
    ESP_ERROR_CHECK(temperature_sensor_disable(temp_handle));
}

void get_time() {
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;
    
    time(&now);
    setenv("TZ", "UTC+2", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    printf("The current date/time is: %s\n", strftime_buf);
}

void init_trigger_pin(int gpio_pin) {
    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << gpio_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_config);
}

bool pin_triggered(int gpio_pin) { 
    if(gpio_get_level(gpio_pin) == 0) {
        vTaskDelay(pdMS_TO_TICKS(50));
        // If pin misread (program debouncing)
        if(gpio_get_level(gpio_pin) == 0) {
            while(gpio_get_level(gpio_pin) == 0) {
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            return true;
        }
    }
    return false;
}
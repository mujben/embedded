#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

#include <led_light_detector.h>

static const char *TAG = "Light detector";

void app_main() {
    ESP_LOGI(TAG, "ESP32-S3 Boot completed successfully.");

    // Led level read
    adc_oneshot_unit_handle_t led;
    setup_led_detector(&led);

    while(true) {
        int level = get_led_measurement(led);
        ESP_LOGI(TAG, "Light level: %d", level);
    }
}
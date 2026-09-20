#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "nimble/nimble_port_freertos.h"

#include <usb_hid.h>
#include <ble_rubber_ducky.h>

void app_main() {
    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // 1. start USB HID
    init_usb();

    // 2. start NimBLE
    nimble_init();
    nimble_port_freertos_init(nimble_host_task);

    // // crtl+alt+del
    // // send_key_combination(KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTALT, HID_KEY_DELETE);

    // // win + L
    // // send_key_combination(KEYBOARD_MODIFIER_LEFTGUI, HID_KEY_L);

    // while(true) {
    //     vTaskDelay(pdMS_TO_TICKS(20000));
    // }
}
#include "driver/gpio.h"

#include <spi_flash_read.h>
#include <spi_flash_write.h>

#define FLASH_PIN GPIO_NUM_4

void app_main() {
    init_trigger_pin(FLASH_PIN);
    spi_device_handle_t spi_flash_device;
    flash_device_setup(SPI2_HOST, &spi_flash_device);

    while(true) {
        if(pin_triggered(FLASH_PIN)) {
            char *message = "Test string written to flash memory W25Q64F! Written by ESP32-S3! ;)";
            write_flash_content(spi_flash_device, 0, (uint8_t *) message, strlen(message));
            read_flash_content(spi_flash_device, 0, 1024 * 8);
        }
    }
}
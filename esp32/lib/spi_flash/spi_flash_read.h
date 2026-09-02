#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"

#define SPI_CS_PIN GPIO_NUM_10
#define SPI_MOSI_PIN GPIO_NUM_11
#define SPI_CLK_PIN GPIO_NUM_12
#define SPI_MISO_PIN GPIO_NUM_13

#define TOTAL_FLASH_SIZE 8 * 1024 * 1024 // 8 MB
#define CHUNK_SIZE 512 // Read 512 bytes at once

void flash_device_setup(int spi_peripheral_id, spi_device_handle_t *spi_flash_device);

esp_err_t read_flash_chunk(spi_device_handle_t spi_flash_device, uint32_t address, uint8_t *out_buffer, size_t length);

void read_flash_content(spi_device_handle_t spi_flash_device, uint32_t start_addr, size_t length);
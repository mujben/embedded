#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"

#include <spi_flash_read.h>

static const char *TAG = "SPI Flash Read";

void flash_device_setup(int spi_peripheral_id, spi_device_handle_t *spi_flash_device) {

    spi_bus_config_t spi_bus_cfg = {
        .miso_io_num = SPI_MISO_PIN,
        .mosi_io_num = SPI_MOSI_PIN,
        .sclk_io_num = SPI_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4 + CHUNK_SIZE // 4 bytes for header
    };
    ESP_ERROR_CHECK(spi_bus_initialize(spi_peripheral_id, &spi_bus_cfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t spi_int_cfg = {
        .clock_speed_hz = 20 * 1000 * 1000, // 20MHz
        .mode = 0,
        .spics_io_num = SPI_CS_PIN,
        .queue_size = 1
    };
    ESP_ERROR_CHECK(spi_bus_add_device(spi_peripheral_id, &spi_int_cfg, spi_flash_device));

    uint8_t tx[4] = {0x9F, 0x00, 0x00, 0x00};
    uint8_t rx[4] = {0};
    spi_transaction_t transaction_device_id = {
        .length = 32,
        .tx_buffer = tx,
        .rx_buffer = rx
    };
    ESP_ERROR_CHECK(spi_device_transmit(*spi_flash_device, &transaction_device_id));
    ESP_LOGI(TAG, "Flash device ID: 0x%02X 0x%02X 0x%02X", rx[1], rx[2], rx[3]);
    if(rx[1] == 0xEF) {
        ESP_LOGI(TAG, "Compatible flash device detected. Winbound W25Q64\n");
    } else if (rx[1] == 0xFF || rx[1] == 0x00) {
        ESP_LOGW(TAG, "Connection error.");
    }
}

esp_err_t read_flash_chunk(spi_device_handle_t spi_flash_device, uint32_t address, uint8_t *out_buffer, size_t chunk_length) {
    size_t total_length = 4 + chunk_length; // 4 bytes for header (0x03 command + 3-byte address)
    
    DMA_ATTR static uint8_t tx_buffer[4 + CHUNK_SIZE];
    DMA_ATTR static uint8_t rx_buffer[4 + CHUNK_SIZE];

    memset(tx_buffer, 0, total_length);

    tx_buffer[0] = (uint8_t) 0x03;
    tx_buffer[1] = (uint8_t) (address >> 16); // & 0xFF;
    tx_buffer[2] = (uint8_t) (address >> 8); // & 0xFF;
    tx_buffer[3] = (uint8_t) address; // & 0xFF;

    spi_transaction_t transaction = {
        .length = total_length * 8, // bits
        .tx_buffer = tx_buffer,
        .rx_buffer = rx_buffer
    };

    esp_err_t err = spi_device_transmit(spi_flash_device, &transaction);
    if(err == ESP_OK){
        memcpy(out_buffer, &rx_buffer[4], chunk_length);
    }
    return err;
}

void read_flash_content(spi_device_handle_t spi_flash_device, uint32_t start_addr, size_t length) {
    uint8_t chunk_buff[CHUNK_SIZE];
    uint32_t current_addr = start_addr;
    uint32_t memory_limit = MIN(start_addr + length, TOTAL_FLASH_SIZE);

    while(current_addr < memory_limit) {
        size_t bytes_to_read = CHUNK_SIZE;
        if (current_addr + bytes_to_read > TOTAL_FLASH_SIZE) {
            bytes_to_read = TOTAL_FLASH_SIZE - current_addr;
        }
        // Read from flash

        if(read_flash_chunk(spi_flash_device, current_addr, chunk_buff, bytes_to_read) == ESP_OK) {
            ESP_LOGI(TAG, "Read %d bytes from address 0x%06X", bytes_to_read, current_addr);
            ESP_LOG_BUFFER_HEXDUMP(TAG, chunk_buff, bytes_to_read, ESP_LOG_INFO);
        }
        current_addr += bytes_to_read;
    }
}
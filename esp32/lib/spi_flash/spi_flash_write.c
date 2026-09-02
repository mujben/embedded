#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"

#include <spi_flash_write.h>

void write_flash_enable(spi_device_handle_t spi_flash_device) {
    uint8_t cmd = 0x06; // Enable write
    spi_transaction_t transaction = {
        .length = 8,
        .tx_buffer = &cmd
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi_flash_device, &transaction));
}

void flash_busy(spi_device_handle_t spi_flash_device) {
    uint8_t tx_buf[2] = {0x05, 0x00};
    uint8_t rx_buf[2] = {0};
    while(1) {
        spi_transaction_t transaction = {
            .length = 16,
            .tx_buffer = tx_buf,
            .rx_buffer = rx_buf
        };
        ESP_ERROR_CHECK(spi_device_transmit(spi_flash_device, &transaction));
        if((rx_buf[1] & 0x01) == 0x00) break;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void erase_flash_sector(spi_device_handle_t spi_flash_device, uint32_t address) {
    write_flash_enable(spi_flash_device);
    uint8_t cmd[4] = {
        0x20, // Erase sector
        (uint8_t) (address >> 16),
        (uint8_t) (address >> 8),
        (uint8_t) address
    };

    spi_transaction_t transaction = {
        .length = 32,
        .tx_buffer = cmd
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi_flash_device, &transaction));
    flash_busy(spi_flash_device);
}

int write_flash_chunk(spi_device_handle_t spi_flash_device, uint32_t address, uint8_t *data, size_t length) {
    if (length > 256) {
        ESP_LOGE("write_flash", "Length exceeds maximum page size of 256 bytes.");
        return ESP_ERR_INVALID_ARG;
    }
    write_flash_enable(spi_flash_device);
    
    size_t total_length = 4 + length; // 1 byte for command, 3 bytes for address
    DMA_ATTR static uint8_t tx_buf[4 + 256];
    tx_buf[0] = 0x02; // Page Program
    tx_buf[1] = (uint8_t) (address >> 16);
    tx_buf[2] = (uint8_t) (address >> 8);
    tx_buf[3] = (uint8_t) address;
    memcpy(&tx_buf[4], data, length);
    
    spi_transaction_t transaction = {
        .length = total_length * 8, // bits
        .tx_buffer = tx_buf
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi_flash_device, &transaction));
    flash_busy(spi_flash_device);
    return ESP_OK;
}

int write_flash_content(spi_device_handle_t spi_flash_device, uint32_t start_addr, uint8_t *data, size_t length) {
    if (length > TOTAL_FLASH_SIZE) {
        ESP_LOGE("write_flash", "Length exceeds total flash size.");
        return ESP_ERR_INVALID_ARG;
    }
    size_t bytes_written = 0;
    size_t last_erased_sector = -1;

    while (bytes_written < length) {
        uint32_t current_addr = start_addr + bytes_written;
        uint32_t current_sector = current_addr / SECTOR_SIZE;

        if(current_sector != last_erased_sector) {
            erase_flash_sector(spi_flash_device, current_addr);
            last_erased_sector = current_sector;
        }
        
        size_t page_offset = current_addr % PAGE_SIZE;
        size_t chunk_size = (length - bytes_written < PAGE_SIZE - page_offset) ? (length - bytes_written) : PAGE_SIZE - page_offset;
        int result = write_flash_chunk(spi_flash_device, start_addr + bytes_written, &data[bytes_written], chunk_size);
        if (result != ESP_OK) {
            return result;
        }
        bytes_written += chunk_size;
    }
    return ESP_OK;
}
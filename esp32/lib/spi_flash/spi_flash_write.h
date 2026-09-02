#include "driver/spi_master.h"

#include "spi_flash_read.h"

#define SECTOR_SIZE 4096
#define PAGE_SIZE 256

void write_flash_enable(spi_device_handle_t spi_flash_device);
void flash_busy(spi_device_handle_t spi_flash_device);
void erase_flash_sector(spi_device_handle_t spi_flash_device, uint32_t address);
int write_flash_chunk(spi_device_handle_t spi_flash_device, uint32_t address, uint8_t *data, size_t length);
int write_flash_content(spi_device_handle_t spi_flash_device, uint32_t start_addr, uint8_t *data, size_t length);
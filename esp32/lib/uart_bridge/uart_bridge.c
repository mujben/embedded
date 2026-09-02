#include "esp_log.h"

#include <uart_bridge.h>

static const char *TAG = "UART bridge";

void process_uart_data(uart_port_t uart_src, uart_port_t uart_dst, line_buffer_t *line_buffer) {
    uint8_t ch;
    
    while(uart_read_bytes(uart_src, &ch, 1, 0) > 0) {
        if(ch == '\r' || ch == '\n') {
            if(line_buffer->length > 0) {
                line_buffer->data[line_buffer->length] = '\n';
                
                line_buffer->length++;  // End the line buffer
                
                uart_write_bytes(uart_dst, line_buffer->data, line_buffer->length);
                ESP_LOGI(TAG, "Sent data [%d B] from UART%d -> UART%d", line_buffer->length, uart_src, uart_dst);

                line_buffer->length = 0;
            }
        }
        // Backspace deletion
        else if(ch == 0x08 || ch == 0x7F) {
            if (line_buffer->length > 0) {
                line_buffer->length--;
                line_buffer->data[line_buffer->length] = '\0';
            }
        }
        // Default case - adding to buffer
        else {
            if(line_buffer->length < BUFF_SIZE - 2) {
                line_buffer->data[line_buffer->length] = ch;
                line_buffer->length++;
            }
        }
    }
}

void setup_uart(uart_port_t uart_num, int tx_pin, int rx_pin) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122
    };

    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(uart_num, BUFF_SIZE * 2, BUFF_SIZE * 2, 0, NULL, 0));
}
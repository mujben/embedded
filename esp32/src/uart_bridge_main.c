#include <uart_bridge.h>

void app_main() {
    const uart_port_t uart_port_0 = UART_NUM_0;
    const uart_port_t uart_port_1 = UART_NUM_1;

    setup_uart(uart_port_0, TX1_PIN, RX1_PIN);
    setup_uart(uart_port_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    // Two buffers for two uart consoles
    line_buffer_t buff_uart_0 = {.length = 0};
    line_buffer_t buff_uart_1 = {.length = 0};

    while(true) {
        process_uart_data(uart_port_0, uart_port_1, &buff_uart_0);
        process_uart_data(uart_port_1, uart_port_0, &buff_uart_1);
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
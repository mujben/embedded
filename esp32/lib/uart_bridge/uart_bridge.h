#include "driver/uart.h"
#include "driver/gpio.h"

#define TX1_PIN GPIO_NUM_17
#define RX1_PIN GPIO_NUM_18
#define BUFF_SIZE 1024

typedef struct {
    char data[BUFF_SIZE];
    size_t length;
} line_buffer_t;

void process_uart_data(uart_port_t uart_src, uart_port_t uart_dst, line_buffer_t *line_buffer);

void setup_uart(uart_port_t uart_num, int tx_pin, int rx_pin);
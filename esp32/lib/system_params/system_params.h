#include "driver/temperature_sensor.h"

void system_params_setup(temperature_sensor_handle_t *temp_handle);
void read_temperature(temperature_sensor_handle_t temp_handle);
void get_time();

void init_trigger_pin(int gpio_pin);
bool pin_triggered(int gpio_pin);
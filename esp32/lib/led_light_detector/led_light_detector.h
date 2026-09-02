#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define DETECTOR_ADC_CHANNEL ADC_CHANNEL_0
#define DETECTOR_ADC_UNIT ADC_UNIT_1
#define DETECTOR_ADC_BITWIDTH ADC_BITWIDTH_12
#define DETECTOR_ADC_ATTEN ADC_ATTEN_DB_0

bool adc_calibration_init(adc_cali_handle_t *out_cali_handle);

void setup_led_detector(adc_oneshot_unit_handle_t *adc_device);
int get_led_measurement(const adc_oneshot_unit_handle_t adc_device);
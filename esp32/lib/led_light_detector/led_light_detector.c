#include "esp_log.h"

#include <led_light_detector.h>

static const char *TAG = "Led light detector";

static adc_cali_handle_t    cali_handle = NULL;
static bool                 do_calibration = false;

bool adc_calibration_init(adc_cali_handle_t *out_cali_handle) {
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Calibration using curve fitting scheme.");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = DETECTOR_ADC_UNIT,
        .atten = DETECTOR_ADC_ATTEN,
        .chan = DETECTOR_ADC_CHANNEL,
        .bitwidth = DETECTOR_ADC_BITWIDTH
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, out_cali_handle);
    if(ret == ESP_OK) calibrated = true;

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Calibration using line fitting scheme.");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = DETECTOR_ADC_UNIT,
        .atten = DETECTOR_ADC_ATTEN,
        .bitwidth = DETECTOR_ADC_BITWIDTH,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, out_cali_handle);
    if (ret == ESP_OK) calibrated = true;
#endif
    if(calibrated) {
        ESP_LOGI(TAG, "Successfully calibrated ADC.");
    } else {
        ESP_LOGW(TAG, "Error calibrating ADC. %s", esp_err_to_name(ret));
    }
    return calibrated;
}

void setup_led_detector(adc_oneshot_unit_handle_t *adc_device) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = DETECTOR_ADC_UNIT,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, adc_device));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = DETECTOR_ADC_BITWIDTH,
        .atten = DETECTOR_ADC_ATTEN
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_device, DETECTOR_ADC_CHANNEL, &config));
    do_calibration = adc_calibration_init(&cali_handle);
}

int get_led_measurement(const adc_oneshot_unit_handle_t adc_device) {
    int voltage_raw = 0;
    int voltage_mv = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_device, DETECTOR_ADC_CHANNEL, &voltage_raw));

    if(do_calibration) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, voltage_raw, &voltage_mv));
        return voltage_mv;
    } else {
        return voltage_raw;
    }
}
#include "hal/adc_types.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

#define TAG "slave"


adc_oneshot_unit_handle_t adc1_handle;

void init_ADC() {
    
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_0,
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config);
}

void read_ADC() {
    int adc_raw;
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw);
    ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, ADC_CHANNEL_6, adc_raw); 
}
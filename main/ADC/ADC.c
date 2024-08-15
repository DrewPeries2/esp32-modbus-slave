#include "hal/adc_types.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "ADC.h"

#define TAG "slave"


void init_ADC() {

    //populate TAGs structs
    //TODO: repeat the same for holding regs - make them share an index so they don't double book ADC channels
    int error_flag = 0;
    int i;
    for (i = 0; i < arraylen(input_registers); i++) {
        input_tags[i].registerNum = input_registers[i];
        if (i <= 8) {
            input_tags[i].channelnum = i;
        } else {
            ESP_LOGI(TAG, "More registers are being initialized than ADC channels"); 
            error_flag = 1;
            break;
        }
    }

    if (error_flag == 1) {
        return;
    }
    
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_0,
    };
    for (i = 0; i < arraylen(input_tags); i++) {
    adc_oneshot_config_channel(adc1_handle, input_tags[i].channelnum, &config);
    }
}

void read_ADC_logging() {
    int adc_raw;
    int i;
    for (i = 0; i < arraylen(input_tags); i++) {
            adc_oneshot_read(adc1_handle, input_tags[i].channelnum, &adc_raw);
            ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, input_tags[i].channelnum, adc_raw); 
            input_tags[i].value = adc_raw;
    }

}

void read_ADC() {
    int adc_raw;
    int i;
    for (i = 0; i < arraylen(input_tags); i++) {
            adc_oneshot_read(adc1_handle, input_tags[i].channelnum, &adc_raw);
            input_tags[i].value = adc_raw;
    }
}
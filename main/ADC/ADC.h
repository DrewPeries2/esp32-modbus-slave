#pragma once

#include "hal/adc_types.h"
#include "esp_adc/adc_oneshot.h"

#define arraylen(x) (sizeof(x) / sizeof((x)[0]))

typedef struct {
    int registerNum;
    int value;
    int channelnum;
} Tag;

Tag input_tags[10];  
Tag holding_tags[10];

adc_oneshot_unit_handle_t adc1_handle;

// Array of registers to be read
//MAXIMUM OF 8 - due to the ESP ADC only having eight channels
//expansion is possible, but you need to initialize ADC2 as well
uint16_t input_registers[] = {10, 9, 11, 12, 4, 0, 1};
uint16_t holding_registers[] = {2};
// Array of values read

void init_ADC();

void read_ADC();

void read_ADC_logging();

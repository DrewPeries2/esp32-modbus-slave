/*
==================================================================================================================================
    MODBUS COMMANDS
==================================================================================================================================
*/

#include <stdio.h>
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "ADC/ADC.c"
#include "esp_event.h"
#include "esp_log.h"
#include "modbus.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "mbcontroller.h" 
#include "wifi/wifi.c"

 uint8_t isInitialized = false;
 uint8_t isKilled = false;


static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;

uint16_t holding_reg_area[5000];
uint16_t input_reg_area[5000];





mb_communication_info_t comm_info = {
    .ip_port = 502,                    // Modbus TCP port number (default = 502)
    .ip_addr_type = MB_IPV4,                   // version of IP protocol
    .ip_mode = MB_MODE_TCP,                    // Port communication mode
    .ip_addr = NULL,                           // This field keeps the client IP address to bind, NULL - bind to any client
};

esp_err_t update_values() {
    //cycle through a list of all the values you need to append.
    // TODO: change "int value" to a pointer to an array of updating values
    //TODO: differentiate between holdign and input registers
                portENTER_CRITICAL(&param_lock);

                for (int i = 0; i < (sizeof(input_tags)/sizeof(input_tags[0])); i++) {
                input_reg_area[input_tags[i].registerNum] = input_tags[i].value;
                }
                portEXIT_CRITICAL(&param_lock);
                return ESP_OK;
}

//Slave Operation
void slave_operation_func_logging()
{
    mb_param_info_t reg_info; // keeps the Modbus registers access information
    uint8_t timer = 0;

    if(isInitialized == false) {
       ESP_LOGI(TAG, "Please initialize Modbus First!"); 
       return;
    }
    ESP_LOGI(TAG, "Starting modbus test...");
    // The cycle below will be terminated when parameter holding_data0
    // incremented each access cycle reaches the CHAN_DATA_MAX_VAL value.
    while(timer <= 10) {
        // Check for read/write events of Modbus master for certain events
        (void)mbc_slave_check_event(MB_READ_WRITE_MASK);
        ESP_ERROR_CHECK_WITHOUT_ABORT(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
        const char* rw_str = (reg_info.type & MB_READ_MASK) ? "READ" : "WRITE";
        // Filter events and process them accordingly
        if(reg_info.type & (MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD)) {
            // Get parameter information from parameter queue
            ESP_LOGI(TAG, "HOLDING %s (%" PRIu32 " us), ADDR:%u, TYPE:%u, INST_ADDR:0x%" PRIx32 ", SIZE:%u",
                            rw_str,
                            reg_info.time_stamp,
                            (unsigned)reg_info.mb_offset,
                            (unsigned)reg_info.type,
                            (uint32_t)reg_info.address,
                            (unsigned)reg_info.size);
        } else if (reg_info.type & MB_EVENT_INPUT_REG_RD) {
            ESP_LOGI(TAG, "INPUT READ (%" PRIu32 " us), ADDR:%u, TYPE:%u, INST_ADDR:0x%" PRIx32 ", SIZE:%u",
                            reg_info.time_stamp,
                            (unsigned)reg_info.mb_offset,
                            (unsigned)reg_info.type,
                            (uint32_t)reg_info.address,
                            (unsigned)reg_info.size);
        }
        timer++;
        vTaskDelay(100);
    }
    // Destroy of Modbus controller on alarm
    ESP_LOGI(TAG,"Modbus logging stopping");
    vTaskDelay(100);
}

//Slave Operation need to change
void slave_operation_func()
{

    mb_param_info_t reg_info; // keeps the Modbus registers access information
    uint8_t error_flag = 0;

    if(isInitialized == false) {
       ESP_LOGI(TAG, "Please initialize Modbus First!"); 
       error_flag = 1;
    }

    ESP_LOGI(TAG, "Start modbus test...");
    // The cycle below will be terminated when parameter holding_data0
    // incremented each access cycle reaches the CHAN_DATA_MAX_VAL value.
    while(error_flag == 0) {

        if (isKilled == true) {
        ESP_LOGI(TAG, "Operation exiting...");
        isKilled = false;
        error_flag = 1;
        }

        //read ADC, update values
        read_ADC();
        update_values();
        // Check for read/write events of Modbus master for certain events
        //note for future programmers: the program stops at this line until its interacted with by a master
        (void)mbc_slave_check_event(MB_READ_WRITE_MASK);
        ESP_ERROR_CHECK_WITHOUT_ABORT(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
        const char* rw_str = (reg_info.type & MB_READ_MASK) ? "READ" : "WRITE";
        // Filter events and process them accordingly
        if(reg_info.type & (MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD)) {
            // Get parameter information from parameter queue
            ESP_LOGI(TAG, "HOLDING %s (%" PRIu32 " us), ADDR:%u, TYPE:%u, INST_ADDR:0x%" PRIx32 ", SIZE:%u",
                            rw_str,
                            reg_info.time_stamp,
                            (unsigned)reg_info.mb_offset,
                            (unsigned)reg_info.type,
                            (uint32_t)reg_info.address,
                            (unsigned)reg_info.size);
            if (reg_info.address >= (uint8_t*)&holding_reg_area[5000])
            {
                error_flag = 1;
            }
        } else if (reg_info.type & MB_EVENT_INPUT_REG_RD) {
            ESP_LOGI(TAG, "INPUT READ (%" PRIu32 " us), ADDR:%u, TYPE:%u, INST_ADDR:0x%" PRIx32 ", SIZE:%u",
                            reg_info.time_stamp,
                            (unsigned)reg_info.mb_offset,
                            (unsigned)reg_info.type,
                            (uint32_t)reg_info.address,
                            (unsigned)reg_info.size);
        }
    }
    // Destroy of Modbus controller on alarm
    mbc_slave_destroy();
    ESP_LOGI(TAG,"Modbus slave/controller destroyed.");
    ESP_LOGI(TAG,"Please reinitialize.");
    vTaskDelay(100);
    vTaskDelete(NULL);
}


esp_err_t kill_slave() {
    isKilled = true;
    portENTER_CRITICAL(&param_lock);
    holding_reg_area[1] = 1;
    portEXIT_CRITICAL(&param_lock);
    
    return ESP_OK;
}


esp_err_t slave_init(mb_communication_info_t* comm_info) {

    init_ADC();

    mb_register_area_descriptor_t reg_area; // Modbus register area descriptor structure
    void* slave_handler = NULL;

    // Initialization of Modbus controller
    esp_err_t err = mbc_slave_init_tcp(&slave_handler);
    MB_RETURN_ON_FALSE((err == ESP_OK && slave_handler != NULL), ESP_ERR_INVALID_STATE,
                                TAG,
                                "mb controller initialization fail.");

    // Setup communication parameters and start stack
    comm_info->ip_addr = NULL; // Bind to any address
    comm_info->ip_netif_ptr = sta_netif;
    comm_info->slave_uid = MB_SLAVE_ID;
    err = mbc_slave_setup((void*)comm_info);

    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                        TAG,
                                        "mbc_slave_setup fail, returns(0x%x).",
                                        (int)err);

    // The code below initializes Modbus register area descriptors
    // If master calls an uninitialized register area, it will throw a stack exception
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = MB_REG_HOLDING_START_AREA0; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&holding_reg_area[0]; // Set pointer to storage instance
    reg_area.size = (5000) << 1; // Set the size of register storage instance
    err = mbc_slave_set_descriptor(reg_area);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                    TAG,
                                    "mbc_slave_set_descriptor fail, returns(0x%x).",
                                    (int)err);
    // Initialization of Input Registers area
    reg_area.type = MB_PARAM_INPUT;
    reg_area.start_offset = MB_REG_INPUT_START_AREA0;
    reg_area.address = (void*)&input_reg_area[0];
    reg_area.size = 5000 << 1;
    err = mbc_slave_set_descriptor(reg_area);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                        TAG,
                                        "mbc_slave_set_descriptor fail, returns(0x%x).",
                                        (int)err);
    // Starts of modbus controller and stack
    err = mbc_slave_start();
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                        TAG,
                                        "mbc_slave_start fail, returns(0x%x).",
                                        (int)err);
    vTaskDelay(5);
    isInitialized = true;
    return err;
}



static esp_err_t slave_destroy(void)
{
    esp_err_t err = mbc_slave_destroy();
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                TAG,
                                "mbc_slave_destroy fail, returns(0x%x).",
                                (int)err);
    return err;
}

void start_slave() {
    
        xTaskCreatePinnedToCore(
        slave_operation_func,       // Task function
        "modbus-slave",      // Task name
        8192,          // Stack size in bytes
        NULL,          // Parameter to pass
        1,             // Task priority
        NULL,          // Task handle
        0              // Core ID (1)
    );
        vTaskDelay(100);
}
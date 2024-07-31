/*
==================================================================================================================================
    Commands
==================================================================================================================================
*/

#include <stdio.h>
#include <string.h>
#include "wifi.c"
#include "modbus.c"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"


#define TAG "slave"

extern void register_wifi() {
    const esp_console_cmd_t cmd = {
        .command = "connect_wpa2",
        .help = "Get the current size of free heap memory",
        .hint = NULL,
        .func = &init_wifi,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}

/*
==================================================================================================================================
    Commands
==================================================================================================================================
*/

#include <stdio.h>
#include <string.h>
#include "wifi/wifi.c"
#include "modbus/modbus.c"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"


#define TAG "slave"

//need to add usernamne, pass, ssid
//only return when you connect
//add a thing where ti disconnects before it tries to connect
extern void register_wifi() {
    const esp_console_cmd_t cmd = {
        .command = "connect_wpa2",
        .help = "connect to wpa2 enterprise",
        .hint = NULL,
        .func = &init_wifi,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}
//command to show ip

//need to show 

extern void register_modbus_init() {
    const esp_console_cmd_t cmd = {
        .command = "init_modbus",
        .help = "initialize modbus",
        .hint = NULL,
        .func_w_context = &slave_init,
        .context = &comm_info
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}

extern void register_start_modbus() {
    const esp_console_cmd_t cmd = {
        .command = "start_modbus",
        .help = "start modbus",
        .hint = NULL,
        .func = &slave_operation_func,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}
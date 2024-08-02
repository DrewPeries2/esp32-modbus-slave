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
extern void register_wifi_enterprise() {

    wifi_enterprise_args.ssid = arg_str1(NULL, NULL, "<ssid>", "Wi-Fi SSID");
    wifi_enterprise_args.username = arg_str1(NULL, NULL, "<username>", "Wi-Fi username");
    wifi_enterprise_args.password = arg_str1(NULL, NULL, "<password>", "Wi-Fi password");
    wifi_enterprise_args.id = arg_str1(NULL, NULL, "<id>", "Wi-Fi ID");
    wifi_enterprise_args.end = arg_end(4);

    const esp_console_cmd_t cmd = {
        .command = "connect_wifi_enterprise",
        .help = "connect to wpa2 enterprise",
        .hint = NULL,
        .func = &init_wifi_enterprise,
        .argtable = &wifi_enterprise_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}

extern void register_wifi() {

    wifi_args.ssid = arg_str1(NULL, NULL, "<ssid>", "Wi-Fi SSID");
    wifi_args.password = arg_str1(NULL, NULL, "<password>", "Wi-Fi password");
    wifi_args.end = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command = "connect_wifi",
        .help = "connect to wpa2 enterprise",
        .hint = NULL,
        .func = &init_wifi, 
        .argtable = &wifi_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}

//command to show ip
extern void register_ip() {
    const esp_console_cmd_t cmd = {
        .command = "show_ip",
        .help = "display IP address",
        .hint = NULL,
        .func = &show_ip, 
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}

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


//USE MULTIPROCESSING
extern void register_start_modbus() {
    const esp_console_cmd_t cmd = {
        .command = "start_modbus",
        .help = "start modbus",
        .hint = NULL,
        .func = &start_slave,

    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}
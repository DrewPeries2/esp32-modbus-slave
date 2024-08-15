/*
==================================================================================================================================
    Commands
==================================================================================================================================
*/

#include <stdio.h>
#include <string.h>
#include "wifi/wifi.h"
#include "modbus/modbus.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

struct wifi_args wpa_args;
struct wifi_enterprise_args wpa2_args;

#define TAG "slave"
    void register_modbus_init();
    void register_start_modbus();
    void register_start_modbus_logs();
    void register_kill();
    void register_wifi_enterprise();
    void register_wifi();
    void register_disconnect();
    void register_ip();
    void register_setup_wifi();
//need to add usernamne, pass, ssid
//only return when you connect
//add a thing where ti disconnects before it tries to connect

extern void register_wifi_commands() {
    register_wifi_enterprise();
    register_wifi();
    register_disconnect();
    register_ip();
    register_setup_wifi();
}

extern void register_modbus_commands() {
    register_modbus_init();
    register_start_modbus();
    register_start_modbus_logs();
    register_kill();
}

void setup_wifi_at_start() {
    setup_wifi();
}

void register_wifi_enterprise() {

    wpa2_args.ssid = arg_str1(NULL, NULL, "<ssid>", "Wi-Fi SSID");
    wpa2_args.username = arg_str1(NULL, NULL, "<username>", "Wi-Fi username");
    wpa2_args.password = arg_str1(NULL, NULL, "<password>", "Wi-Fi password");
    wpa2_args.id = arg_str1(NULL, NULL, "<id>", "Wi-Fi ID");
    wpa2_args.end = arg_end(4);

    const esp_console_cmd_t cmd = {
        .command = "connect_wifi_enterprise",
        .help = "connect to wpa2 enterprise",
        .hint = NULL,
        .func = &init_wifi_enterprise,
        .argtable = &wpa2_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}

void register_wifi() {

    wpa_args.ssid = arg_str1(NULL, NULL, "<ssid>", "Wi-Fi SSID");
    wpa_args.password = arg_str1(NULL, NULL, "<password>", "Wi-Fi password");
    wpa_args.end = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command = "connect_wifi",
        .help = "connect to wpa wifi",
        .hint = NULL,
        .func = &init_wifi, 
        .argtable = &wpa_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}

void register_disconnect() {
    const esp_console_cmd_t cmd = {
    .command = "disconnect_wifi",
    .help = "disconnect from wifi",
    .hint = NULL,
    .func = &disconnect_wifi,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}
//command to show ip
void register_ip() {
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

void register_setup_wifi() {
        const esp_console_cmd_t cmd = {
        .command = "setup_wifi",
        .help = "setup wifi configurations",
        .hint = NULL,
        .func = &setup_wifi, 
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}

void register_modbus_init() {
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

void register_kill() {
        const esp_console_cmd_t cmd = {
        .command = "kill_slave",
        .help = "kill modbus",
        .hint = NULL,
        .func = &kill_slave,

    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}


//USE MULTIPROCESSING
void register_start_modbus() {
    const esp_console_cmd_t cmd = {
        .command = "start_modbus",
        .help = "start modbus",
        .hint = NULL,
        .func = &start_slave,

    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}

void register_start_modbus_logs() {
        const esp_console_cmd_t cmd = {
        .command = "start_modbus_logs",
        .help = "start modbus",
        .hint = NULL,
        .func = &slave_operation_func_logging,

    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
    return;
}
#pragma once

#include "esp_err.h"

struct wifi_args {
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
};

struct wifi_enterprise_args {
    struct arg_str *ssid;
    struct arg_str *username;
    struct arg_str *password;
    struct arg_str *id;
    struct arg_end *end;
};

extern struct wifi_args wpa_args;
extern struct wifi_enterprise_args wpa2_args;



esp_err_t init_wifi(int argc, char **argv);


//TODO: TWEAK LATER - implement busy-wait sync with the event handler - blind cycle is so scuffed
 esp_err_t init_wifi_enterprise(int argc, char **argv);

esp_err_t setup_wifi();

esp_err_t show_ip();


esp_err_t disconnect_wifi(void);
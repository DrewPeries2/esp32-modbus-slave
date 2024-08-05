/*
=============================================================================================================================
    WIFI COMMANDS
=============================================================================================================================
*/ 
#pragma once

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "argtable3/argtable3.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mbcontroller.h" 

#define TAG "slave"

#define EXAMPLE_WIFI_SSID SECRET_SSID
#define EXAMPLE_EAP_METHOD "PEAP"

#define EXAMPLE_EAP_ID SECRET_ID
#define EXAMPLE_EAP_USERNAME SECRET_USERNAME
#define EXAMPLE_EAP_PASSWORD SECRET_PASSWORD



/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* esp netif object representing the WIFI station */
extern esp_netif_t *sta_netif = NULL;
esp_netif_ip_info_t ip;
/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

bool isConnected = false;

static struct {
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} wifi_args;

static struct {
    struct arg_str *ssid;
    struct arg_str *username;
    struct arg_str *password;
    struct arg_str *id;
    struct arg_end *end;
} wifi_enterprise_args;

static char stored_ssid[32] = {0};
static char stored_password[64] = {0};
static char stored_username[32] = {0};
static char stored_id[32] = {0};


static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}



//TODO: TWEAK LATER - implement busy-wait sync with the event handler - blind cycle is so scuffed
extern esp_err_t init_wifi(int argc, char **argv) {
    if (isConnected == true) {
    ESP_LOGI(TAG, "Already connected to wifi!");
    ESP_LOGI(TAG, "Run show_ip to see ip address");
    return ESP_OK;
}

    int nerrors = arg_parse(argc, argv, (void **)&wifi_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, wifi_args.end, argv[0]);
        return 1;
    }

    strncpy(stored_ssid, wifi_args.ssid->sval[0], sizeof(stored_ssid) - 1);
    strncpy(stored_password, wifi_args.password->sval[0], sizeof(stored_password) - 1);

    ESP_LOGI(TAG, "%s", stored_ssid);
    ESP_LOGI(TAG, "%s", stored_password);

ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_deinit();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .pmf_cfg = {
                .required = false
            },
        },
    };
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", stored_ssid);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", stored_password);
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);

    //CREDENTIALS 
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

    //CONNECTING 
    ESP_ERROR_CHECK(esp_wifi_start());

    //WAITING FOR CONNECTION
    memset(&ip, 0, sizeof(esp_netif_ip_info_t));
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    uint8_t x = 0;
    while (x < 3) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        if (esp_netif_get_ip_info(sta_netif, &ip) == 0) {
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "IP:"IPSTR, IP2STR(&ip.ip));
            ESP_LOGI(TAG, "MASK:"IPSTR, IP2STR(&ip.netmask));
            ESP_LOGI(TAG, "GW:"IPSTR, IP2STR(&ip.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
        }
        x++;
    }
    char ip_str[16];
    sprintf(ip_str, IPSTR, IP2STR(&ip.ip));

    ESP_LOGI(TAG, "%s", ip_str);

    if (strcmp(ip_str, "0.0.0.0") == 0) {
        ESP_LOGI(TAG, "Unable to connect. Stopping...");
        esp_wifi_disconnect();
    } else {
        isConnected = true;
    }
    return ESP_OK;
}

extern esp_err_t setup_wifi () {
        if (isConnected == true) {
        ESP_LOGI(TAG, "Already connected to wifi!");
        ESP_LOGI(TAG, "Run show_ip to see ip address");
        return ESP_OK;
    }

    //CONFIGURATION
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_deinit();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    return ESP_OK;
}



//TODO: TWEAK LATER - implement busy-wait sync with the event handler - blind cycle is so scuffed
extern esp_err_t init_wifi_enterprise(int argc, char **argv)
{
    if (isConnected == true) {
        ESP_LOGI(TAG, "Already connected to wifi!");
        ESP_LOGI(TAG, "Run show_ip to see ip address");
        return ESP_OK;
    }
    int nerrors = arg_parse(argc, argv, (void **)&wifi_enterprise_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, wifi_args.end, argv[0]);
        return 1;
    }

    strncpy(stored_ssid, wifi_enterprise_args.ssid->sval[0], sizeof(stored_ssid) - 1);
    strncpy(stored_username, wifi_enterprise_args.username->sval[0], sizeof(stored_username) - 1);
    strncpy(stored_password, wifi_enterprise_args.password->sval[0], sizeof(stored_password) - 1);
    strncpy(stored_id, wifi_enterprise_args.id->sval[0], sizeof(stored_id) - 1);


    wifi_config_t wifi_config = {
        .sta = {
            .pmf_cfg = {
                .required = false
            },
        },
    };
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", stored_ssid);
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    //CREDENTIALS 
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_eap_client_set_identity((uint8_t *)stored_id, strlen(stored_id)) );
    ESP_ERROR_CHECK(esp_eap_client_set_username((uint8_t *)stored_username, strlen(stored_username)) );
    ESP_ERROR_CHECK(esp_eap_client_set_password((uint8_t *)stored_password, strlen(stored_password)) );
    //CONNECTING
    ESP_ERROR_CHECK(esp_wifi_sta_enterprise_enable());  
    ESP_ERROR_CHECK(esp_wifi_start());
    //WAITING FOR CONNECTION
    memset(&ip, 0, sizeof(esp_netif_ip_info_t));
    vTaskDelay(10000 / portTICK_PERIOD_MS);
        if (esp_netif_get_ip_info(sta_netif, &ip) == 0) {
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "IP:"IPSTR, IP2STR(&ip.ip));
            ESP_LOGI(TAG, "MASK:"IPSTR, IP2STR(&ip.netmask));
            ESP_LOGI(TAG, "GW:"IPSTR, IP2STR(&ip.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
    }
    char ip_str[16];
    sprintf(ip_str, IPSTR, IP2STR(&ip.ip));

    ESP_LOGI(TAG, "%s", ip_str);

    if (strcmp(ip_str, "0.0.0.0") == 0) {
        esp_wifi_disconnect();
    } else {
        isConnected = true;
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    return ESP_OK;
}



esp_err_t show_ip() {
    if (isConnected == false) {
        ESP_LOGI(TAG, "Not connected to wifi!");
        return ESP_OK;
    }
    if (esp_netif_get_ip_info(sta_netif, &ip) == 0) {
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "IP:"IPSTR, IP2STR(&ip.ip));
            ESP_LOGI(TAG, "MASK:"IPSTR, IP2STR(&ip.netmask));
            ESP_LOGI(TAG, "GW:"IPSTR, IP2STR(&ip.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
    }
    return ESP_OK;

}


static esp_err_t disconnect_wifi(void)
{
    esp_err_t err = ESP_OK;

    if (isConnected == false) {
        ESP_LOGI(TAG, "Please connect to wifi first!");
        return ESP_OK;
    }
    err = esp_wifi_stop();
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                       TAG,
                                       "Unable to disconnect, returns(0x%x).",
                                       (int)err);
    isConnected = false;
    return err;
}
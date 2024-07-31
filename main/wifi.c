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
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "secrets.h"
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

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;


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

extern esp_err_t init_wifi(void)
{

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
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .pmf_cfg = {
                .required = false
            },
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);

    //CREDENTIALS 
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_eap_client_set_identity((uint8_t *)EXAMPLE_EAP_ID, strlen(EXAMPLE_EAP_ID)) );
    ESP_ERROR_CHECK(esp_eap_client_set_username((uint8_t *)EXAMPLE_EAP_USERNAME, strlen(EXAMPLE_EAP_USERNAME)) );
    ESP_ERROR_CHECK(esp_eap_client_set_password((uint8_t *)EXAMPLE_EAP_PASSWORD, strlen(EXAMPLE_EAP_PASSWORD)) );

    //CONNECTING
    ESP_ERROR_CHECK(esp_wifi_sta_enterprise_enable());  
    ESP_ERROR_CHECK(esp_wifi_start());

    //WAITING FOR CONNECTION
    esp_netif_ip_info_t ip;
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
    return ESP_OK;
}




static esp_err_t destroy_services(void)
{
    esp_err_t err = ESP_OK;

    err = esp_event_loop_delete_default();
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                       TAG,
                                       "esp_event_loop_delete_default fail, returns(0x%x).",
                                       (int)err);
    err = esp_netif_deinit();
    MB_RETURN_ON_FALSE((err == ESP_OK || err == ESP_ERR_NOT_SUPPORTED), ESP_ERR_INVALID_STATE,
                                        TAG,
                                        "esp_netif_deinit fail, returns(0x%x).",
                                        (int)err);
    err = nvs_flash_deinit();
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
                                TAG,
                                "nvs_flash_deinit fail, returns(0x%x).",
                                (int)err);

    return err;
}
#include <stdio.h>
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "esp_netif.h"
#include "esp_eap_client.h"
#include "esp_mac.h"
#include "mbcontroller.h" 
#include "secrets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "nvs.h"

// WIFI Defines
#define EXAMPLE_WIFI_SSID SECRET_SSID
#define EXAMPLE_EAP_METHOD "PEAP"

#define EXAMPLE_EAP_ID SECRET_ID
#define EXAMPLE_EAP_USERNAME SECRET_USERNAME
#define EXAMPLE_EAP_PASSWORD SECRET_PASSWORD

// MB DEFINES

#define MB_SLAVE_ID (1)
#define MB_TCP_PORT_NUMBER      (CONFIG_FMB_TCP_PORT_DEFAULT)
#define MB_MDNS_PORT            (502)   

// Defines below are used to define register start address for each type of Modbus registers
#define MB_REG_INPUT_START_AREA0            0 // register offset input area 0
#define MB_REG_HOLDING_START_AREA0          0

#define MB_PAR_INFO_GET_TOUT                (10) // Timeout for get parameter info
#define MB_CHAN_DATA_MAX_VAL                (10)
#define MB_CHAN_DATA_OFFSET                 (1.1f)

#define MB_READ_MASK                        (MB_EVENT_INPUT_REG_RD \
                                                | MB_EVENT_HOLDING_REG_RD \
                                                | MB_EVENT_DISCRETE_RD \
                                                | MB_EVENT_COILS_RD)
#define MB_WRITE_MASK                       (MB_EVENT_HOLDING_REG_WR \
                                                | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK                  (MB_READ_MASK | MB_WRITE_MASK)

#define MB_SLAVE_ADDR (CONFIG_MB_SLAVE_ADDR)


//Data Structures
mb_communication_info_t comm_info = {
    .ip_port = 502,                    // Modbus TCP port number (default = 502)
    .ip_addr_type = MB_IPV4,                   // version of IP protocol
    .ip_mode = MB_MODE_TCP,                    // Port communication mode
    .ip_addr = NULL,                           // This field keeps the client IP address to bind, NULL - bind to any client
};

static const char *TAG = "SLAVE_DEMO";

static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* esp netif object representing the WIFI station */
static esp_netif_t *sta_netif = NULL;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

uint16_t holding_reg_area[5000];
uint16_t input_reg_area[5000];




//Slave Operation
static void slave_operation_func(void *arg)
{
    mb_param_info_t reg_info; // keeps the Modbus registers access information
    uint8_t error_flag = 0;

    ESP_LOGI(TAG, "Modbus slave stack initialized.");
    ESP_LOGI(TAG, "Start modbus test...");
    // The cycle below will be terminated when parameter holding_data0
    // incremented each access cycle reaches the CHAN_DATA_MAX_VAL value.
    while(error_flag == 0) {
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
    ESP_LOGI(TAG,"Modbus controller destroyed.");
    vTaskDelay(100);
}


static esp_err_t slave_init(mb_communication_info_t* comm_info) {

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

static void create_test_data() {
            portENTER_CRITICAL(&param_lock);
        holding_reg_area[1] = 1;
        holding_reg_area[2] = 2;
        holding_reg_area[3] = 3;
        holding_reg_area[4] = 4;

        holding_reg_area[11] = 11;
        holding_reg_area[12] = 12;
        holding_reg_area[13] = 13;
        holding_reg_area[14] = 14;

        input_reg_area[501] = 173;
        input_reg_area[506] = 543;
        portEXIT_CRITICAL(&param_lock);
}


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


static esp_err_t init_wifi(void)
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

void app_main(void) {

    //new setup:
        //register all commands in here, and initialize slave i guess? dunno how import settings will work

        esp_console_repl_t *repl = NULL;
        esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
        repl_config.prompt = PROMPT_STR ">";
        repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;

    initialize_nvs();

        ESP_ERROR_CHECK(nvs_flash_init());

        ESP_ERROR_CHECK(init_wifi());

        esp_log_level_set(TAG, ESP_LOG_INFO);

        comm_info.ip_addr_type = MB_IPV4;
        comm_info.ip_mode = MB_MODE_TCP;
        comm_info.ip_port = MB_TCP_PORT_NUMBER;
        ESP_ERROR_CHECK(slave_init(&comm_info));

        create_test_data();
        slave_operation_func(NULL);

        ESP_ERROR_CHECK(slave_destroy());
        ESP_ERROR_CHECK(destroy_services());

}

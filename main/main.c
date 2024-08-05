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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "commands.c"
//Defines

#define PROMPT_STR "slave"
#define CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH 1024

#define TAG "slave"


/* Console command history can be stored to and loaded from a file.
 * The easiest way to do this is to use FATFS filesystem on top of
 * wear_levelling library.
 */
#if CONFIG_CONSOLE_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}
#endif // CONFIG_STORE_HISTORY

static void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void start_CLI() {
    //new setup:
        esp_console_repl_t *repl = NULL;
        esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
        repl_config.prompt = PROMPT_STR ">";
        repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;

        initialize_nvs();
        ESP_ERROR_CHECK(nvs_flash_init());
        esp_log_level_set(TAG, ESP_LOG_VERBOSE);

        //registering command families
        register_wifi_commands();
        register_modbus_commands();
        setup_wifi_at_start();

        esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
        ESP_LOGI(TAG, "Starting REPL");
        ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
        esp_console_start_repl(repl);
       //vTaskDelete(NULL);
        while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay loop
    }
}

void app_main(void) {
    xTaskCreatePinnedToCore(
        start_CLI,       // Task function
        "CLI",      // Task name
        8192,          // Stack size in bytes
        NULL,          // Parameter to pass
        1,             // Task priority
        NULL,          // Task handle
        1              // Core ID (1)
    );
}

//#include "file_storage.h"
#include "wifi_setup.h"
#include "https_server.h"
#include "led_control.h"


static const char* TAG= "MAIN";
void app_main()
 {
 ESP_LOGI(TAG, "Initializing LED...");
    configure_led();  // Initialize the LED strip
    set_brightness(30);
    ESP_LOGI(TAG, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI("APP_MAIN", "Starting Wi-Fi setup...");
    wifi_init_sta();
    start_https_server();  // Start HTTP server
}


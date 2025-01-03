// #include "file_storage.h"
#include "https_server.h"
#include "led_control.h"
#include "wifi_setup.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <inttypes.h>

static const char *TAG = "MAIN";
// Task to log free heap memory periodically
void log_free_heap_task(void *arg) {
    while (1) {
    ESP_LOGI(TAG, "Free heap memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    vTaskDelay(pdMS_TO_TICKS(50000)); // Log every 5 seconds
    }
}

// Function to start the heap logging task
void start_heap_logging(void) {
    xTaskCreate(log_free_heap_task, "log_free_heap_task", 2048, NULL, 5, NULL);
}
void app_main() {
  ESP_LOGI(TAG, "Initializing LED...");
  configure_led(); // Initialize the LED strip
  set_brightness(30);
  ESP_LOGI(TAG, "Initializing NVS...");
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_LOGI(TAG, "Initializing Wi-Fi...");
  wifi_init_sta();
  static httpd_handle_t https_server_handle = NULL;

ESP_LOGI(TAG, "Registering HTTPS server event handlers...");
ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &https_connect_handler, &https_server_handle));
ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &https_disconnect_handler, &https_server_handle));
ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_SERVER_EVENT, ESP_EVENT_ANY_ID, &https_server_event_handler, NULL));


ESP_LOGI(TAG, "Event handlers registered.");
 /* Start the server for the first time */
   // https_server_handle= start_https_server();
    // Start HTTPS server if already connected
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
        ESP_LOGI(TAG, "Network already connected, starting HTTPS server...");
        set_led_state(LED_STATE_WEBSERVER_STARTING);

        https_server_handle = start_https_server();
        if (https_server_handle) {
            ESP_LOGI(TAG, "HTTPS server started successfully.");
            set_led_state(LED_STATE_WEBSERVER_RUNNING); // Update LED state here

        } else {
            ESP_LOGE(TAG, "Failed to start HTTPS server.");
            set_led_state(LED_STATE_FAILED); // Update LED state on failure

        }
    } else {
        ESP_LOGI(TAG, "Waiting for network connection to start HTTPS server...");
    }

  #ifdef TEST_HTTP_EVENT_DISCONNECT
    test_trigger_https_event(&https_server_handle);
  #endif

    // Start logging free heap memory
    start_heap_logging();
    ESP_LOGI(TAG, "Main function setup complete.");

}

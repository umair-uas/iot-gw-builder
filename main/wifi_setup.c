#include "wifi_setup.h"
#include "led_control.h"


// Event group bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "WIFI_SETUP";
// Event group for Wi-Fi events
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;


#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY


static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
         ESP_LOGI(TAG, "Connecting to AP...");
     
        esp_wifi_connect();
        set_led_state(LED_STATE_CONNECTING);  // Slow blinking

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP...");
            set_led_state(LED_STATE_CONNECTING);  // Slow blinking
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "Failed to connect to the AP.");
        set_led_state(LED_STATE_FAILED);  // Rapid blinking



    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        set_led_state(LED_STATE_CONNECTED);  // Steady green

    }
}

void wifi_init_sta(void) {
    ESP_LOGI(TAG, "Initializing Wi-Fi...");
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
     esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
      ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

wifi_config_t wifi_config = {
    .sta = {
        .ssid = CONFIG_ESP_WIFI_SSID,
        .password = CONFIG_ESP_WIFI_PASSWORD,
#if CONFIG_ESP_WIFI_AUTH_MODE_OPEN
        .threshold.authmode = WIFI_AUTH_OPEN,
#elif CONFIG_ESP_WIFI_AUTH_MODE_WEP
        .threshold.authmode = WIFI_AUTH_WEP,
#elif CONFIG_ESP_WIFI_AUTH_MODE_WPA_PSK
        .threshold.authmode = WIFI_AUTH_WPA_PSK,
#elif CONFIG_ESP_WIFI_AUTH_MODE_WPA2_PSK
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
#elif CONFIG_ESP_WIFI_AUTH_MODE_WPA_WPA2_PSK
        .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
#else
#error "Unsupported Wi-Fi auth mode!"
#endif
    },
};

  

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi initialization complete.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID: %s Password: %s",
                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID: %s Password: %s",
                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    } else {
        ESP_LOGE(TAG, "Unexpected event occurred.");
    }
     // Clean up the event group
    vEventGroupDelete(s_wifi_event_group);
}

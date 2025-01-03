#include "wifi_setup.h"
#include "led_control.h"

// Event group for Wi-Fi events
static EventGroupHandle_t s_wifi_event_group;
// Event group bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

static const char *TAG = "WIFI_SETUP";
static int s_retry_num = 0;

/* Update LED State */
static void wifi_update_led_state(led_state_t state) {
    ESP_LOGI(TAG, "Updating LED state to %d", state);
    set_led_state(state);
}

/*
static void got_ip_handler(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    xEventGroupClearBits(wifi_event_group, DISCONNECTED_BIT);
    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

   
    wifi_phy_mode_t phymode;
    wifi_config_t sta_cfg = { 0, };
    esp_wifi_get_config(WIFI_IF_STA, &sta_cfg);
    esp_wifi_sta_get_negotiated_phymode(&phymode);
    if (phymode == WIFI_PHY_MODE_HE20) {
        esp_err_t err = ESP_OK;
        wifi_itwt_setup_config_t setup_config = {
            .setup_cmd = TWT_REQUEST,
            .flow_id = 0,
            .twt_id = CONFIG_EXAMPLE_ITWT_ID,
            .flow_type = flow_type_announced ? 0 : 1,
            .min_wake_dura = CONFIG_EXAMPLE_ITWT_MIN_WAKE_DURA,
            .wake_duration_unit = CONFIG_EXAMPLE_ITWT_WAKE_DURATION_UNIT,
            .wake_invl_expn = CONFIG_EXAMPLE_ITWT_WAKE_INVL_EXPN,
            .wake_invl_mant = CONFIG_EXAMPLE_ITWT_WAKE_INVL_MANT,
            .trigger = trigger_enabled,
            .timeout_time_ms = CONFIG_EXAMPLE_ITWT_SETUP_TIMEOUT_TIME_MS,
        };
        err = esp_wifi_sta_itwt_setup(&setup_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "itwt setup failed, err:0x%x", err);
        }
    } else {
        ESP_LOGE(TAG, "Must be in 11ax mode to support itwt");
    }
}

static void start_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "sta connect to %s", DEFAULT_SSID);
    esp_wifi_connect();
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "sta disconnect, reconnect...");
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    esp_wifi_connect();
}
*/
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting to AP...");
        esp_wifi_connect();
        set_led_state(LED_STATE_CONNECTING); // Slow blinking (yellow)
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP... (%d/%d)", s_retry_num, CONFIG_ESP_MAXIMUM_RETRY);
            esp_wifi_connect();
            set_led_state(LED_STATE_CONNECTING); // Continue blinking (yellow)
        } else {
            ESP_LOGE(TAG, "Failed to connect after maximum retries.");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            set_led_state(LED_STATE_FAILED); // Blinking red
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi connected to AP, waiting for IP...");
        set_led_state(LED_STATE_CONNECTED_NO_IP); // Blinking cyan
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        set_led_state(LED_STATE_CONNECTED); // Steady green
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
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

/*
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    WIFI_EVENT_STA_START,
                    &start_handler,
                    NULL,
                    NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    WIFI_EVENT_STA_DISCONNECTED,
                    &disconnect_handler,
                    NULL,
                    NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &got_ip_handler,
                    NULL,
                    NULL));

                    */
  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = CONFIG_ESP_WIFI_SSID,
              .password = CONFIG_ESP_WIFI_PASSWORD,
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
          },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "Wi-Fi initialization complete.");

  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "Connected to AP SSID:%s ", CONFIG_ESP_WIFI_SSID);
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s ", CONFIG_ESP_WIFI_SSID);
  } else {
    ESP_LOGE(TAG, "Unexpected event occurred.");
  }
  // Clean up the event group
  vEventGroupDelete(s_wifi_event_group);
}

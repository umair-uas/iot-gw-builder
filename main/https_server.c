// https_server.c

#include "https_server.h"
#include "cJSON.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sys/param.h"

static const char *TAG = "HTTPS_SERVER";

// Embedded HTML, CSS, and JS files
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t status_html_start[] asm("_binary_status_html_start");
extern const uint8_t status_html_end[] asm("_binary_status_html_end");
extern const uint8_t clients_html_start[] asm("_binary_clients_html_start");
extern const uint8_t clients_html_end[] asm("_binary_clients_html_end");
extern const uint8_t settings_html_start[] asm("_binary_settings_html_start");
extern const uint8_t settings_html_end[] asm("_binary_settings_html_end");
extern const uint8_t styles_css_start[] asm("_binary_styles_css_start");
extern const uint8_t styles_css_end[] asm("_binary_styles_css_end");
extern const uint8_t app_js_start[] asm("_binary_app_js_start");
extern const uint8_t app_js_end[] asm("_binary_app_js_end");

// Embedded certificate and key files
extern const unsigned char cert_pem_start[] asm("_binary_cert_pem_start");
extern const unsigned char cert_pem_end[] asm("_binary_cert_pem_end");
extern const unsigned char key_pem_start[] asm("_binary_key_pem_start");
extern const unsigned char key_pem_end[] asm("_binary_key_pem_end");

// Helper function to serve embedded files
static esp_err_t serve_embedded_file(httpd_req_t *req,
                                     const unsigned char *start,
                                     const unsigned char *end,
                                     const char *content_type) {
  size_t file_len = end - start;
  httpd_resp_set_type(req, content_type);
  return httpd_resp_send(req, (const char *)start, file_len);
}

// URI Handlers
static esp_err_t handle_index(httpd_req_t *req) {
  ESP_LOGI(TAG, "Serving index.html");
  return serve_embedded_file(req, index_html_start, index_html_end,
                             "text/html");
}

static esp_err_t handle_status(httpd_req_t *req) {
  ESP_LOGI(TAG, "Serving status.html");
  return serve_embedded_file(req, status_html_start, status_html_end,
                             "text/html");
}
static esp_err_t handle_clients(httpd_req_t *req) {
  ESP_LOGI(TAG, "Serving clients.html");
  return serve_embedded_file(req, clients_html_start, clients_html_end,
                             "text/html");

}
  esp_err_t clients_handler(httpd_req_t * req) {
    const char *response =
        "{\"count\": 2, \"clients\": [{\"name\": \"Device1\", \"ip\": "
        "\"192.168.0.101\"}, {\"name\": \"Device2\", \"ip\": "
        "\"192.168.0.102\"}]}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }

  static esp_err_t handle_settings(httpd_req_t * req) {
    ESP_LOGI(TAG, "Serving settings.html");
    return serve_embedded_file(req, settings_html_start, settings_html_end,
                               "text/html");
  }

  static esp_err_t handle_css(httpd_req_t * req) {
    ESP_LOGI(TAG, "Serving styles.css");
    return serve_embedded_file(req, styles_css_start, styles_css_end,
                               "text/css");
  }

  static esp_err_t handle_js(httpd_req_t * req) {
    ESP_LOGI(TAG, "Serving app.js");
    return serve_embedded_file(req, app_js_start, app_js_end,
                               "application/javascript");
  }

  // Handler for System Info API
  esp_err_t system_info_handler(httpd_req_t * req) {
    // Retrieve chip info
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);

    // Collect system info
    char response[256];
    snprintf(response, sizeof(response),
             "{"
             "\"chip\":\"%s\","
             "\"cores\":%d,"
             "\"features\":\"%s%s%s%s\","
             "\"revision\":\"v%d.%d\","
             "\"flash_size\":\"%" PRIu32 "MB\","
             "\"heap_free\":%" PRIu32 "}",
             CONFIG_IDF_TARGET, chip_info.cores,
             (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
             (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
             (chip_info.features & CHIP_FEATURE_IEEE802154)
                 ? ", 802.15.4 (Zigbee/Thread)"
                 : "",
             chip_info.revision / 100, chip_info.revision % 100,
             (esp_flash_get_size(NULL, &flash_size) == ESP_OK)
                 ? flash_size / (1024 * 1024)
                 : 0,
             esp_get_free_heap_size());

    // Set response headers and send the JSON data
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }
  // Handler for Wi-Fi Status API
  esp_err_t wifi_status_handler(httpd_req_t * req) {
    wifi_ap_record_t ap_info;
    esp_wifi_sta_get_ap_info(&ap_info);

    char response[128];
    snprintf(response, sizeof(response),
             "{"
             "\"ssid\":\"%s\","
             "\"ip\":\"%s\","
             "\"rssi\":%d"
             "}",
             ap_info.ssid,
             "192.168.0.109", // Replace with dynamic IP retrieval
             ap_info.rssi);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
  }

  // Start HTTPS Server
  httpd_handle_t start_https_server(void) {
    httpd_handle_t server = NULL;

    httpd_ssl_config_t ssl_config = HTTPD_SSL_CONFIG_DEFAULT();
    ssl_config.servercert = cert_pem_start;
    ssl_config.servercert_len = cert_pem_end - cert_pem_start;
    ssl_config.prvtkey_pem = key_pem_start;
    ssl_config.prvtkey_len = key_pem_end - key_pem_start;
    ssl_config.port_secure = 443;

    // Optional: Enable keep-alive
    ssl_config.httpd.keep_alive_enable = true;
    ssl_config.httpd.keep_alive_idle = 5;     // Idle time in seconds
    ssl_config.httpd.keep_alive_interval = 2; // Interval in seconds
    ssl_config.httpd.keep_alive_count = 3;    // Max retries

    // Optional: Set maximum connections
    ssl_config.httpd.max_open_sockets = 4;

    // Optional: Set timeouts
    ssl_config.httpd.recv_wait_timeout = 5; // Seconds
    ssl_config.httpd.send_wait_timeout = 5; // Seconds
    ESP_LOGI(TAG, "Starting HTTPS server");
    esp_err_t ret = httpd_ssl_start(&server, &ssl_config);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to start HTTPS server: %s", esp_err_to_name(ret));
      return NULL;
    }

   httpd_uri_t uri_handlers[] = {
        {.uri = "/", .method = HTTP_GET, .handler = handle_index},
        {.uri = "/status", .method = HTTP_GET, .handler = handle_status},
        {.uri = "/clients", .method = HTTP_GET, .handler = handle_clients},
        {.uri = "/settings", .method = HTTP_GET, .handler = handle_settings},
        {.uri = "/css/styles.css", .method = HTTP_GET, .handler = handle_css},
        {.uri = "/js/app.js", .method = HTTP_GET, .handler = handle_js},
        {.uri = "/api/system_info", .method = HTTP_GET, .handler = system_info_handler},
        {.uri = "/api/wifi_status", .method = HTTP_GET, .handler = wifi_status_handler},
        {.uri = "/api/clients", .method = HTTP_GET, .handler = clients_handler},
    };
    for (int i = 0; i < sizeof(uri_handlers) / sizeof(uri_handlers[0]); i++) {
      httpd_register_uri_handler(server, &uri_handlers[i]);
    }

    ESP_LOGI(TAG, "HTTPS server started successfully");
    return server;
  }

  // Stop HTTPS Server
  void stop_https_server(httpd_handle_t server) {
    if (server) {
      httpd_ssl_stop(server);
      ESP_LOGI(TAG, "HTTPS server stopped");
    }
  }

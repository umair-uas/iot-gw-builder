// https_server.c

#include "https_server.h"
#include "led_control.h"
#include "esp_tls.h" 
#include "cJSON.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sys/param.h"
#include <inttypes.h>

static const char *TAG = "HTTPS_SERVER";

static void https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb);
static void handle_tls_error(esp_https_server_last_error_t *error);
static const char* get_tls_version_string(esp_tls_proto_ver_t version);

// Embedded HTML, CSS, and JS files
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t status_html_start[] asm("_binary_status_html_start");
extern const uint8_t status_html_end[] asm("_binary_status_html_end");
// Embedded HTML for LED Control
extern const uint8_t led_control_html_start[] asm("_binary_led_control_html_start");
extern const uint8_t led_control_html_end[] asm("_binary_led_control_html_end");
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
static esp_err_t httpd_resp_send_400(httpd_req_t *req) {
    httpd_resp_set_status(req, "400 Bad Request");
    return httpd_resp_send(req, "400 Bad Request", HTTPD_RESP_USE_STRLEN);
}
// URI Handlers
static esp_err_t handle_index(httpd_req_t *req) {
  ESP_LOGI(TAG, "Serving index.html");
  return serve_embedded_file(req, index_html_start, index_html_end,
                             "text/html");
}
static esp_err_t handle_led_control(httpd_req_t *req) {
    ESP_LOGI(TAG, "Serving led_control.html");
  return serve_embedded_file(req, led_control_html_start, led_control_html_end,
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
esp_err_t clients_handler(httpd_req_t *req) {
  const char *response =
      "{\"count\": 2, \"clients\": [{\"name\": \"Device1\", \"ip\": "
      "\"192.168.0.101\"}, {\"name\": \"Device2\", \"ip\": "
      "\"192.168.0.102\"}]}";
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t handle_settings(httpd_req_t *req) {
  ESP_LOGI(TAG, "Serving settings.html");
  return serve_embedded_file(req, settings_html_start, settings_html_end,
                             "text/html");
}

static esp_err_t handle_css(httpd_req_t *req) {
  ESP_LOGI(TAG, "Serving styles.css");
  return serve_embedded_file(req, styles_css_start, styles_css_end, "text/css");
}

static esp_err_t handle_js(httpd_req_t *req) {
  ESP_LOGI(TAG, "Serving app.js");
  return serve_embedded_file(req, app_js_start, app_js_end,
                             "application/javascript");
}

static esp_err_t example_uri_handler(httpd_req_t *req) {
    const char *uri = req->uri;
    if (strlen(uri) > CONFIG_HTTPD_MAX_URI_LEN) {
        ESP_LOGW(TAG, "URI exceeds maximum length: %zu", strlen(uri));
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "URI exceeds maximum allowed length.");
        return ESP_FAIL; // Let the HTTP server know the handler failed
    }

    // Normal processing of the request
    httpd_resp_sendstr(req, "Hello, URI is valid!");
    return ESP_OK;
}

// Handler to turn on the LED
static esp_err_t led_on_handler(httpd_req_t *req) {
    char buf[100];
    int ret;

    if ((ret = httpd_req_recv(req, buf, sizeof(buf))) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    buf[ret] = '\0';
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        httpd_resp_send_400(req);
        return ESP_FAIL;
    }

    cJSON *color_json = cJSON_GetObjectItem(json, "color");
    if (!cJSON_IsString(color_json)) {
        cJSON_Delete(json);
        httpd_resp_send_400(req);
        return ESP_FAIL;
    }

    const char *color = color_json->valuestring;
    ESP_LOGI(TAG, "Turning on LED with color %s", color);
    // Add code to set LED color based on the color value
    set_led_state(LED_STATE_ON);
    cJSON_Delete(json);
    httpd_resp_sendstr(req, "LED turned on");
    return ESP_OK;
}

// Handler to turn off the LED
static esp_err_t led_off_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Turning off LED");
    set_led_state(LED_STATE_OFF);
    httpd_resp_sendstr(req, "LED turned off");
    return ESP_OK;
}

// Handler to adjust LED brightness
static esp_err_t led_brightness_handler(httpd_req_t *req) {
    char buf[100];
    int ret, brightness;

    if ((ret = httpd_req_recv(req, buf, sizeof(buf))) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    buf[ret] = '\0';
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        httpd_resp_send_400(req);
        return ESP_FAIL;
    }

    cJSON *brightness_json = cJSON_GetObjectItem(json, "brightness");
    if (!cJSON_IsNumber(brightness_json)) {
        cJSON_Delete(json);
        httpd_resp_send_400(req);
        return ESP_FAIL;
    }

    brightness = brightness_json->valueint;
    ESP_LOGI(TAG, "Setting LED brightness to %d", brightness);
    set_brightness(brightness); // Corrected function call
    cJSON_Delete(json);
    httpd_resp_sendstr(req, "LED brightness set");
    return ESP_OK;
}

// Handler for System Info API
esp_err_t system_info_handler(httpd_req_t *req) {
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
esp_err_t wifi_status_handler(httpd_req_t *req) {
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

// SSL Configuration Function
httpd_ssl_config_t get_ssl_config(void) {
    httpd_ssl_config_t ssl_config = HTTPD_SSL_CONFIG_DEFAULT();
    ssl_config.servercert = cert_pem_start;
    ssl_config.servercert_len = cert_pem_end - cert_pem_start;
    ssl_config.prvtkey_pem = key_pem_start;
    ssl_config.prvtkey_len = key_pem_end - key_pem_start;
    ssl_config.port_secure = 443;
    ssl_config.transport_mode = HTTPD_SSL_TRANSPORT_SECURE;
    ssl_config.httpd.max_uri_handlers = 12;
    ssl_config.httpd.max_open_sockets = 6;
    ssl_config.httpd.recv_wait_timeout = 10;
    ssl_config.httpd.send_wait_timeout = 10;
    ssl_config.httpd.keep_alive_enable = true;
    ssl_config.httpd.keep_alive_idle = 3;     // Idle time in seconds
    ssl_config.httpd.keep_alive_interval = 2; // Interval in seconds
    ssl_config.httpd.keep_alive_count = 2;    // Max retries
     // Advanced Features
    ssl_config.session_tickets = true;  // Enable session tickets
    ssl_config.user_cb = https_server_user_callback;


    return ssl_config;
}
// Start HTTPS Server
httpd_handle_t start_https_server(void) {
  httpd_handle_t server = NULL;
  httpd_ssl_config_t ssl_config = get_ssl_config();

  ESP_LOGI(TAG, "Starting server on port: '%d'", ssl_config.port_secure);

  esp_err_t ret = httpd_ssl_start(&server, &ssl_config);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTPS server: %s", esp_err_to_name(ret));
    return NULL;
  }

 httpd_uri_t uri_handlers[] = {
        {.uri = "/", .method = HTTP_GET, .handler = handle_index},
        {.uri = "/index.html", .method = HTTP_GET, .handler = handle_index},
        {.uri = "/status.html", .method = HTTP_GET, .handler = handle_status},
        {.uri = "/clients.html", .method = HTTP_GET, .handler = handle_clients},
        {.uri = "/settings.html", .method = HTTP_GET, .handler = handle_settings},
        {.uri = "/led_control.html", .method = HTTP_GET, .handler = handle_led_control}, 
        {.uri = "/css/styles.css", .method = HTTP_GET, .handler = handle_css},
        {.uri = "/js/app.js", .method = HTTP_GET, .handler = handle_js},
        {.uri = "/api/system_info", .method = HTTP_GET, .handler = system_info_handler},
        {.uri = "/api/wifi_status", .method = HTTP_GET, .handler = wifi_status_handler},
        {.uri = "/api/clients", .method = HTTP_GET, .handler = clients_handler},
        {.uri = "/api/resource", .method = HTTP_GET, .handler = example_uri_handler},
        {.uri = "/api/led/on", .method = HTTP_POST, .handler = led_on_handler},
        {.uri = "/api/led/off", .method = HTTP_POST, .handler = led_off_handler},
        {.uri = "/api/led/brightness", .method = HTTP_POST, .handler = led_brightness_handler},
    };
  
  for (int i = 0; i < sizeof(uri_handlers) / sizeof(uri_handlers[0]); i++) {
    httpd_register_uri_handler(server, &uri_handlers[i]);
  }

  ESP_LOGI(TAG, "HTTPS server started successfully");
  return server;
}

void stop_https_server(httpd_handle_t server) {
  if (!server) {
    ESP_LOGW(TAG, "No server instance to stop.");
    return;
  }

  httpd_ssl_stop(server);
  ESP_LOGI(TAG, "HTTPS server stopped.");
}

void https_connect_handler(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data) {
    httpd_handle_t *server_handle = (httpd_handle_t *)arg;
    ESP_LOGI(TAG, "HTTPS connect handler invoked with event ID: %" PRIi32, event_id);

    if (*server_handle == NULL) {
        ESP_LOGI(TAG, "Starting HTTPS server.");
        *server_handle = start_https_server();
        if (*server_handle) {
            ESP_LOGI(TAG, "HTTPS server started successfully.");
            set_led_state(LED_STATE_WEBSERVER_RUNNING); // Update LED
        } else {
            ESP_LOGE(TAG, "Failed to start HTTPS server. Retrying...");
            set_led_state(LED_STATE_FAILED); // Indicate failure
            vTaskDelay(pdMS_TO_TICKS(2000)); // Wait 2 seconds
            *server_handle = start_https_server();
            if (*server_handle) {
                ESP_LOGI(TAG, "HTTPS server recovered successfully.");
                set_led_state(LED_STATE_WEBSERVER_RUNNING);
            } else {
                ESP_LOGE(TAG, "Retry failed. Server remains stopped.");
            }
        }
    } else {
        ESP_LOGW(TAG, "HTTPS server is already running.");
    }
}

void https_disconnect_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
    httpd_handle_t *server_handle = (httpd_handle_t *)arg;
    ESP_LOGI(TAG, "HTTPS disconnect handler invoked with event ID: %" PRIi32,
             event_id);

    if (*server_handle != NULL) {
        ESP_LOGI(TAG, "Stopping HTTPS server.");
        stop_https_server(*server_handle);
        *server_handle = NULL;
        ESP_LOGI(TAG, "HTTPS server stopped successfully.");
        set_led_state(LED_STATE_WEBSERVER_STOPPED); // Indicate server is stopped
    } else {
        ESP_LOGW(TAG, "HTTPS server is not running.");
        set_led_state(LED_STATE_WEBSERVER_STOPPED); // Ensure LED reflects stopped status
    }
}

void https_server_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == ESP_HTTPS_SERVER_EVENT) {
        switch (event_id) {
            case HTTPS_SERVER_EVENT_START:
                ESP_LOGI("HTTPS_SERVER", "HTTPS Server started.");
                set_led_state(LED_STATE_WEBSERVER_STARTING);
                break;

            case HTTPS_SERVER_EVENT_STOP:
                ESP_LOGI("HTTPS_SERVER", "HTTPS Server stopped.");
                set_led_state(LED_STATE_WEBSERVER_STOPPED);
                break;

            case HTTPS_SERVER_EVENT_ON_CONNECTED:
                ESP_LOGI("HTTPS_SERVER", "Client connected.");
                // Optionally blink the LED briefly to indicate a new connection
                set_led_state(LED_STATE_CLIENT_CONNECTED);
                break;

            case HTTPS_SERVER_EVENT_ON_DATA:
                ESP_LOGI("HTTPS_SERVER", "Data received.");
                // Add additional LED indications or processing logic if needed
                break;

            case HTTPS_SERVER_EVENT_SENT_DATA:
                ESP_LOGI("HTTPS_SERVER", "Data sent to client.");
                // You can add specific LED indication for data sent if necessary
                break;

            case HTTPS_SERVER_EVENT_DISCONNECTED:
                ESP_LOGI("HTTPS_SERVER", "Client disconnected.");
               set_led_state(LED_STATE_WEBSERVER_RUNNING); // Revert to running state
                break;

            case HTTPS_SERVER_EVENT_ERROR:
                esp_https_server_last_error_t *last_error = (esp_https_server_last_error_t *)event_data;
                ESP_LOGE("HTTPS_SERVER", "Error event triggered: last_error = %s, last_tls_err = %d, tls_flag = %d",
                         esp_err_to_name(last_error->last_error),
                         last_error->esp_tls_error_code,
                         last_error->esp_tls_flags);

                // Handle specific error cases
                if (last_error->last_error == ESP_ERR_MBEDTLS_SSL_HANDSHAKE_FAILED) {
                    ESP_LOGE("HTTPS_SERVER", "SSL handshake failed.");
                  //  set_led_state(LED_STATE_SSL_HANDSHAKE_FAILED); // Add a specific state if required
                } else {
                    // Update the LED state to indicate a general error
                    set_led_state(LED_STATE_FAILED);
                }
                break;

            default:
                ESP_LOGW("HTTPS_SERVER", "Unhandled HTTPS Server Event ID: %" PRIi32, event_id);

                break;
        }
    }
}

    void test_trigger_https_event(httpd_handle_t *server_handle) {
    ESP_LOGI("TEST", "Manually triggering IP_EVENT_STA_GOT_IP...");
    https_connect_handler(server_handle, IP_EVENT, IP_EVENT_STA_GOT_IP, NULL);

    vTaskDelay(pdMS_TO_TICKS(5000)); // Wait 5 seconds to simulate running state

    ESP_LOGI("TEST", "Manually triggering WIFI_EVENT_STA_DISCONNECTED...");
    https_disconnect_handler(server_handle, WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, NULL);
}
/* 
static const char* get_tls_version_string(esp_tls_proto_ver_t version) {
    switch (version) {
        case ESP_TLS_VER_TLS_1_2:
            return "TLS 1.2";
        case ESP_TLS_VER_TLS_1_3:
            return "TLS 1.3";
        default:
            return "Unknown TLS version";
    }
} */
static void print_peer_cert_info(const mbedtls_ssl_context *ssl)
{
    const mbedtls_x509_crt *cert;
    const size_t buf_size = 1024;
    char *buf = calloc(buf_size, sizeof(char));
    if (buf == NULL) {
        ESP_LOGE(TAG, "Out of memory - Callback execution failed!");
        return;
    }

    // Logging the peer certificate info
    cert = mbedtls_ssl_get_peer_cert(ssl);
    if (cert != NULL) {
        mbedtls_x509_crt_info((char *) buf, buf_size - 1, "    ", cert);
        ESP_LOGI(TAG, "Peer certificate info:\n%s", buf);
    } else {
        ESP_LOGW(TAG, "Could not obtain the peer certificate!");
    }

    free(buf);
}
static void https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb) {
    // Log the session creation or closure
     ESP_LOGI(TAG, "User callback invoked!");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
    mbedtls_ssl_context *ssl_ctx = NULL;
#endif
    switch(user_cb->user_cb_state) {
        case HTTPD_SSL_USER_CB_SESS_CREATE:
            ESP_LOGD(TAG, "At session creation");

            // Logging the socket FD
            int sockfd = -1;
            esp_err_t esp_ret;
            esp_ret = esp_tls_get_conn_sockfd(user_cb->tls, &sockfd);
            if (esp_ret != ESP_OK) {
                ESP_LOGE(TAG, "Error in obtaining the sockfd from tls context");
                break;
            }
            ESP_LOGI(TAG, "Socket FD: %d", sockfd);
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            // Logging the current ciphersuite
            ESP_LOGI(TAG, "Current Ciphersuite: %s", mbedtls_ssl_get_ciphersuite(ssl_ctx));
#endif
            break;

        case HTTPD_SSL_USER_CB_SESS_CLOSE:
            ESP_LOGD(TAG, "At session close");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
            // Logging the peer certificate
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            print_peer_cert_info(ssl_ctx);
#endif
            break;
        default:
            ESP_LOGE(TAG, "Illegal state!");
            return;
    }
}

// add function prototype to the top of the file
static void handle_tls_error(esp_https_server_last_error_t *error) {
  ESP_LOGE(TAG, "TLS Error: %s", esp_err_to_name(error->last_error));
  ESP_LOGE(TAG, "TLS Error Code: %d", error->esp_tls_error_code);
  ESP_LOGE(TAG, "TLS Flags: %d", error->esp_tls_flags);

  // Additional debugging actions

}

// https_server.c

#include "https_server.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sys/param.h"
#include "cJSON.h"

static const char *TAG = "HTTPS_SERVER";

// Embedded HTML files
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t status_html_start[] asm("_binary_status_html_start");
extern const uint8_t status_html_end[] asm("_binary_status_html_end");
extern const uint8_t clients_html_start[] asm("_binary_clients_html_start");
extern const uint8_t clients_html_end[] asm("_binary_clients_html_end");

/* Handler for serving the index.html */
static esp_err_t index_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Serving index.html");
    const size_t index_html_len = index_html_end - index_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_len);
    return ESP_OK;
}

/* Handler for serving the status.html */
static esp_err_t status_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Serving status.html");
    const size_t status_html_len = status_html_end - status_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)status_html_start, status_html_len);
    return ESP_OK;
}

/* Handler for serving the clients.html */
static esp_err_t clients_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Serving clients.html");
    const size_t clients_html_len = clients_html_end - clients_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)clients_html_start, clients_html_len);
    return ESP_OK;
}


/* Function to start the HTTPS server */
httpd_handle_t start_https_server(void) {
    httpd_handle_t server = NULL;
    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

    // Embedded certificate and key files
    extern const unsigned char cert_pem_start[] asm("_binary_cert_pem_start");
    extern const unsigned char cert_pem_end[] asm("_binary_cert_pem_end");
    extern const unsigned char key_pem_start[] asm("_binary_key_pem_start");
    extern const unsigned char key_pem_end[] asm("_binary_key_pem_end");

    conf.servercert = cert_pem_start;
    conf.servercert_len = cert_pem_end - cert_pem_start;
    conf.prvtkey_pem = key_pem_start;
    conf.prvtkey_len = key_pem_end - key_pem_start;

    ESP_LOGI(TAG, "Starting HTTPS server");
    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTPS server");
        return NULL;
    }

 // Register URI handlers
    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &index_uri);

    httpd_uri_t status_uri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &status_uri);

    httpd_uri_t clients_uri = {
        .uri = "/clients",
        .method = HTTP_GET,
        .handler = clients_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &clients_uri);

    ESP_LOGI(TAG, "HTTPS server started successfully");
    return server;

   }

/* Function to stop the HTTPS server */
void stop_https_server(httpd_handle_t server) {
    if (server) {
        httpd_ssl_stop(server);
    }
}

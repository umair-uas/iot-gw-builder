#ifndef HTTPS_SERVER_H
#define HTTPS_SERVER_H

#include "esp_https_server.h"
#include "esp_err.h"
#include "esp_event.h"
#include <inttypes.h> 

httpd_handle_t start_https_server(void);
void  stop_https_server(httpd_handle_t server);

void https_connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void https_disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void https_server_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void https_server_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void test_trigger_https_event(httpd_handle_t *server_handle);



#endif // HTTPS_SERVER_H

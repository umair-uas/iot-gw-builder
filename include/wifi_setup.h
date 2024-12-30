#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

// Wi-Fi Initialization
void wifi_init_sta(void);

#endif // WIFI_SETUP_H


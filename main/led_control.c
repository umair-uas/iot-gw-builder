#include "led_control.h"
#include "esp_log.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "LED_CONTROL";

static led_strip_handle_t led_strip;
static uint8_t brightness = 30; // Default brightness percentage (0-100)
static TaskHandle_t blinking_task_handle = NULL;
static SemaphoreHandle_t led_mutex;

// Scale brightness for colors
static uint8_t scale_brightness(uint8_t color) {
    return (color * brightness) / 100;
}

void configure_led() {
    ESP_LOGI(TAG, "Configuring LED strip...");
    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_BLINK_GPIO,
        .max_leds = 1, // Number of LEDs in the strip
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip); // Clear the strip
    led_mutex = xSemaphoreCreateMutex();
}
// Blinking task for specific states
static void blinking_task(void *state) {
    led_state_t led_state = (led_state_t)state;

    while (1) {
        uint8_t r = 0, g = 0, b = 0;

        if (led_state == LED_STATE_CONNECTING) {
            r = scale_brightness(255);
            g = scale_brightness(255);
            b = 0; // Yellow
            ESP_LOGI(TAG, "Blinking yellow...");
            vTaskDelay(pdMS_TO_TICKS(300)); // 300ms on
        } else if (led_state == LED_STATE_FAILED) {
            r = scale_brightness(255);
            g = 0;
            b = 0; // Red
            ESP_LOGI(TAG, "Blinking red...");
            vTaskDelay(pdMS_TO_TICKS(200)); // 200ms on
        } else if (led_state == LED_STATE_WEBSERVER_STARTING) {
            r = scale_brightness(128); // Purple (dim red + blue)
            g = 0;
            b = scale_brightness(255);
            ESP_LOGI(TAG, "Blinking purple (webserver starting)...");
            vTaskDelay(pdMS_TO_TICKS(500)); // 500ms on
        } else if (led_state == LED_STATE_CONNECTED_NO_IP) {
            r = 0;
            g = scale_brightness(255);
            b = scale_brightness(128); // Teal (green + dim blue)
            ESP_LOGI(TAG, "Blinking teal (connected, no IP)...");
            vTaskDelay(pdMS_TO_TICKS(400)); // 400ms on
        }

        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        vTaskDelay(pdMS_TO_TICKS(700)); // Off time
        ESP_ERROR_CHECK(led_strip_clear(led_strip));
    }
}
// Stop blinking task
void stop_blinking_task() {
    if (blinking_task_handle) {
        vTaskDelete(blinking_task_handle);
        blinking_task_handle = NULL;
    }
}


// Set LED state

void set_led_state(led_state_t state) {
    // Acquire the mutex to ensure thread safety
    xSemaphoreTake(led_mutex, portMAX_DELAY);

    // Stop any ongoing blinking task before changing state
    stop_blinking_task();

    uint8_t r = 0, g = 0, b = 0; // Initialize RGB colors to zero

    switch (state) {
    case LED_STATE_OFF:
            ESP_LOGI(TAG, "LED state: OFF");
            r = 0;
            g = 0;
            b = 0; // Off
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            break;

        case LED_STATE_ON:
            ESP_LOGI(TAG, "LED state: ON (steady white)");
            r = scale_brightness(255);
            g = scale_brightness(255);
            b = scale_brightness(255); // White
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            break;
    case LED_STATE_CONNECTING:
        ESP_LOGI(TAG, "LED state: CONNECTING (blinking yellow)");
        xTaskCreatePinnedToCore(blinking_task, "blinking_task", 2048, (void *)state,
                                5, &blinking_task_handle, 0);
        break;

    case LED_STATE_FAILED:
        ESP_LOGI(TAG, "LED state: FAILED (blinking red)");
        xTaskCreatePinnedToCore(blinking_task, "blinking_task", 2048, (void *)state,
                                5, &blinking_task_handle, 0);
        break;

    case LED_STATE_WEBSERVER_STARTING:
        ESP_LOGI(TAG, "LED state: WEBSERVER STARTING (blinking purple)");
        xTaskCreatePinnedToCore(blinking_task, "blinking_task", 2048, (void *)state,
                                5, &blinking_task_handle, 0);
        break;

    case LED_STATE_CONNECTED_NO_IP:
        ESP_LOGI(TAG, "LED state: CONNECTED (no IP) - blinking teal");
        xTaskCreatePinnedToCore(blinking_task, "blinking_task", 2048, (void *)state,
                                5, &blinking_task_handle, 0);
        break;

    case LED_STATE_CONNECTED:
        ESP_LOGI(TAG, "LED state: CONNECTED (steady green)");
        r = 0;
        g = scale_brightness(255);
        b = 0; // Green
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        break;
// add  case for LED_STATE_CLIENT_CONNECTED
    case LED_STATE_CLIENT_CONNECTED:
        ESP_LOGI(TAG, "LED state: CLIENT CONNECTED (steady cyan)");
        r = 0;
        g = scale_brightness(255);
        b = scale_brightness(255); // Cyan
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        break;
    case LED_STATE_WEBSERVER_RUNNING:
        ESP_LOGI(TAG, "LED state: WEBSERVER RUNNING (steady white)");
        r = scale_brightness(255);
        g = scale_brightness(255);
        b = scale_brightness(255); // White
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        break;

    case LED_STATE_WEBSERVER_STOPPED:
        ESP_LOGI(TAG, "LED state: WEBSERVER STOPPED (steady blue)");
        r = 0;
        g = 0;
        b = scale_brightness(255); // Blue
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        break;

    default:
        ESP_LOGW(TAG, "Unhandled LED state.");
        ESP_ERROR_CHECK(led_strip_clear(led_strip));
        break;
    }

    // Release the mutex after changing the state
    xSemaphoreGive(led_mutex);
}

// Set LED brightness
void set_brightness(uint8_t level) {
    if (level > 100)
        level = 100; // Clamp brightness to max 100%
    brightness = level;
    ESP_LOGI(TAG, "Brightness set to %d%%", brightness);
}

#include "led_control.h"
#include "esp_log.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "LED_CONTROL";

static led_strip_handle_t led_strip;
static uint8_t brightness = 25; // Default brightness percentage (0-100)
static SemaphoreHandle_t led_semaphore = NULL; // Semaphore for thread safety
static TaskHandle_t blinking_task_handle = NULL; // Task handle for blinking
static led_state_t current_led_state = LED_STATE_OFF; // Tracks the current LED state

// Scale brightness for colors
static uint8_t scale_brightness(uint8_t color) {
    return (color * brightness) / 100;
}

// Configure the LED strip
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

    // Initialize the semaphore
    led_semaphore = xSemaphoreCreateMutex();
    if (led_semaphore == NULL) {
        ESP_LOGE(TAG, "Failed to create LED semaphore!");
    }
}

// // Blinking task for specific states
// void blinking_task(void *state) {
//     led_state_t led_state = (led_state_t)state;

//     while (1) {
//         uint8_t r = 0, g = 0, b = 0;

//         if (led_state == LED_STATE_CONNECTING) {
//             r = scale_brightness(255);
//             g = scale_brightness(255);
//             b = 0; // Yellow
//             ESP_LOGI(TAG, "Blinking yellow...");
//             ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
//             ESP_ERROR_CHECK(led_strip_refresh(led_strip));
//             vTaskDelay(pdMS_TO_TICKS(300)); // 300ms on
//             ESP_ERROR_CHECK(led_strip_clear(led_strip));
//             vTaskDelay(pdMS_TO_TICKS(700)); // 700ms off
//         } else if (led_state == LED_STATE_FAILED) {
//             r = scale_brightness(255);
//             g = 0;
//             b = 0; // Red
//             ESP_LOGI(TAG, "Blinking red...");
//             ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
//             ESP_ERROR_CHECK(led_strip_refresh(led_strip));
//             vTaskDelay(pdMS_TO_TICKS(200)); // 200ms on
//             ESP_ERROR_CHECK(led_strip_clear(led_strip));
//             vTaskDelay(pdMS_TO_TICKS(800)); // 800ms off
//         }
//     }
// }
void blinking_task(void *state) {
    led_state_t led_state = (led_state_t)state;

    while (1) {
        uint8_t r = 0, g = 0, b = 0;

        if (led_state == LED_STATE_CONNECTING) {
            r = scale_brightness(255);
            g = scale_brightness(255);
            b = 0; // Yellow
            ESP_LOGI(TAG, "Blinking yellow...");
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            vTaskDelay(pdMS_TO_TICKS(300)); // 300ms on
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
            vTaskDelay(pdMS_TO_TICKS(700)); // 700ms off
        } else if (led_state == LED_STATE_FAILED) {
            r = scale_brightness(255);
            g = 0;
            b = 0; // Red
            ESP_LOGI(TAG, "Blinking red...");
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            vTaskDelay(pdMS_TO_TICKS(200)); // 200ms on
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
            vTaskDelay(pdMS_TO_TICKS(800)); // 800ms off
        } else if (led_state == LED_STATE_WEBSERVER_STARTING) {
            r = scale_brightness(255);
            g = scale_brightness(255);
            b = scale_brightness(255); // White
            ESP_LOGI(TAG, "Blinking white for HTTPS server starting...");
            for (int i = 0; i < 5; i++) { // Blink 5 times quickly
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
                ESP_ERROR_CHECK(led_strip_refresh(led_strip));
                vTaskDelay(pdMS_TO_TICKS(200)); // 200ms on
                ESP_ERROR_CHECK(led_strip_clear(led_strip));
                vTaskDelay(pdMS_TO_TICKS(200)); // 200ms off
            }
            break; // Exit blinking task after quick blinks
        } else {
            ESP_LOGW(TAG, "Unhandled LED state in blinking task.");
            break; // Exit the task for unhandled states
        }
    }

    vTaskDelete(NULL); // Clean up the task when done
}
// Stop the blinking task
void stop_blinking_task() {
    if (blinking_task_handle) {
        vTaskDelete(blinking_task_handle);
        blinking_task_handle = NULL;
    }
}

// Set the LED state
void set_led_state(led_state_t state) {
    if (led_semaphore == NULL) {
        ESP_LOGE(TAG, "LED semaphore not initialized!");
        return;
    }

    if (xSemaphoreTake(led_semaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (current_led_state == state) {
            ESP_LOGI(TAG, "LED state unchanged: %d", state);
            xSemaphoreGive(led_semaphore);
            return;
        }

        current_led_state = state; // Update the current state
        stop_blinking_task(); // Ensure any blinking is stopped before proceeding

        uint8_t r = 0, g = 0, b = 0;

        switch (state) {
        case LED_STATE_OFF:
            ESP_LOGI(TAG, "LED state: OFF");
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
            break;

        case LED_STATE_CONNECTING:
        case LED_STATE_FAILED:
            ESP_LOGI(TAG, "LED state: %s",
                     (state == LED_STATE_CONNECTING) ? "CONNECTING" : "FAILED");
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
            case LED_STATE_WEBSERVER_STARTING:
    ESP_LOGI(TAG, "LED state: WEBSERVER STARTING (quick blinking white)");
    xTaskCreatePinnedToCore(blinking_task, "blinking_task", 2048, (void *)state,
                            5, &blinking_task_handle, 0);
    break;


        default:
            ESP_LOGW(TAG, "Unhandled LED state.");
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
            break;
        }

        xSemaphoreGive(led_semaphore);
    } else {
        ESP_LOGW(TAG, "Failed to take LED semaphore. State change skipped.");
    }
}

// Set the brightness of the LED
void set_brightness(uint8_t level) {
    if (level > 100)
        level = 100; // Clamp brightness to max 100%
    brightness = level;
    ESP_LOGI(TAG, "Brightness set to %d%%", brightness);
}

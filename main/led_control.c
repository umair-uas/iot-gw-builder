#include "led_control.h"
#include "esp_log.h"
#include "led_strip.h"

static const char *TAG = "LED_CONTROL";

static led_strip_handle_t led_strip;
static uint8_t brightness = 50; // Default brightness percentage (0-100)

// Helper function to scale brightness
static uint8_t scale_brightness(uint8_t color) {
    return (color * brightness) / 100;
}

void configure_led() {
    ESP_LOGI(TAG, "Configuring LED strip...");
    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_BLINK_GPIO,
        .max_leds = 1, // Number of LEDs in the strip
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "Unsupported LED strip backend"
#endif
    led_strip_clear(led_strip); // Clear the strip
}

void set_led_state(led_state_t state) {
    uint8_t r = 0, g = 0, b = 0;
    switch (state) {
        case LED_STATE_OFF:
            ESP_LOGI(TAG, "LED state: OFF");
            break;
        case LED_STATE_CONNECTING:
            ESP_LOGI(TAG, "LED state: CONNECTING (yellow blinking)");
            r = scale_brightness(255);
            g = scale_brightness(255);
            b = 0;
            break;
        case LED_STATE_CONNECTED:
            ESP_LOGI(TAG, "LED state: CONNECTED (steady green)");
            r = 0;
            g = scale_brightness(255);
            b = 0;
            break;
        case LED_STATE_FAILED:
            ESP_LOGI(TAG, "LED state: FAILED (red blinking)");
            r = scale_brightness(255);
            g = 0;
            b = 0;
            break;
        default:
            ESP_LOGE(TAG, "Unknown LED state");
            return;
    }

    // Set the LED pixel with adjusted brightness
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

void set_brightness(uint8_t level) {
    if (level > 100) level = 100; // Limit brightness to 100%
    brightness = level;
    ESP_LOGI(TAG, "Brightness set to %d%%", brightness);
}

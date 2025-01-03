#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>

typedef enum {
    LED_STATE_OFF,
    LED_STATE_ON,
    LED_STATE_CONNECTING,
    LED_STATE_CONNECTED,
    LED_STATE_CONNECTED_NO_IP,
    LED_STATE_WEBSERVER_STARTING,
    LED_STATE_WEBSERVER_RUNNING,
    LED_STATE_WEBSERVER_STOPPED,
    LED_STATE_CLIENT_CONNECTED,
    LED_STATE_FAILED,
    LED_STATE_DATA_TRANSFER,  // Future state
    // Add more states as needed
} led_state_t;

void configure_led();
void set_led_state(led_state_t state);
void set_brightness(uint8_t level);

#endif // LED_CONTROL_H

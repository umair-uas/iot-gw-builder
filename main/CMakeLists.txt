idf_component_register(
    SRCS "main.c"
         "wifi_setup.c"
         "led_control.c"
         "https_server.c"
    INCLUDE_DIRS "../include"
    EMBED_TXTFILES "../certs/cert.pem"
                   "../certs/key.pem"
                   "../html/index.html"
                   "../html/status.html"
                   "../html/clients.html"
                    "../html/settings.html"
                    "../html/led_control.html"
                   "../html/css/styles.css"
                   "../html/js/app.js"
    REQUIRES esp_wifi esp_event nvs_flash driver led_strip esp_https_server json esp_timer
    PRIV_REQUIRES spi_flash
  
)


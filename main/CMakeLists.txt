idf_component_register(SRCS "main.c" "usb.c" "wifi.c" "gpio.c" "httpd.c" "led.c" "led_strip_encoder.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_wifi nvs_flash esp_http_server)

idf_component_register(SRCS "led_strip_encoder.c" "main.c" "led_strip_encoder.c"
                    REQUIRES esp_wifi esp_driver_gpio nvs_flash esp_driver_rmt
                    INCLUDE_DIRS ".")

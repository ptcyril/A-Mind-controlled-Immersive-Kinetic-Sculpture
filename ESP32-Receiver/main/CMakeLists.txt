idf_component_register(SRCS "main.c" "led_strip_encoder.c" 
                    REQUIRES nvs_flash esp_wifi esp_driver_rmt esp_driver_gpio esp_driver_ledc esp_timer
                    INCLUDE_DIRS ".")

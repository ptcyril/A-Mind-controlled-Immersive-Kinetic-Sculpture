idf_component_register(SRCS "main.c" "led_strip_encoder.c" "stepper_motor_encoder.c"
                    REQUIRES esp_wifi nvs_flash esp_driver_rmt esp_driver_gpio
                    INCLUDE_DIRS ".")

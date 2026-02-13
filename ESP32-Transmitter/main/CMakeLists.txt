idf_component_register(SRCS "main.c"
                    REQUIRES esp_wifi esp_driver_gpio nvs_flash esp_driver_gpio
                    INCLUDE_DIRS ".")

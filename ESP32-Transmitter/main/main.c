#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include <driver/gpio.h>

// MAC addresses
uint8_t esp2_mac[] = {0x3C, 0x8A, 0x1F, 0x7F, 0x35, 0x54};

void wifi_setup(){
    
    const wifi_init_config_t WICD = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&WICD));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(1,WIFI_SECOND_CHAN_NONE));

}

void app_main(void)
{

nvs_flash_init();
wifi_setup();

// Setup of ESPNOW
ESP_ERROR_CHECK(esp_now_init());

esp_now_peer_info_t esp2;
memset(&esp2, 0, sizeof(esp2));
memcpy(esp2.peer_addr, esp2_mac, 6);
esp2.channel = 1;
esp2.ifidx = WIFI_IF_STA;
esp2.encrypt = false;

ESP_ERROR_CHECK(esp_now_add_peer(&esp2));


// PIN 0 configuration
gpio_config_t pin0;
pin0.pin_bit_mask  = 1ULL << 4;
pin0.mode = GPIO_MODE_INPUT;
pin0.pull_up_en = GPIO_PULLUP_ENABLE;
pin0.pull_down_en = GPIO_PULLDOWN_DISABLE;
pin0.intr_type = GPIO_INTR_DISABLE;

ESP_ERROR_CHECK(gpio_config(&pin0));

// Sending Data
while(1){

    int level = gpio_get_level(4);
    if(level == 0){
        uint8_t x = 1;
        ESP_ERROR_CHECK(esp_now_send(esp2.peer_addr,&x, sizeof(x)));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    else{
        uint8_t x = 0;
        ESP_ERROR_CHECK(esp_now_send(esp2.peer_addr,&x, sizeof(x)));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    }
}
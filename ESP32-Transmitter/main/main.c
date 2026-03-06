#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "esp_log.h"
// ESP MAC addresses
const uint8_t esp_reciever_mac[] = {0x3C, 0x8A, 0x1F, 0x7F, 0x35, 0x54};

typedef struct{
    uint8_t led_device;
    uint8_t hue;
    uint8_t sat;
    uint8_t val;
} led_msg_t;

typedef struct{
    uint8_t motor_device;
    uint8_t speed;
    uint8_t direction;
    uint8_t reserved;
} motor_msg_t;

void wifi_setup(){
    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(1,WIFI_SECOND_CHAN_NONE));
}

void espnow_setup(const uint8_t *mac_address){
    uint8_t peer_mac[6];
    memcpy(peer_mac, mac_address, 6);

    ESP_ERROR_CHECK(esp_now_init());
    
    esp_now_peer_info_t esp;
    memset(&esp, 0, sizeof(esp));
    memcpy(esp.peer_addr, peer_mac, 6);
    esp.channel = 1;
    esp.ifidx = WIFI_IF_STA;
    esp.encrypt = false;

    ESP_ERROR_CHECK(esp_now_add_peer(&esp));
}

void app_main(void){

    nvs_flash_init();
    wifi_setup();
    espnow_setup(esp_reciever_mac);

    led_msg_t led_data;
    led_data.led_device = 0;
    led_data.hue = 0;
    led_data.sat = 100;
    led_data.val = 100;
    
    motor_msg_t motor_data;
    motor_data.motor_device = 1;
    motor_data.speed = 0;
    motor_data.direction = 0;
    motor_data.reserved =0;
    

    ESP_ERROR_CHECK(esp_now_send(esp_reciever_mac,(const uint8_t*)&led_data, sizeof(led_data)));
    ESP_LOGI("SENT","LED DATA");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(esp_now_send(esp_reciever_mac,(const uint8_t*)&motor_data, sizeof(motor_data)));
    ESP_LOGI("SENT","MOTOR DATA");
}
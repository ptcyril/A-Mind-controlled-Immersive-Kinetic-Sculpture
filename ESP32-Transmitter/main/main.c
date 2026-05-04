#include <stdint.h>
#include <string.h>

#include "espnow_protocol.h"

#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

// ESP MAC addresses
const uint8_t esp_reciever_mac[9][6] = {
    {0x3C, 0x8A, 0x1F, 0x76, 0xA3, 0xE8}, // ESP 1
    {0x3C, 0x8A, 0x1F, 0x7F, 0x35, 0x54}, // ESP 2
    {0x3C, 0x8A, 0x1F, 0x76, 0xDE, 0x44}, // ESP 3
    {0x5C, 0x01, 0x3B, 0x73, 0x6C, 0x0C}, // ESP 4
    {0x3C, 0x8A, 0x1F, 0x77, 0x8F, 0xB0}, // ESP 5
    {0x3C, 0x8A, 0x1F, 0xA0, 0xE5, 0x74}, // ESP 6
    {0x3C, 0x8A, 0x1F, 0x77, 0x8C, 0xAC}, // ESP 7
    {0x3C, 0x8A, 0x1F, 0x77, 0x2C, 0x84}, // ESP 8
    {0x3C, 0x8A, 0x1F, 0x7E, 0x34, 0xBC}, // ESP 9
};

void uart_setup(){
    const int uart_buffer_size = 512;

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, uart_buffer_size, 0, 0,
                                        NULL, 0));

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));
}

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
    uart_setup();
    wifi_setup();
    espnow_setup(esp_reciever_mac[6]);

    uint8_t device;
    led_msg_format_t led_msg;
    motor_msg_format_t motor_msg;

    while(true){
        int len = uart_read_bytes(UART_NUM_0, &device, sizeof(uint8_t), 
                                  pdMS_TO_TICKS(10));
        
        if(len == 1){     
            if(device == LED_CMD){
                led_msg.cmd_type = LED_CMD;
                uart_read_bytes(UART_NUM_0, ((uint8_t*)&led_msg)+1, 
                                sizeof(led_msg)-1, pdMS_TO_TICKS(10));
                ESP_ERROR_CHECK(esp_now_send(esp_reciever_mac[6], 
                                (const uint8_t*)&led_msg, sizeof(led_msg)));
            }
            else if(device == MOTOR_CMD){
                motor_msg.cmd_type = MOTOR_CMD;
                len = uart_read_bytes(UART_NUM_0, ((uint8_t*)&motor_msg)+1, 
                                sizeof(motor_msg)-1,pdMS_TO_TICKS(20));
                ESP_ERROR_CHECK(esp_now_send(esp_reciever_mac[6], 
                                (const uint8_t*)&motor_msg, sizeof(motor_msg)));
            }
        }
    }
}
#include <stdint.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

typedef enum{
    DEBUG,
    INFO,
    ERROR,
    WARN,
    SUCCESS
} log_level_t;

typedef enum{
    LED_DEVICE,
    MOTOR_DEVICE
} device_t;

// ESP MAC addresses
const uint8_t esp_reciever_mac[] = {0x3C, 0x8A, 0x1F, 0x7F, 0x35, 0x54};

typedef struct __attribute__((packed)){
    uint8_t target_device;
    uint8_t R_val;
    uint8_t G_val;
    uint8_t B_val;
} led_msg_format_t;

typedef struct __attribute__((packed)){
    uint8_t target_device;
    uint8_t motor_run_status;
    uint8_t dir;
    uint16_t delta_time;
    uint16_t delta_freq;
    uint32_t target_freq;
} motor_msg_format_t;

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

void gpio_setup(){
    gpio_config_t io_config = {
        .pin_bit_mask = 1ULL << 25,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en =  GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&io_config));
}

void app_main(void){    
    nvs_flash_init();
    uart_setup();
    wifi_setup();
    espnow_setup(esp_reciever_mac);
    gpio_setup();

    uint8_t device;
    led_msg_format_t led_msg;
    motor_msg_format_t motor_msg;

    while(true){
        int len = uart_read_bytes(UART_NUM_0, &device, sizeof(uint8_t), 
                                  pdMS_TO_TICKS(10));
        
        if(len == 1){     
            if(device == LED_DEVICE){
                led_msg.target_device = LED_DEVICE;
                uart_read_bytes(UART_NUM_0, ((uint8_t*)&led_msg)+1, 
                                sizeof(led_msg)-1, pdMS_TO_TICKS(10));
                ESP_ERROR_CHECK(esp_now_send(esp_reciever_mac, 
                                (const uint8_t*)&led_msg, sizeof(led_msg)));
                //ESP_LOGI("SENT", "Device=%d, p1=%d, p2=%d, p3=%d", led_msg.target_device, led_msg.R_val, led_msg.G_val, led_msg.B_val);
            }
            else if(device == MOTOR_DEVICE){
                motor_msg.target_device = MOTOR_DEVICE;
                len = uart_read_bytes(UART_NUM_0, ((uint8_t*)&motor_msg)+1, 
                                sizeof(motor_msg)-1,pdMS_TO_TICKS(20));
                ESP_ERROR_CHECK(esp_now_send(esp_reciever_mac, 
                                (const uint8_t*)&motor_msg, sizeof(motor_msg)));
                //ESP_LOGI("SENT", "Device=%d, p1=%d, p2=%u, p3=%u", motor_msg.target_device, motor_msg.dir, motor_msg.tot_step, motor_msg.freq);
            }
        }
    }
}
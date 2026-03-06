/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <stdint.h>
#include <string.h>

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "stepper_motor_encoder.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_rom_sys.h"

// LED Configuration
#define LED_NUMBERS 80
#define LED_SIGNAL_GPIO 12
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)

// Motor Configuration
#define MOTOR_DIR_GPIO 18
#define MOTOR_PUL_GPIO 25
#define RMT_MOTOR_RESOLUTION_HZ 1000000

// ESP MAC addresses
const uint8_t esp_transmitter_mac[] = {0x3C, 0x8A, 0x1F, 0x76, 0xA3, 0xE8};

// Message format
typedef struct{
    uint8_t target_device;
    uint8_t parameter1;
    uint8_t parameter2;
    uint8_t parameter3;
} msg_t;

static QueueHandle_t msg_queue;


static rmt_channel_handle_t tx_channel[2] = {NULL,NULL}; 

// LED
static led_strip_encoder_config_t encoder_config = {
    .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
};

static rmt_encoder_handle_t led_encoder = NULL;

static rmt_transmit_config_t tx_config = {0};

// MOTOR
static stepper_motor_curve_encoder_config_t accel_encoder_config = {
    .resolution = RMT_MOTOR_RESOLUTION_HZ,
    .sample_points = 500,
    .start_freq_hz = 500,
    .end_freq_hz = 1500,
};

static stepper_motor_uniform_encoder_config_t uniform_encoder_config = {
    .resolution = RMT_MOTOR_RESOLUTION_HZ,
};

static stepper_motor_curve_encoder_config_t decel_encoder_config = {
    .resolution = RMT_MOTOR_RESOLUTION_HZ,
    .sample_points = 500,
    .start_freq_hz = 1500,
    .end_freq_hz = 500,
};

static rmt_encoder_handle_t accel_motor_encoder = NULL;
static rmt_encoder_handle_t uniform_motor_encoder = NULL;
static rmt_encoder_handle_t decel_motor_encoder = NULL;


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

void received_data(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len){
    if (data_len != sizeof(msg_t)){
        ESP_LOGE("RECIEVED_DATA:", "DATA INVALID");
        return;
    }

    msg_t msg;
    memcpy(&msg, data, sizeof(msg_t));

    xQueueSendToBack(msg_queue, &msg,( TickType_t ) 0);
}

void rmt_setup(){ 
    int tx_gpio_number[2] = {LED_SIGNAL_GPIO,MOTOR_PUL_GPIO};
    uint32_t tx_resolution[2] = {RMT_LED_STRIP_RESOLUTION_HZ,RMT_MOTOR_RESOLUTION_HZ};
    int num_transactions[2] = {4,10};

    for(int i = 0;i<2;++i){
        ESP_LOGI("RMT_SETUP:", "Create RMT TX channel");
        rmt_tx_channel_config_t tx_chan_config = {  // *!*!*!* channel config TBD for MOTOR_PUL *!*!*!*
            .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
            .gpio_num = tx_gpio_number[i],
            .mem_block_symbols = 64, // increase the block size can make the LED less flickering
            .resolution_hz = tx_resolution[i],
            .trans_queue_depth = num_transactions[i], // set the number of transactions that can be pending in the background 10 for stepper motor
        };
        ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_channel[i]));
    }
    for(int i = 0;i<2;++i){
        ESP_LOGI("RMT_SETUP:", "Enable RMT TX channel");
        ESP_ERROR_CHECK(rmt_enable(tx_channel[i]));
    }
}

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b){
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void led_control(uint32_t hue,uint32_t sat,uint32_t val,rmt_channel_handle_t led_channel){
    static uint8_t led_strip_pixels[LED_NUMBERS * 3];

    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;

    led_strip_hsv2rgb(hue, sat, val, &red, &green, &blue);

    for (int i = 0; i < LED_NUMBERS; ++i) {
    // Build RGB pixels
    if(red>255) red=255;
    if(green>255) green=255;
    if(blue>255) blue=255;      
    led_strip_pixels[i * 3 + 0] = green;
    led_strip_pixels[i * 3 + 1] = blue;
    led_strip_pixels[i * 3 + 2] = red;
    }
    
    tx_config.loop_count =0;

    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(rmt_transmit(led_channel, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_channel, portMAX_DELAY));
}

void gpio_setup(){
    gpio_config_t io_config = {
        .pin_bit_mask = 1ULL << MOTOR_DIR_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en =  GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&io_config));
}

void pul_dir_control(int speed,int direction,rmt_channel_handle_t motor_channel){
    uint32_t accel_samples = 500;
    uint32_t uniform_speed_hz = 1500;
    uint32_t decel_samples = 500;

    if(accel_samples > 1024) accel_samples = 1024;  // Max per RMT channel
    if(uniform_speed_hz > 2000) uniform_speed_hz = 2000;
    if(decel_samples > 1024) decel_samples = 1024;

    gpio_set_level(MOTOR_DIR_GPIO,direction);
    esp_rom_delay_us(5);

    tx_config.loop_count = 0;
    ESP_ERROR_CHECK(rmt_transmit(motor_channel, accel_motor_encoder, &accel_samples, sizeof(accel_samples), &tx_config));

    tx_config.loop_count = 5000;
    ESP_ERROR_CHECK(rmt_transmit(motor_channel, uniform_motor_encoder, &uniform_speed_hz, sizeof(uniform_speed_hz), &tx_config));

    tx_config.loop_count = 0;
    ESP_ERROR_CHECK(rmt_transmit(motor_channel, decel_motor_encoder, &decel_samples, sizeof(decel_samples), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_channel, -1));
}

void app_main(void){
    typedef enum{
        LED_DEVICE = 0,
        MOTOR_DEVICE = 1
    } device_t;
    
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_setup();
    espnow_setup(esp_transmitter_mac);

    rmt_setup();
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&accel_encoder_config, &accel_motor_encoder));
    ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_encoder_config, &uniform_motor_encoder));
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&decel_encoder_config, &decel_motor_encoder));

    gpio_setup();

    //Recieveing data
    msg_queue = xQueueCreate(10, sizeof(msg_t));
    esp_now_register_recv_cb(received_data);

    msg_t msg;

    //Processing data
    while(true){
    xQueueReceive(msg_queue,&msg,portMAX_DELAY);

    if((device_t)msg.target_device == LED_DEVICE){
        ESP_LOGI("Recieved","LED DATA");
        led_control((uint32_t) msg.parameter1,(uint32_t) msg.parameter2,(uint32_t) msg.parameter3,tx_channel[LED_DEVICE]);
    }
    else if((device_t)msg.target_device == MOTOR_DEVICE){
        pul_dir_control((int) msg.parameter1,(int) msg.parameter2,tx_channel[MOTOR_DEVICE]);
        ESP_LOGI("Recieved","MOTOR DATA");
    }
    else
        ESP_LOGE("DEVICE_CTRL","TARGET DEVICE INVALID");
    }
}
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_rom_sys.h"

// LED Configuration
#define LED_NUMBERS 80
#define LED_SIGNAL_GPIO 12

#define LED_RESOLUTION_HZ 10000000

// Motor Configuration
#define MOTOR_PUL_GPIO 18 // BLUE wire
#define MOTOR_DIR_GPIO 25 // ORANGE wire

#define SAFEGUARD_LOW_FREQ 600
static int prev_direction = -1;

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
const uint8_t esp_transmitter_mac[] = {0x3C, 0x8A, 0x1F, 0x76, 0xA3, 0xE8};

// Message formats
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

static QueueHandle_t led_msg_queue, motor_msg_queue;

// LED
static rmt_channel_handle_t tx_channel = NULL; 

static led_strip_encoder_config_t encoder_config = {
    .resolution = LED_RESOLUTION_HZ,
};

static rmt_encoder_handle_t led_encoder = NULL;

static rmt_transmit_config_t tx_config = {0};

// MOTOR
ledc_channel_config_t ledc_channel = {
    .gpio_num       = MOTOR_PUL_GPIO,
    .speed_mode     = LEDC_HIGH_SPEED_MODE,
    .channel        = LEDC_CHANNEL_0,
    .timer_sel      = LEDC_TIMER_0,
    .duty           = 0, // Duty(how the signal is turned off)
    .hpoint         = 0
};

ledc_timer_config_t ledc_timer = {
    .speed_mode       = LEDC_HIGH_SPEED_MODE,
    .duty_resolution  = LEDC_TIMER_8_BIT,
    .timer_num        = LEDC_TIMER_0,
    .freq_hz          = SAFEGUARD_LOW_FREQ,
    .clk_cfg          = LEDC_USE_APB_CLK
};

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
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if(data_len == sizeof(led_msg_format_t)){
        led_msg_format_t led_msg;
        memcpy(&led_msg, data, sizeof(led_msg_format_t));

        xQueueSendToBackFromISR(led_msg_queue, &led_msg, &xHigherPriorityTaskWoken);
    }
    else if(data_len == sizeof(motor_msg_format_t)){
        motor_msg_format_t motor_msg;
        memcpy(&motor_msg, data, sizeof(motor_msg_format_t));

        xQueueSendToBackFromISR(motor_msg_queue, &motor_msg, &xHigherPriorityTaskWoken);
    }
    else{
        ESP_LOGE("RECEIVED_DATA:", "DATA INVALID,%d",data_len);
    }
    if(xHigherPriorityTaskWoken){
        portYIELD_FROM_ISR ();
    }     
}

void rmt_setup(){ 
    ESP_LOGI("RMT_SETUP:", "Create RMT TX channel");
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = LED_SIGNAL_GPIO,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = LED_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background 10 for stepper motor
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_channel));
    
    ESP_LOGI("RMT_SETUP:", "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(tx_channel));
}

void led_control(uint8_t red, uint8_t green, uint8_t blue, rmt_channel_handle_t led_channel){
    static uint8_t led_strip_pixels[LED_NUMBERS * 3];   

    for (int i = 0; i < LED_NUMBERS; ++i) {
    // Build RGB pixels
    led_strip_pixels[i * 3 + 0] = green;
    led_strip_pixels[i * 3 + 1] = red;
    led_strip_pixels[i * 3 + 2] = blue; 
    }
    
    tx_config.loop_count = 0;

    // Flush GRB values to LEDs and only returns after all have been flushed
    ESP_ERROR_CHECK(rmt_transmit(led_channel, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_channel, portMAX_DELAY));
}

void gpio_setup(){
    gpio_config_t gpio_dir_config = {
        .pin_bit_mask = 1ULL << MOTOR_DIR_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en =  GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_dir_config));
}

void motor_control(uint8_t motor_run_status, uint8_t new_direction, uint16_t delta_time, uint16_t delta_freq, uint32_t target_freq){ 
    // Flow chart provided in github titled ""
    ESP_LOGI("ENTER","MOTOR_CONTROL");
    uint32_t current_freq = ledc_get_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0);
    
    if(motor_run_status == 0){ // Turn off Motor condition
        ESP_LOGI("CONDITION","MOTOR STOPPED");
        while(current_freq>SAFEGUARD_LOW_FREQ){
            current_freq -= delta_freq;
            if(current_freq<SAFEGUARD_LOW_FREQ){
                current_freq = SAFEGUARD_LOW_FREQ;
            }
            ESP_ERROR_CHECK(ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0,current_freq));
            vTaskDelay(delta_time / portTICK_PERIOD_MS);
            ESP_LOGI("FREQ ","%d",current_freq);
        }
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0)); // Sets Duty to 0
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 Second Delay
        prev_direction = -1;
        return;
    }

    if(prev_direction == -1){ // Start up from rest condition
        prev_direction = new_direction; 
        gpio_set_level(MOTOR_DIR_GPIO, new_direction);
        esp_rom_delay_us(5);
    }
    else if(prev_direction != new_direction){ // Changing direction condition 
        while(current_freq>SAFEGUARD_LOW_FREQ){
            current_freq -= delta_freq;
            if(current_freq<SAFEGUARD_LOW_FREQ){
                current_freq = SAFEGUARD_LOW_FREQ;
            }
            ESP_ERROR_CHECK(ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0,current_freq));
            vTaskDelay(delta_time / portTICK_PERIOD_MS);
            ESP_LOGI("FREQ ","%d",current_freq);
        }
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0)); // Sets Duty to 0
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 Second Delay
        
        prev_direction = new_direction;
        gpio_set_level(MOTOR_DIR_GPIO, new_direction); // Set new direction
        esp_rom_delay_us(5);

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 128)); // Sets Duty to 50%
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
    }

    if(current_freq>target_freq){ // Decelerate from higher frequency condition
        while(current_freq>target_freq){
        current_freq -= delta_freq;
        if(current_freq<SAFEGUARD_LOW_FREQ){
            current_freq = SAFEGUARD_LOW_FREQ;
        }
        ESP_ERROR_CHECK(ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0,current_freq));
        vTaskDelay(delta_time / portTICK_PERIOD_MS);
        ESP_LOGI("FREQ ","%d",current_freq);
        }
    }

    while(current_freq<target_freq){ // Accelerate and hold frequency condition
        vTaskDelay(delta_time / portTICK_PERIOD_MS);
        current_freq += delta_freq;
        if(current_freq+delta_freq>=target_freq){
            current_freq = target_freq;
        }
        ESP_ERROR_CHECK(ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0,current_freq));
        ESP_LOGI("FREQ ","%d",current_freq);
    }
    ESP_LOGI("EXITED","WHILE LOOP");
}

void led_task(void *pvParameters){
    led_msg_format_t led_msg;
    while(true){
        BaseType_t led_received = xQueueReceive(led_msg_queue,&led_msg,portMAX_DELAY);
        if((device_t)led_msg.target_device == LED_DEVICE && led_received == pdTRUE){
            ESP_LOGI("LED Task","LED DATA");
            led_control(led_msg.R_val,led_msg.G_val,led_msg.B_val,tx_channel);
        }
    }
}

void motor_task(void *pvParameters){
    motor_msg_format_t motor_msg;
    while(true){
        ESP_LOGI("WAITING","MOTOR DATA");
        BaseType_t motor_received = xQueueReceive(motor_msg_queue,&motor_msg,portMAX_DELAY);
        if((device_t)motor_msg.target_device == MOTOR_DEVICE && motor_received == pdTRUE){
            ESP_LOGI("Motor Task","MOTOR DATA");
            if(prev_direction == -1){
                ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 128)); // Sets Duty to 50%
                ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
            }

            motor_control(motor_msg.motor_run_status, motor_msg.dir, motor_msg.delta_time, motor_msg.delta_freq, motor_msg.target_freq);
        }
    }
}

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_setup();
    espnow_setup(esp_transmitter_mac);

    rmt_setup();
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    gpio_setup();
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel)); // Sets PWM Duty Cycle to Zero, signal off
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    //Recieveing data
    led_msg_queue = xQueueCreate(10, sizeof(led_msg_format_t));
    motor_msg_queue = xQueueCreate(10, sizeof(motor_msg_format_t));
    esp_now_register_recv_cb(received_data);

    xTaskCreate(led_task, "LED", 2048, NULL, 1, NULL);
    xTaskCreate(motor_task, "MOTOR", 2048, NULL, 1, NULL);
}

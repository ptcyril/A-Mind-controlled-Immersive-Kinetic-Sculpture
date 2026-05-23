#include <stdint.h>
#include <string.h>
#include <math.h>

#include "espnow_protocol.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"
#include "esp_timer.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// LED Configuration
#define LED_NUMBERS 340
#define LED_SIGNAL_GPIO 25

// Motor Configuration
#define MOTOR_PUL_GPIO 33 // BLUE wire
#define MOTOR_DIR_GPIO 19 // ORANGE wire

#define HALL_EFFECT_SENSOR_GPIO 26

#define MICROSTEP 2000
#define SAFEGUARD_LOW_FREQ 400
#define NUM_MAGNETS 2

const motor_msg_format_t e_stop = {
    .mode = STOP,
    .accel_mag = 2500,
    .target_freq = SAFEGUARD_LOW_FREQ
};

static QueueHandle_t led_msg_queue, motor_msg_queue;

typedef struct{
    volatile int direction;
    volatile int max_rev;
    volatile int rev;
    volatile int num_rev;
    QueueHandle_t queue;
} motor_isr_t;

motor_isr_t isr_data = {
    .direction = CLOCKWISE,
    .max_rev = 22,
    .rev = 0,
    .num_rev = 3,
};

TaskHandle_t motor_stop_task_handle = NULL;

// LED
static rmt_channel_handle_t tx_channel = NULL; 

static led_strip_encoder_config_t encoder_config = {
    .resolution = 10000000,
};

static rmt_encoder_handle_t led_encoder = NULL;

static rmt_transmit_config_t tx_config = {0};

// MOTOR
ledc_channel_config_t ledc_channel = {
    .gpio_num       = MOTOR_PUL_GPIO,
    .speed_mode     = LEDC_HIGH_SPEED_MODE,
    .channel        = LEDC_CHANNEL_0,
    .timer_sel      = LEDC_TIMER_0,
    .duty           = 0, // Duty (how the signal is turned off)
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
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void rmt_setup(){ 
    ESP_LOGI("RMT_SETUP:", "Create RMT TX channel");
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = LED_SIGNAL_GPIO,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = 10000000,
        .trans_queue_depth = 4, 
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_channel));
    
    ESP_LOGI("RMT_SETUP:", "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(tx_channel));
}

void led_control(led_msg_format_t *msg){
    static uint8_t led_strip_pixels[LED_NUMBERS * 3]; // initialized to 0 on startup
    
    uint8_t initial_g = led_strip_pixels[0];
    uint8_t initial_r = led_strip_pixels[1];
    uint8_t initial_b = led_strip_pixels[2];

    int dg = msg->G_val - initial_g;
    int dr = msg->R_val - initial_r;
    int db = msg->B_val - initial_b;

    uint8_t g;
    uint8_t r;
    uint8_t b;

    // delta_step is between 0,60
    uint8_t steps = msg->delta_step > 0 ? msg->delta_step : 1; 

    for(int i = 0; i<=msg->delta_step; ++i){
        float t = (float) i / steps;
        t = t * t * (3.0f - 2.0f * t); // Smoothstep
        g = initial_g + (uint8_t)(dg*t + 0.5f);
        r = initial_r + (uint8_t)(dr*t + 0.5f);
        b = initial_b + (uint8_t)(db*t + 0.5f); 

        for(int j = 0; j < LED_NUMBERS; ++j) {
        // Build RGB pixels
        led_strip_pixels[j * 3 + 0] = g;
        led_strip_pixels[j * 3 + 1] = r;
        led_strip_pixels[j * 3 + 2] = b; 
        }

        // Flush GRB values to LEDs and only returns after all have been flushed
        tx_config.loop_count = 0;
        ESP_ERROR_CHECK(rmt_transmit(tx_channel, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(tx_channel, portMAX_DELAY));
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}

void led_task(void *pvParameters){
    led_msg_format_t led_msg;
    while(true){
        BaseType_t led_received = xQueueReceive(led_msg_queue,&led_msg,portMAX_DELAY);
        if(led_received == pdTRUE){
            ESP_LOGI("LED Task","LED DATA");
            led_control(&led_msg);
        }
    }
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

    gpio_config_t gpio_hall_effect_sensor_config = {
        .pin_bit_mask = 1ULL << HALL_EFFECT_SENSOR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en =  GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_hall_effect_sensor_config));  
}

void motor_task(void *pvParameters){
    enum MOTOR_STATE{
        CHANGE_DIR,
        IDLE,
        RAMP,
    };

    motor_msg_format_t msg = {
        .mode = STOP,
        .dir = CLOCKWISE,
        .target_freq = SAFEGUARD_LOW_FREQ,
        .num_rev = 0,
    };

    uint8_t motor_state = IDLE;
    uint8_t current_dir = msg.dir;

    float current_freq = SAFEGUARD_LOW_FREQ;

    TickType_t wait_time = 0;
    int64_t initial_time = esp_timer_get_time();

    float start_rev = 0.0f;
    float target_rev = 0.0f;

    while(true){        
        vTaskDelay(pdMS_TO_TICKS(10));

        float current_rev = (float)isr_data.rev / NUM_MAGNETS;
        ESP_LOGI("MOTOR","rev:%f", current_rev);
        if(xQueueReceive(motor_msg_queue, &msg, wait_time) == pdTRUE){
            ESP_LOGI("MOTOR","mode:%d, dir:%d, rev:%d, accel:%d, freq:%d", msg.mode, msg.dir, msg.num_rev, msg.accel_mag, msg.target_freq);
            
            gpio_set_level(MOTOR_DIR_GPIO, msg.dir);
            esp_rom_delay_us(5);

            start_rev = current_rev;

            if(msg.dir == CLOCKWISE){
                target_rev = start_rev + msg.num_rev;
            } else {
                target_rev = start_rev - msg.num_rev;
            }

            current_dir = msg.dir;
            isr_data.direction = msg.dir;

            motor_state = RAMP;
            wait_time = 0;
            initial_time = esp_timer_get_time();

            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 128);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
        }

        switch(motor_state){

            case IDLE:
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
                wait_time = portMAX_DELAY;
                break;

            case CHANGE_DIR:
                gpio_set_level(MOTOR_DIR_GPIO, msg.dir);
                esp_rom_delay_us(5);

                current_dir = msg.dir;
                isr_data.direction = msg.dir;
                motor_state = IDLE;
                break;

            case RAMP: {

                int64_t final_time = esp_timer_get_time();
                float dt = (final_time - initial_time) / 1e6f;
                initial_time = final_time;

                current_rev = (float)isr_data.rev / NUM_MAGNETS;

                float remaining_rev;

                if(current_dir == CLOCKWISE){
                    remaining_rev = target_rev - current_rev;
                } else {
                    remaining_rev = current_rev - target_rev;
                }

                if(remaining_rev < 0) remaining_rev = 0;

                if (remaining_rev <= 0.0f){
                    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
                    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
                    motor_state = IDLE;
                    break;
                }

                float d_stop = (current_freq * current_freq) /
                               (2.0f * msg.accel_mag * MICROSTEP);

                float max_freq = sqrtf(2.0f * msg.accel_mag * MICROSTEP * remaining_rev);

                if(max_freq < SAFEGUARD_LOW_FREQ){
                    max_freq = SAFEGUARD_LOW_FREQ;
                }

                float desired_freq = msg.target_freq;
                if(desired_freq > max_freq){
                    desired_freq = max_freq;
                }

                float target_freq;

                if(msg.mode == STOP || remaining_rev <= 0){
                    target_freq = SAFEGUARD_LOW_FREQ;
                }
                else if(d_stop >= remaining_rev){
                    // must brake
                    target_freq = SAFEGUARD_LOW_FREQ;
                }
                else{
                    target_freq = desired_freq;
                }

                float signed_accel = (current_freq < target_freq) ? 
                                      msg.accel_mag : -msg.accel_mag;

                current_freq += signed_accel * dt;

                // clamp overshoot
                if((signed_accel > 0 && current_freq > target_freq) ||
                   (signed_accel < 0 && current_freq < target_freq)){
                    current_freq = target_freq;
                }

                if(current_freq < SAFEGUARD_LOW_FREQ){
                    current_freq = SAFEGUARD_LOW_FREQ;
                }

                ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0,
                              (uint32_t)current_freq);

                if(current_freq == SAFEGUARD_LOW_FREQ && remaining_rev <= 0){
                    motor_state = IDLE;
                }
                break;
            }
        }
    }
}

void IRAM_ATTR rev_counter_isr(void* arg){
    motor_isr_t* data = (motor_isr_t*) arg;

    if(data->direction == CLOCKWISE){
        data->rev++;
    } 
    else {
        data->rev--;
    }
}

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    
    wifi_setup();
    ESP_ERROR_CHECK(esp_now_init());

    rmt_setup();
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    gpio_setup();
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(HALL_EFFECT_SENSOR_GPIO, rev_counter_isr, &isr_data));

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ESP_ERROR_CHECK(esp_timer_early_init());

    //Recieveing data
    led_msg_queue = xQueueCreate(1, sizeof(led_msg_format_t));
    motor_msg_queue = xQueueCreate(1, sizeof(motor_msg_format_t));
    isr_data.queue = motor_msg_queue;

    esp_now_register_recv_cb(received_data);

    xTaskCreate(led_task, "LED", 2048, NULL, 1, NULL);
    xTaskCreate(motor_task, "MOTOR", 2048, NULL, 1, NULL);
}
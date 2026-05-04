#pragma once

#include <stdint.h>

typedef uint8_t device_cmd_t;
#define SETTING_CMD 0
#define LED_CMD 1
#define MOTOR_CMD 2

typedef struct __attribute__((packed)){
    device_cmd_t cmd_type;
    uint8_t dst_esp;
} header_t;

typedef uint8_t log_level_t;
#define OFF 0
#define DEBUG 1
#define INFO 2
#define ERROR 3
#define WARN 4 
#define SUCCESS 5

typedef struct __attribute__((packed)){ // 4 bytes
    device_cmd_t cmd_type;
    log_level_t log_type;
    uint16_t max_motor_rev;
} setting_msg_format_t;

typedef struct __attribute__((packed)){ // 5 bytes
    device_cmd_t cmd_type;
    uint8_t R_val;
    uint8_t G_val;
    uint8_t B_val;
    uint8_t delta_step;
} led_msg_format_t;

typedef uint8_t motor_mode_t;
#define RUN 0
#define STOP 1

typedef uint8_t motor_dir_t;
#define CLOCKWISE 0
#define COUNTER_CLOCKWISE 1

typedef struct __attribute__((packed)){ // 9 bytes
    device_cmd_t cmd_type;
    motor_mode_t mode;
    motor_dir_t dir;
    uint16_t accel_mag;
    uint32_t target_freq;
} motor_msg_format_t;

typedef union {
    setting_msg_format_t setting;
    led_msg_format_t led;
    motor_msg_format_t motor;
} payload_t;

typedef struct __attribute__((packed)){
    uint8_t start_byte;
    uint8_t length;
    header_t header;
    payload_t payload;
} packet_t;
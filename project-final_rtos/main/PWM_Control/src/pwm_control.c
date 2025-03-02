#include "pwm_control.h"

void pwm_channel_init(const PWM_Config *config, const PWM_Channel *channel) {
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = config->mode,
        .duty_resolution  = config->duty_res,
        .timer_num        = config->timer,
        .freq_hz          = config->frequency,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = config->mode,
        .channel        = channel->channel,
        .timer_sel      = config->timer,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = channel->gpio_num,
        .duty           = 0,
        .hpoint         = 0,
        .flags.output_invert = config->invert
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void pwm_set_duty(const PWM_Channel *channel, uint32_t duty, ledc_mode_t mode) {
    ledc_set_duty(mode, channel->channel, duty);
    ledc_update_duty(mode, channel->channel);
}

void rgb_led_init(const RGB_LED *rgb_led) {
    pwm_channel_init(&rgb_led->config, &rgb_led->red);
    pwm_channel_init(&rgb_led->config, &rgb_led->green);
    pwm_channel_init(&rgb_led->config, &rgb_led->blue);
}

void rbg_set_duty(const RGB_LED *rgb_led, uint32_t red_duty, uint32_t green_duty, uint32_t blue_duty) {
    pwm_set_duty(&rgb_led->red, red_duty, rgb_led->config.mode);
    pwm_set_duty(&rgb_led->green, green_duty, rgb_led->config.mode);
    pwm_set_duty(&rgb_led->blue, blue_duty, rgb_led->config.mode);
}

void rgb_led_set_color(const RGB_LED *rgb_led, uint32_t red_value, uint32_t green_value, uint32_t blue_value) {
    uint32_t red_duty  = (red_value * ((1 << rgb_led->config.duty_res) - 1)) / 100;
    uint32_t green_duty = (green_value * ((1 << rgb_led->config.duty_res) - 1)) / 100;
    uint32_t blue_duty = (blue_value * ((1 << rgb_led->config.duty_res) - 1)) / 100;
    rgb_led_set_duty(rgb_led, red_duty, green_duty, blue_duty);
}
#include "pwm_control.h"

#define TAG "PWM_CONTROL"

#define SERVO_MIN_PULSE_US 500  // Pulso mínimo en microsegundos
#define SERVO_MAX_PULSE_US 2500  // Pulso máximo en microsegundos
#define SERVO_MAX_ANGLE    180   // Ángulo máximo (por ejemplo, 180°)
#define PWM_MAX_VALUE      4096   // Valor máximo para una resolución de 12 bits

void pwm_channel_init(const PWM_Config *config, const PWM_Channel *channel) {
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = config->mode,
        .duty_resolution  = config->duty_res,
        .timer_num        = config->timer,
        .freq_hz          = config->frequency,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = config->mode,
        .channel        = channel->channel,
        .timer_sel      = config->timer,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = channel->gpio_num,
        .duty           = 0,
        .hpoint         = 0,
        .flags.output_invert = config->invert
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ESP_LOGI(TAG, "Configuración de canal PWM en GPIO %d completada", channel->gpio_num);
}

void pwm_set_duty(const PWM_Channel *channel, uint32_t duty, ledc_mode_t mode) {
    ledc_set_duty(mode, channel->channel, duty);
    ledc_update_duty(mode, channel->channel);
}

void rgb_led_init(const RGB_LED *rgb_led) {
    pwm_channel_init(&rgb_led->config, &rgb_led->red);
    pwm_channel_init(&rgb_led->config, &rgb_led->green);
    pwm_channel_init(&rgb_led->config, &rgb_led->blue);
}

void rbg_set_duty(const RGB_LED *rgb_led, uint32_t red_duty, uint32_t green_duty, uint32_t blue_duty) {
    pwm_set_duty(&rgb_led->red, red_duty, rgb_led->config.mode);
    pwm_set_duty(&rgb_led->green, green_duty, rgb_led->config.mode);
    pwm_set_duty(&rgb_led->blue, blue_duty, rgb_led->config.mode);
}

void rgb_led_set_color(const RGB_LED *rgb_led, uint32_t red_value, uint32_t green_value, uint32_t blue_value) {
    uint32_t red_duty  = (red_value * ((1 << rgb_led->config.duty_res) - 1)) / 100;
    uint32_t green_duty = (green_value * ((1 << rgb_led->config.duty_res) - 1)) / 100;
    uint32_t blue_duty = (blue_value * ((1 << rgb_led->config.duty_res) - 1)) / 100;
    rgb_led_set_duty(rgb_led, red_duty, green_duty, blue_duty);
}

uint32_t angle_to_duty(uint32_t angle, ledc_timer_bit_t duty_res) {
    uint32_t pulse_us = SERVO_MIN_PULSE_US + 
                       ((SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * angle) / SERVO_MAX_ANGLE;
    uint32_t max_duty = (1 << duty_res) - 1; // Ej: 4095 para 12 bits
    uint32_t duty = (pulse_us * max_duty) / 20000;
    return duty;
}

void servo_init(const Servo *servo) {
    pwm_channel_init(&servo->config, &servo->channel);
    ESP_LOGI(TAG, "Servo inicializado en GPIO %d", servo->channel.gpio_num); // Corregir log
}

void servo_set_angle(const Servo *servo, uint32_t angle) {
    uint32_t duty = angle_to_duty(angle, servo->config.duty_res); // Usa la resolución del PWM_Config
    pwm_set_duty(&servo->channel, duty, servo->config.mode);
    ESP_LOGI(TAG, "Ángulo: %d, Ciclo de trabajo: %d", (int)angle, (int)duty);
}


/* Example of use:
PWM_Config servo_config = {
    .mode = LEDC_LOW_SPEED_MODE,
    .timer = LEDC_TIMER_0,
    .duty_res = LEDC_TIMER_12_BIT, // Resolución de 12 bits (0-4095)
    .frequency = 50,                // Frecuencia de 50Hz (20ms período)
    .invert = 0
    };

    Servo mi_servo = {
        .channel = { .gpio_num = 18, .channel = LEDC_CHANNEL_0 },
        .config = servo_config
    };
*/

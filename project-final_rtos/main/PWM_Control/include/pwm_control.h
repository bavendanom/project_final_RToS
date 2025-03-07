#ifndef RGB_LED_H
#define RGB_LED_H

#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"    




/**
 * @brief Configuración genérica para PWM.
 *
 * Estructura que define los parámetros básicos para configurar
 * un temporizador PWM, como el modo, la resolución del ciclo de trabajo
 * y la frecuencia.
 */
typedef struct PWM_Config {
    ledc_mode_t mode;          /**< Modo del PWM (alta o baja velocidad). */
    ledc_timer_t timer;        /**< Número del temporizador. */
    ledc_timer_bit_t duty_res; /**< Resolución del ciclo de trabajo en bits. */
    uint32_t frequency;        /**< Frecuencia del PWM en Hz. */
    int invert;                /**< Invertir la salida del PWM. */
} PWM_Config;

/**
 * @brief Configuración de un canal GPIO-PWM.
 *
 * Estructura que asocia un pin GPIO a un canal de PWM específico.
 */
typedef struct PWM_Channel {
    uint8_t gpio_num;          /**< Número del pin GPIO. */
    ledc_channel_t channel;    /**< Canal de PWM asignado. */
} PWM_Channel;

/**
 * @brief Configuración de un LED RGB.
 *
 * Estructura que define los tres canales de PWM (rojo, verde y azul)
 * necesarios para controlar un LED RGB, junto con su configuración PWM.
 */
typedef struct RGB_LED {
    PWM_Channel red;           /**< Canal PWM para el LED rojo. */
    PWM_Channel green;         /**< Canal PWM para el LED verde. */
    PWM_Channel blue;          /**< Canal PWM para el LED azul. */
    PWM_Config config;         /**< Configuración compartida del PWM. */
} RGB_LED;


extern const PWM_Config servo_config;


/**
 * @brief Inicializa un canal de PWM con la configuración dada.
 *
 * Esta función configura un canal PWM utilizando los parámetros del temporizador
 * y del canal asociados. Es necesario llamarla antes de usar el canal para establecer
 * ciclos de trabajo.
 *
 * @param[in] config Configuración del PWM.
 * @param[in] channel Canal del PWM que se configurará.
 */
void pwm_channel_init(const PWM_Config *config, const PWM_Channel *channel);

/**
 * @brief Configura el ciclo de trabajo de un canal de PWM.
 *
 * Ajusta el ciclo de trabajo (duty cycle) para un canal PWM específico.
 * El ciclo de trabajo se especifica en unidades absolutas, de acuerdo con
 * la resolución configurada en el temporizador PWM.
 *
 * @param[in] channel Canal PWM en el que se configurará el ciclo de trabajo.
 * @param[in] duty Valor del ciclo de trabajo (duty cycle).
 * @param[in] mode Modo del PWM (alta o baja velocidad).
 */
void pwm_set_duty(const PWM_Channel *channel, uint32_t duty, ledc_mode_t mode);

/**
 * @brief Inicializa un LED RGB con la configuración dada.
 *
 * Configura los tres canales de PWM necesarios para controlar un LED RGB,
 * utilizando los parámetros de configuración especificados en la estructura `RGB_LED`.
 *
 * @param[in] rgb_led Configuración del LED RGB.
 */
void rgb_led_init(const RGB_LED *rgb_led);

/**
 * @brief Ajusta los ciclos de trabajo para un LED RGB.
 *
 * Controla los colores del LED RGB ajustando los ciclos de trabajo
 * de los canales PWM correspondientes al rojo, verde y azul.
 *
 * @param[in] rgb_led Configuración del LED RGB.
 * @param[in] red_duty Ciclo de trabajo para el canal rojo.
 * @param[in] green_duty Ciclo de trabajo para el canal verde.
 * @param[in] blue_duty Ciclo de trabajo para el canal azul.
 */
void rgb_led_set_duty(const RGB_LED *rgb_led, uint32_t red_duty, uint32_t green_duty, uint32_t blue_duty);

/**
 * @brief Ajusta el color de un LED RGB de 0 a 100.
 *
 * ...
 *
 * @param[in] rgb_led Configuración del LED RGB.
 * @param[in] red_value Intensidad del color rojo (0-100).
 * @param[in] green_value Intensidad del color verde (0-100).
 * @param[in] blue_value Intensidad del color azul (0-100).
 */
void rgb_led_set_color(const RGB_LED *rgb_led, uint32_t red_value, uint32_t green_value, uint32_t blue_value);

/**
 * @brief Convierte un ángulo a un ciclo de trabajo PWM.
 *
 * Convierte un ángulo en grados a un valor de ciclo de trabajo PWM
 * para controlar un servo motor. El ciclo de trabajo se calcula en base
 * a los valores mínimo y máximo de pulso en microsegundos, y el periodo
 * del PWM en microsegundos.
 *
 * @param[in] angle Ángulo en grados (0-180).
 * @return Ciclo de trabajo PWM para el ángulo especificado.
 */

typedef struct Servo {
    PWM_Channel channel;  // Combina gpio_num y channel
    PWM_Config config;
} Servo;

extern const Servo mi_servo;

void servo_init(void);

/**
 * @brief Ajusta el ángulo de un servo motor.
 *
 * Ajusta el ángulo de un servo motor en base a un valor de ciclo de trabajo
 * PWM calculado a partir de un ángulo en grados. El ciclo de trabajo se
 * calcula en base a los valores mínimo y máximo de pulso en microsegundos,
 * y el periodo del PWM en microsegundos.
 *
 * @param[in] servo Configuración del servo motor.
 * @param[in] angle Ángulo en grados (0-180).
 */
void servo_set_angle(const Servo *servo, uint32_t angle);


/* Servo mi_servo = {
    .channel = { .gpio_num = 18, .channel = LEDC_CHANNEL_0 },
    .config = {
        .mode = LEDC_LOW_SPEED_MODE,
        .timer = LEDC_TIMER_0,
        .duty_res = LEDC_TIMER_12_BIT, // Resolución de 12 bits (0-4095)
        .frequency = 50,                // Frecuencia de 50Hz (20ms período)
        .invert = 0
        }
}; */

void servo_angle(uint32_t value_angle);

#endif // RGB_LED_H
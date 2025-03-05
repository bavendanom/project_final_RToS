#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "PWM_Control/include/pwm_control.h"

//include "NTC_library/include/NTC_library.h"
#include "ADC_library/include/ADC_library.h"
//include "led_RGB_LIBRARY/include/led_RGB_library.h"
#include "uart_library/include/uart_library.h"
#include "command_library/include/command_library.h"
#include "wifi_library/include/wifi_library.h"
#include "server_library/include/server_library.h"

#define ADC_CHANNEL ADC_CHANNEL_4 
#define ADC_UNIT ADC_UNIT_1



adc_config_t adc1_config_ch4 = {
    .channel = ADC_CHANNEL,
    .attenuation = ADC_ATTEN_DB_12,
    .cali_handle = NULL,
    .do_calibration = false
}; 


static void config_adc_unit(adc_config_t *acd_ch, adc_unit_t adc_unit ){
    config_unit init_adc_unit = init_adc(adc_unit);
    init_adc_ch(acd_ch, init_adc_unit);
}

void automatic_window_task(void *arg) {
    int adc_value;
    int state;

    ESP_LOGI("automatic_window_task", "Task started");

    config_adc_unit(&adc1_config_ch4, ADC_UNIT);

    while (1) {
        if (xQueueReceive(set_mode_manual_auto, &state, pdMS_TO_TICKS(1000)) == pdPASS) {
            char resp_str[64];
            snprintf(resp_str, sizeof(resp_str), "{\"Temperature\": %i}", state);
            sendData("Server:", resp_str);

            while (state == 0) {
                read_adc_raw(&adc1_config_ch4, &adc_value);
                ESP_LOGI("automatic_window_task", "ADC Value: %d", adc_value);
                uint32_t value_angle = (adc_value * 180) / 4095;
                servo_angle(value_angle);
                ESP_LOGI("automatic_window_task", "Servo Angle: %li", value_angle);
                if (xQueueReceive(set_mode_manual_auto, &state, pdMS_TO_TICKS(1000)) == pdPASS) {
                    state = 1;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}




static const char *TAG = "wifi station";
static void init_server(void){
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Configurar el pin GPIO para el LED
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);


    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_app_start();

    // Iniciar el servidor web
    start_webserver();
}
void app_main(void)
{   
    uart_init();
    queue_comandos_init();
    comandos_init_server();
    init_server();
    servo_init();
    //servo_init(&mi_servo);

    xTaskCreate(automatic_window_task, "automatic_window_task", 2048*6, NULL,5, NULL);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}

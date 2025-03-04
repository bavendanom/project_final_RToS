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
#include "ADC_library/include/ADC_library.h"
#include "uart_library/include/uart_library.h"
#include "command_library/include/command_library.h"
#include "wifi_library/include/wifi_library.h"
#include "server_library/include/server_library.h"


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
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}

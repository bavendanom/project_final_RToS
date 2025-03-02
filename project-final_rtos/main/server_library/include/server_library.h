#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "cJSON.h" 


#include "lwip/err.h"
#include "lwip/sys.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define LED_GPIO GPIO_NUM_2  // Define el pin GPIO donde está conectado el LED



// Definir la cola 
extern QueueHandle_t read_pot;
extern QueueHandle_t change_current_color;
extern QueueHandle_t rgb_crhomatic_circle_red_queue;
extern QueueHandle_t rgb_crhomatic_circle_green_queue;
extern QueueHandle_t rgb_crhomatic_circle_blue_queue; 
extern QueueHandle_t slider_crhomatic_circle_queue; 
extern QueueHandle_t rgb_time_on_queue;
extern QueueHandle_t rgb_time_off_queue;

void start_webserver(void);
void comandos_init_server(void);
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "cJSON.h" 




#include "lwip/err.h"
#include "lwip/sys.h"


#define OTA_UPDATE_PENDING 		0
#define OTA_UPDATE_SUCCESSFUL	1
#define OTA_UPDATE_FAILED		-1

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
extern QueueHandle_t set_mode_manual_auto;

void start_webserver(void);


/**
 * Messages for the HTTP monitor
 */
typedef enum http_server_message
{
	/* HTTP_MSG_WIFI_CONNECT_INIT = 0,
	HTTP_MSG_WIFI_CONNECT_SUCCESS,
	HTTP_MSG_WIFI_CONNECT_FAIL, */
	HTTP_MSG_OTA_UPDATE_SUCCESSFUL,
	HTTP_MSG_OTA_UPDATE_FAILED,
} http_server_message_e;

/**
 * Structure for the message queue
 */
typedef struct http_server_queue_message
{
	http_server_message_e msgID;
} http_server_queue_message_t;


/**
 * Sends a message to the queue
 * @param msgID message ID from the http_server_message_e enum.
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE.
 * @note Expand the parameter list based on your requirements e.g. how you've expanded the http_server_queue_message_t.
 */
BaseType_t http_server_monitor_send_message(http_server_message_e msgID);


/**
 * Timer callback function which calls esp_restart upon successful firmware update.
 */
void http_server_fw_update_reset_callback(void *arg);


void comandos_init_server(void);
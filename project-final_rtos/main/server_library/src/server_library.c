#include "server_library.h"
#include "command_library.h"
#include "uart_library.h"
#include "wifi_library.h"
#include "tasks_common.h"
#include "pwm_control.h"

#define HTTPD_MAX_URI_HANDLERS 16

static const char *TAG = "wifi station";

// Firmware update status
static int g_fw_update_status = OTA_UPDATE_PENDING;

// HTTP server monitor task handle
static TaskHandle_t task_http_server_monitor = NULL;

static uint8_t s_led_state = 0;

/**
 * ESP32 timer configuration passed to esp_timer_create.
 */
const esp_timer_create_args_t fw_update_reset_args = {
    .callback = &http_server_fw_update_reset_callback,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "fw_update_reset"
};
esp_timer_handle_t fw_update_reset;

QueueHandle_t read_pot;
QueueHandle_t change_current_color;
QueueHandle_t rgb_crhomatic_circle_red_queue;
QueueHandle_t rgb_crhomatic_circle_green_queue;
QueueHandle_t rgb_crhomatic_circle_blue_queue;
QueueHandle_t slider_crhomatic_circle_queue;
QueueHandle_t rgb_time_on_queue;
QueueHandle_t rgb_time_off_queue;

// Queue handle used to manipulate the main queue of events
static QueueHandle_t http_server_monitor_queue_handle;

esp_err_t post_handler(httpd_req_t *req) {
    // Reservar memoria para almacenar los datos recibidos
    char *buf = malloc(req->content_len + 1);
    if (!buf) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Leer los datos del cuerpo de la solicitud
    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) {
        free(buf);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    // Asegurar que la cadena esté terminada correctamente
    buf[req->content_len] = '\0';

    // Aquí procesas los datos recibidos en `buf`
    printf("Datos recibidos: %s\n", buf);

    // Enviar respuesta al cliente
    httpd_resp_send(req, "Datos recibidos correctamente", HTTPD_RESP_USE_STRLEN);

    // Liberar la memoria asignada
    free(buf);
    return ESP_OK;
}

//MARK: HANDLER TOOGLE LED
static esp_err_t http_server_toogle_led_handler(httpd_req_t *req)
{
	//ESP_LOGI(TAG, "/toogle_led.json requested");

	s_led_state = !s_led_state;
	gpio_set_level(LED_GPIO, s_led_state);

	// Cerrar la conexion
    httpd_resp_set_hdr(req, "Connection", "close");
    
    httpd_resp_send(req, "Cambio de estado", HTTPD_RESP_USE_STRLEN);
    
	return ESP_OK;
}


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//MARK: FIRMWARE UPDATE 
static void http_server_fw_update_reset_timer(void)
{
	if (g_fw_update_status == OTA_UPDATE_SUCCESSFUL)
	{
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW updated successful starting FW update reset timer");

		// Give the web page a chance to receive an acknowledge back and initialize the timer
		ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
		ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000));
	}
	else
	{
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW update unsuccessful");
	}
}

/**
 * HTTP server monitor task used to track events of the HTTP server
 * @param pvParameters parameter which can be passed to the task.
 */
static void http_server_monitor(void *parameter)
{
	http_server_queue_message_t msg;

	for (;;)
	{
		if (xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
		{
			switch (msg.msgID)
			{
				/* case HTTP_MSG_WIFI_CONNECT_INIT:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");
					
					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECTING;

					break; */

				/* case HTTP_MSG_WIFI_CONNECT_SUCCESS:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");

					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_SUCCESS;
					mqtt_app_start();
					break; */

				/* case HTTP_MSG_WIFI_CONNECT_FAIL:         
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAIL");

					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_FAILED;

					break; */

				case HTTP_MSG_OTA_UPDATE_SUCCESSFUL:
					ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_SUCCESSFUL");
					g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
					http_server_fw_update_reset_timer();
					break;

				case HTTP_MSG_OTA_UPDATE_FAILED:
					ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_FAILED");
					g_fw_update_status = OTA_UPDATE_FAILED;

					break;

				default:
					break;
			}
		}
	}
}

/**
 * Receives the .bin file fia the web page and handles the firmware update
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK, otherwise ESP_FAIL if timeout occurs and the update cannot be started.
 */
esp_err_t http_server_OTA_update_handler(httpd_req_t *req)
{
	esp_ota_handle_t ota_handle;

	char ota_buff[1024];
	int content_length = req->content_len;
	int content_received = 0;
	int recv_len;
	bool is_req_body_started = false;
	bool flash_successful = false;

	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

	do
	{
		// Read the data for the request
		if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0)
		{
			// Check if timeout occurred
			if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
			{
				ESP_LOGI(TAG, "http_server_OTA_update_handler: Socket Timeout");
				continue; ///> Retry receiving if timeout occurred
			}
			ESP_LOGI(TAG, "http_server_OTA_update_handler: OTA other Error %d", recv_len);
			return ESP_FAIL;
		}
		printf("http_server_OTA_update_handler: OTA RX: %d of %d\r", content_received, content_length);

		// Is this the first data we are receiving
		// If so, it will have the information in the header that we need.
		if (!is_req_body_started)
		{
			is_req_body_started = true;

			// Get the location of the .bin file content (remove the web form data)
			char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
			int body_part_len = recv_len - (body_start_p - ota_buff);

			printf("http_server_OTA_update_handler: OTA file size: %d\r\n", content_length);

			esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
			if (err != ESP_OK)
			{
				printf("http_server_OTA_update_handler: Error with OTA begin, cancelling OTA\r\n");
				return ESP_FAIL;
			}
			else
			{
				printf("http_server_OTA_update_handler: Writing to partition subtype %d at offset 0x%lx\r\n", update_partition->subtype, update_partition->address);
			}

			// Write this first part of the data
			esp_ota_write(ota_handle, body_start_p, body_part_len);
			content_received += body_part_len;
		}
		else
		{
			// Write OTA data
			esp_ota_write(ota_handle, ota_buff, recv_len);
			content_received += recv_len;
		}

	} while (recv_len > 0 && content_received < content_length);

	if (esp_ota_end(ota_handle) == ESP_OK)
	{
		// Lets update the partition
		if (esp_ota_set_boot_partition(update_partition) == ESP_OK)
		{
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
			ESP_LOGI(TAG, "http_server_OTA_update_handler: Next boot partition subtype %d at offset 0x%lx", boot_partition->subtype, boot_partition->address);
			flash_successful = true;
		}
		else
		{
			ESP_LOGI(TAG, "http_server_OTA_update_handler: FLASHED ERROR!!!");
		}
	}
	else
	{
		ESP_LOGI(TAG, "http_server_OTA_update_handler: esp_ota_end ERROR!!!");
	}

	// We won't update the global variables throughout the file, so send the message about the status
	if (flash_successful) { http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESSFUL); } else { http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED); }

	return ESP_OK;
}
/**
 * OTA status handler responds with the firmware update status after the OTA update is started
 * and responds with the compile time/date when the page is first requested
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
esp_err_t http_server_OTA_status_handler(httpd_req_t *req)
{
	char otaJSON[100];

	ESP_LOGI(TAG, "OTAstatus requested");

	sprintf(otaJSON, "{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", g_fw_update_status, __TIME__, __DATE__);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, otaJSON, strlen(otaJSON));

	return ESP_OK;
}





//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------


//MARK: HANDLER VALUES ADC
// Función para manejar la solicitud de obtener el valor de ADC
esp_err_t adc_handler(httpd_req_t *req) {
    int32_t adc_value;
    if (xQueueReceive(read_pot, &adc_value, pdMS_TO_TICKS(1000)) == pdPASS) {
        char resp_str[64];
        snprintf(resp_str, sizeof(resp_str), "{\"adc_value\": %li}", adc_value);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send_500(req);
    }
    return ESP_OK;
    // Cerrar la conexión
    httpd_resp_set_hdr(req, "Connection", "close");
}

//MARK: HANDLER TEMPERATURE ADC
// Función para manejar la solicitud de obtener el valor de ADC temperatura
esp_err_t adc_tem_handler(httpd_req_t *req) {
    float temp_Celsius;
    if (xQueueReceive(temp_queue, &temp_Celsius, pdMS_TO_TICKS(1000)) == pdPASS) {
        char resp_str[64];
        snprintf(resp_str, sizeof(resp_str), "{\"Temperature\": %.2f}", temp_Celsius);
        sendData("Server:", resp_str);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send_500(req);
    }
    return ESP_OK;
    // Cerrar la conexión
    httpd_resp_set_hdr(req, "Connection", "close");
}

//MARK: HANDLER SET COLORS RGB 
esp_err_t set_rgb_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;


    if (remaining >= sizeof(buf)) {
        ESP_LOGE(TAG, "Error: JSON demasiado grande");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }


    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }

    buf[req->content_len] = '\0';  // Asegurarse de que la cadena esté terminada en nulo

    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        ESP_LOGE(TAG, "Error al parsear JSON");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    const cJSON *color = cJSON_GetObjectItem(json, "color");
    const cJSON *min = cJSON_GetObjectItem(json, "min");
    const cJSON *max = cJSON_GetObjectItem(json, "max");

    if (cJSON_IsString(color) && cJSON_IsNumber(min) && cJSON_IsNumber(max)) {
        ESP_LOGI(TAG, "Received RGB settings - Color: %s, Min: %.2f, Max: %.2f", color->valuestring, min->valuedouble, max->valuedouble);
        
        //MARK:RED
        // ---------------- RED ---------------//
        if (strcmp(color->valuestring, "red") == 0) {
            float min_red = min->valuedouble;
            float max_red = max->valuedouble;
            ESP_LOGI(TAG,"se agrego a la cola");
            char mensaje[50];

        // Enviar el nuevo valor a la cola de MIN
        if (xQueueSend(min_red_queue, &min_red, portMAX_DELAY) == pdTRUE) {
            snprintf(mensaje, sizeof(mensaje), "MIN_RED  configurado en %f°C\n", min_red);
            sendData("CMD_HANDLER", mensaje);
        } else {
            sendData("CMD_HANDLER", "Error al enviar MIN_RED a la cola");
        }

        // Enviar el nuevo valor a la cola de MAX
        if (xQueueSend(max_red_queue, &max_red, portMAX_DELAY) == pdTRUE) {
            snprintf(mensaje, sizeof(mensaje), "MAX_RED  configurado en %f°C\n", max_red);
            sendData("CMD_HANDLER", mensaje);
        } else {
            sendData("CMD_HANDLER", "Error al enviar MAX_RED  a la cola");
        }
        //MARK:GREEN
        // ---------------- GREEN ---------------//
        } else if (strcmp(color->valuestring, "green") == 0) {
            float min_green = min->valuedouble;
            float max_green = max->valuedouble;
            ESP_LOGI(TAG,"se agrego a la cola");
            char mensaje[50];


            // Enviar el nuevo valor a la cola de MIN
        if (xQueueSend(min_green_queue, &min_green, portMAX_DELAY) == pdTRUE) {
            snprintf(mensaje, sizeof(mensaje), "MIN_GREEN  configurado en %f°C\n", min_green);
            sendData("CMD_HANDLER", mensaje);
        } else {
            sendData("CMD_HANDLER", "Error al enviar MIN_GREEN a la cola");
        }

        // Enviar el nuevo valor a la cola de MAX
        if (xQueueSend(max_green_queue, &max_green, portMAX_DELAY) == pdTRUE) {
            snprintf(mensaje, sizeof(mensaje), "MAX_GREEN  configurado en %f°C\n", max_green);
            sendData("CMD_HANDLER", mensaje);
        } else {
            sendData("CMD_HANDLER", "Error al enviar MAX_GREEN  a la cola");
        }

        //MARK:BLUE
        // ---------------- BLUE ---------------//
        } else if (strcmp(color->valuestring, "blue") == 0) {
            float min_blue = min->valuedouble;
            float max_blue = max->valuedouble;
            ESP_LOGI(TAG,"se agrego a la cola");
            char mensaje[50];


            // Enviar el nuevo valor a la cola de MIN
        if (xQueueSend(min_blue_queue, &min_blue, portMAX_DELAY) == pdTRUE) {
            snprintf(mensaje, sizeof(mensaje), "MIN_BLUE  configurado en %f°C\n", min_blue);
            sendData("CMD_HANDLER", mensaje);
        } else {
            sendData("CMD_HANDLER", "Error al enviar MIN_BLUE a la cola");
        }

        // Enviar el nuevo valor a la cola de MAX
        if (xQueueSend(max_blue_queue, &max_blue, portMAX_DELAY) == pdTRUE) {
            snprintf(mensaje, sizeof(mensaje), "MAX_BLUE  configurado en %f°C\n", max_blue);
            sendData("CMD_HANDLER", mensaje);
        } else {
            sendData("CMD_HANDLER", "Error al enviar MAX_BLUE  a la cola");
        }

        } else {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
    } else {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    cJSON_Delete(json);
    httpd_resp_send(req, "RGB settings updated", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

//MARK: HANDLER CHANGE COLOR READ ADC
static esp_err_t change_color_handler(httpd_req_t *req) {
   int current_color = 1;
   char mensaje[50];

    if (xQueueSend(change_current_color, &current_color, portMAX_DELAY) == pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "COLOR CHANGED %i\n", current_color);
        sendData("WED_SERVER_HANDLER", mensaje);
    } else {
        sendData("CMD_HANDLER", "Error al enviar MIN_BLUE a la cola");
    }

    // Cerrar la conexión
    httpd_resp_send(req, "COLOR CHANGE", HTTPD_RESP_USE_STRLEN);
    httpd_resp_set_hdr(req, "Connection", "close");
    return ESP_OK;
}
//MARK: HANDLER CONNECT WIFI HANDLER
esp_err_t connect_wifi_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;

    char* ssid_str = NULL;
    char* pass_str = NULL;

    // Leer el cuerpo de la solicitud
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf) - 1))) <= 0) {  // Asegurar espacio para el terminador nulo
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }

    buf[MIN(req->content_len, sizeof(buf) - 1)] = '\0';  // Asegurar que la cadena esté terminada en nulo

    // Parsear los datos JSON
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    const cJSON *ssid = cJSON_GetObjectItem(json, "ssid");
    const cJSON *pass = cJSON_GetObjectItem(json, "pass");

    if (cJSON_IsString(ssid) && (ssid->valuestring != NULL) && 
        cJSON_IsString(pass) && (pass->valuestring != NULL)) {

        ESP_LOGI(TAG, "Received WiFi settings - SSID: %s, Password: %s", ssid->valuestring, pass->valuestring);

        // Extraer SSID y contraseña del JSON
        ssid_str = strdup(ssid->valuestring);
        pass_str = strdup(pass->valuestring);

        if (ssid_str == NULL || pass_str == NULL) {
            httpd_resp_send_500(req);
            if (ssid_str) free(ssid_str);
            if (pass_str) free(pass_str);
            cJSON_Delete(json);
            return ESP_FAIL;
        }

        // Actualizar la configuración de Wi-Fi
        wifi_config_t* wifi_config_sta = wifi_app_get_wifi_config();
        memset(wifi_config_sta, 0x00, sizeof(wifi_config_t));
        strncpy((char *)wifi_config_sta->sta.ssid, ssid_str, sizeof(wifi_config_sta->sta.ssid) - 1);
        wifi_config_sta->sta.ssid[sizeof(wifi_config_sta->sta.ssid) - 1] = '\0';  // Asegurar terminación nula
        strncpy((char *)wifi_config_sta->sta.password, pass_str, sizeof(wifi_config_sta->sta.password) - 1);
        wifi_config_sta->sta.password[sizeof(wifi_config_sta->sta.password) - 1] = '\0';  // Asegurar terminación nula

        // Guardar credenciales y reconectar
        save_wifi_credentials(ssid_str, pass_str);
        esp_wifi_disconnect();
        connect_to_wifi();

        // Liberar memoria asignada con strdup
        free(ssid_str);
        free(pass_str);

        // Enviar una respuesta exitosa
        httpd_resp_send(req, "WiFi settings received", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send_500(req);
    }

    cJSON_Delete(json);
    return ESP_OK;
}

//MARK: HANDLER RGB CHROMATIC CIRCLE
esp_err_t rgb_crhomatic_circle_handler(httpd_req_t *req) {

    char content[100];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    content[ret] = '\0';
    char mensaje[50];

    int red, green, blue; 


    sscanf(content, "{\"red\":%d,\"green\":%d,\"blue\":%d}", &red, &green, &blue);
    printf("Received RGB data: Red=%d, Green=%d, Blue=%d\n", red, green, blue);

    //MARK: RED
    if (xQueueSend(rgb_crhomatic_circle_red_queue, &red, pdMS_TO_TICKS(100)) == pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "VALUE_RED  configurado en %i\n", red);
        sendData("CRHOMATIC CIRCLE", mensaje);
    } else {
        sendData("CRHOMATIC CIRCLE", "Error al enviar VALUE_RED a la cola");
    }
    //MARK: GREEN
    if (xQueueSend(rgb_crhomatic_circle_green_queue, &green, pdMS_TO_TICKS(100)) == pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "VALUE_GREEN  configurado en %i\n", green);
        sendData("CRHOMATIC CIRCLE", mensaje);
    } else {
        sendData("CRHOMATIC CIRCLE", "Error al enviar VALUE_GREEN a la cola");
    }
    //MARK: BLUE
    if (xQueueSend(rgb_crhomatic_circle_blue_queue, &blue, pdMS_TO_TICKS(100)) == pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "VALUE_BLUE  configurado en %i\n", blue);
        sendData("CRHOMATIC CIRCLE", mensaje);
    } else {
        sendData("CRHOMATIC CIRCLE", "Error al enviar VALUE_BLUE  a la cola");
    }

    // Cerrar la conexión
    httpd_resp_send(req, "COLOR CHANGE", HTTPD_RESP_USE_STRLEN);
    httpd_resp_set_hdr(req, "Connection", "close");
    
    return ESP_OK;
} 

//MARK: HANDLER SLIDER CHROMATIC CIRCLE
esp_err_t slider_crhomatic_circle_handler(httpd_req_t *req) {
    char content[100];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);

    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    content[ret] = '\0'; // Asegurar que el contenido esté terminado en null

    // Depuración: Imprimir el contenido recibido
    printf("Contenido recibido: %s\n", content);

    // Analizar el JSON recibido
    cJSON *root = cJSON_Parse(content);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error antes: %s\n", error_ptr);
        }
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Extraer el valor de "value" del JSON
    cJSON *value_json = cJSON_GetObjectItemCaseSensitive(root, "value");
    if (!cJSON_IsNumber(value_json)) {
        printf("El campo 'value' no es un número o no existe.\n");
        cJSON_Delete(root);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int brightness = value_json->valueint; // Obtener el valor como entero
    printf("Valor recibido: %d\n", brightness);

    // Enviar el valor a la cola
    if (xQueueSend(slider_crhomatic_circle_queue, &brightness, pdMS_TO_TICKS(100)) == pdTRUE) {
        char mensaje[50];
        snprintf(mensaje, sizeof(mensaje), "VALUE_BRIGTHNESS configurado en %d\n", brightness);
        sendData("SLIDER CRHOMATIC CIRCLE", mensaje);
    } else {
        sendData("SLIDER CRHOMATIC CIRCLE", "Error al enviar VALUE_BRIGTHNESS a la cola");
    }

    // Liberar la memoria del JSON
    cJSON_Delete(root);

    // Responder al cliente
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "Valor recibido correctamente", HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}


//MARK: TIME ON OFF
esp_err_t set_time_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;


    if (remaining >= sizeof(buf)) {
        ESP_LOGE(TAG, "Error: JSON demasiado grande");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }


    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }

    buf[req->content_len] = '\0';  // Asegurarse de que la cadena esté terminada en nulo

    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        ESP_LOGE(TAG, "Error al parsear JSON");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    
    const cJSON *min = cJSON_GetObjectItem(json, "min");

    if ( cJSON_IsNumber(min) ) {
        ESP_LOGI(TAG, "Received RGB settings Min: %.2f", min->valuedouble);
            uint32_t min_time = min->valuedouble;
            servo_angle(min_time);
            char mensaje[50];
            snprintf(mensaje, sizeof(mensaje), "VALUE_ANGLEconfigurado en %li\n", min_time);
            sendData("ANGLE", mensaje);
            // Aquí procesas los datos recibidos en `buf`
            printf("Datos recibidos: %s\n", buf);

            // Enviar respuesta al cliente
            httpd_resp_send(req, "Datos recibidos correctamente", HTTPD_RESP_USE_STRLEN);
    }


    cJSON_Delete(json);
    httpd_resp_send(req, "RGB settings updated", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
    
}

//MARK: REGISTROS ///////////////////////////
/**
 * regchange.json handler is invoked after the "enviar registro" button is pressed
 * and handles receiving the data entered by the user
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */


 static esp_err_t http_server_register_change_handler(httpd_req_t *req)
 {
     size_t header_len;
     char* header_value;
     char* hour_str = NULL;
     char* reg_str = NULL;
     char* min_str = NULL;
     char* days = NULL;
     int content_length;
 
     ESP_LOGI(TAG, "/regchange.json requested");
 
     // Get the "Content-Length" header to determine the length of the request body
     header_len = httpd_req_get_hdr_value_len(req, "Content-Length");
     if (header_len <= 0) {
         // Content-Length header not found or invalid
         //httpd_resp_send_err(req, HTTP_STATUS_411_LENGTH_REQUIRED, "Content-Length header is missing or invalid");
         ESP_LOGI(TAG, "Content-Length header is missing or invalid");
         return ESP_FAIL;
     }
 
     // Allocate memory to store the header value
     header_value = (char*)malloc(header_len + 1);
     if (httpd_req_get_hdr_value_str(req, "Content-Length", header_value, header_len + 1) != ESP_OK) {
         // Failed to get Content-Length header value
         free(header_value);
         //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Failed to get Content-Length header value");
         ESP_LOGI(TAG, "Failed to get Content-Length header value");
         return ESP_FAIL;
     }
 
     // Convert the Content-Length header value to an integer
     content_length = atoi(header_value);
     free(header_value);
 
     if (content_length <= 0) {
         // Content length is not a valid positive integer
         //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid Content-Length value");
         ESP_LOGI(TAG, "Invalid Content-Length value");
         return ESP_FAIL;
     }
 
     // Allocate memory for the data buffer based on the content length
     char* data_buffer = (char*)malloc(content_length + 1);
 
     // Read the request body into the data buffer
     if (httpd_req_recv(req, data_buffer, content_length) <= 0) {
         // Handle error while receiving data
         free(data_buffer);
         //httpd_resp_send_err(req, HTTP_STATUS_INTERNAL_SERVER_ERROR, "Failed to receive request body");
         ESP_LOGI(TAG, "Failed to receive request body");
         return ESP_FAIL;
     }
 
     // Null-terminate the data buffer to treat it as a string
     data_buffer[content_length] = '\0';
 
     // Parse the received JSON data
     cJSON* root = cJSON_Parse(data_buffer);
     free(data_buffer);
 
     if (root == NULL) {
         // JSON parsing error
         //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid JSON data");
         ESP_LOGI(TAG, "Invalid JSON data");
         return ESP_FAIL;
     }
 
     cJSON* reg_number_json = cJSON_GetObjectItem(root, "selectedNumber");
     cJSON* hour_json = cJSON_GetObjectItem(root, "hours");
     cJSON* min_json = cJSON_GetObjectItem(root, "minutes");
     cJSON* selectedDays_json = cJSON_GetObjectItem(root, "selectedDays");
     
     if (hour_json == NULL || min_json == NULL || selectedDays_json == NULL|| !cJSON_IsString(hour_json) || !cJSON_IsString(min_json) || !cJSON_IsArray(selectedDays_json)) {
         cJSON_Delete(root);
         // Missing or invalid JSON fields
         //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Missing or invalid JSON data fields");
         ESP_LOGI(TAG, "Missing or invalid JSON data fields");
         return ESP_FAIL;
     }
 
     // Extract SSID and password from JSON
     reg_str = strdup(reg_number_json->valuestring);
     hour_str = strdup(hour_json->valuestring);
     min_str = strdup(min_json->valuestring);
 
     
 
     ESP_LOGI(TAG, "Received reg: %s", reg_str);
     ESP_LOGI(TAG, "Received hour: %s", hour_str);
     ESP_LOGI(TAG, "Received min: %s", min_str);
  
     
     char str_to_save[12];
     //str_to_save[11]=0x00;
     memset(str_to_save, 0x00, 12);
     
     if (cJSON_IsArray(selectedDays_json)) {
     cJSON* day_item;
 
     // Iterate over each element in the array
     
     
     //strcat(str_to_save, reg_str);
     strcat(str_to_save, hour_str);
     strcat(str_to_save, min_str);
     cJSON_ArrayForEach(day_item, selectedDays_json) {
         // Check if the array element is a string
         if (cJSON_IsString(day_item)) {
             const char* day_str = day_item->valuestring;
             strcat(str_to_save, day_str);
             // Perform actions with the day_str
             printf("Selected Day: %s\n", day_str);
         }
     }
     
     } else {
         printf("SelectedDays is not an array\n");
     }
     printf("%s\n", str_to_save);
     save_reg_data(atoi(reg_str), str_to_save);
     update_register(atoi(reg_str));
     // Process the selected days array
    // cJSON* day_item;
     //cJSON_ArrayForEach(day_item, selectedDays_json) {
        // if (cJSON_IsString(day_item)) {
          //   const char* day_str = day_item->valuestring;
            // ESP_LOGI(TAG, "Selected Day: %s", day_str);
 
             // Perform any additional actions based on the selected day
             // ...
 
             // Release memory when no longer needed
         //}
         
     //}
     //free(day_item);
 
     //free(selectedDays_json);
 
     // Send a success response to the client
     // Cerrar la conexion
     free(reg_str);
     free(hour_str);
     free(min_str);
     cJSON_Delete(root);
     httpd_resp_set_hdr(req, "Connection", "close");
     httpd_resp_send(req, NULL, 0);
     
 
     return ESP_OK;
 }
 
 
 
 
 /**
  * erasereg.json handler is invoked after the "enviar registro" button is pressed
  * and handles receiving the data entered by the user
  * @param req HTTP request for which the uri needs to be handled.
  * @return ESP_OK
  */
 
 
 static esp_err_t http_server_register_erase_handler(httpd_req_t *req)
 {
     size_t header_len;
     char* header_value;
     char* hour_str = NULL;
     char* reg_str = NULL;
     char* min_str = NULL;
     char* days = NULL;
     int content_length;
 
     ESP_LOGI(TAG, "/regerase.json requested");
 
     // Get the "Content-Length" header to determine the length of the request body
     header_len = httpd_req_get_hdr_value_len(req, "Content-Length");
     if (header_len <= 0) {
         // Content-Length header not found or invalid
         //httpd_resp_send_err(req, HTTP_STATUS_411_LENGTH_REQUIRED, "Content-Length header is missing or invalid");
         ESP_LOGI(TAG, "Content-Length header is missing or invalid");
         return ESP_FAIL;
     }
 
     // Allocate memory to store the header value
     header_value = (char*)malloc(header_len + 1);
     if (httpd_req_get_hdr_value_str(req, "Content-Length", header_value, header_len + 1) != ESP_OK) {
         // Failed to get Content-Length header value
         free(header_value);
         //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Failed to get Content-Length header value");
         ESP_LOGI(TAG, "Failed to get Content-Length header value");
         return ESP_FAIL;
     }
 
     // Convert the Content-Length header value to an integer
     content_length = atoi(header_value);
     free(header_value);
 
     if (content_length <= 0) {
         // Content length is not a valid positive integer
         //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid Content-Length value");
         ESP_LOGI(TAG, "Invalid Content-Length value");
         return ESP_FAIL;
     }
 
     // Allocate memory for the data buffer based on the content length
     char* data_buffer = (char*)malloc(content_length + 1);
 
     // Read the request body into the data buffer
     if (httpd_req_recv(req, data_buffer, content_length) <= 0) {
         // Handle error while receiving data
         free(data_buffer);
         //httpd_resp_send_err(req, HTTP_STATUS_INTERNAL_SERVER_ERROR, "Failed to receive request body");
         ESP_LOGI(TAG, "Failed to receive request body");
         return ESP_FAIL;
     }
 
     // Null-terminate the data buffer to treat it as a string
     data_buffer[content_length] = '\0';
 
     // Parse the received JSON data
     cJSON* root = cJSON_Parse(data_buffer);
     free(data_buffer);
 
     if (root == NULL) {
         // JSON parsing error
         //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Invalid JSON data");
         ESP_LOGI(TAG, "Invalid JSON data");
         return ESP_FAIL;
     }
 
     cJSON* reg_number_json = cJSON_GetObjectItem(root, "selectedNumber");
     
     if (reg_number_json == NULL || !cJSON_IsString(reg_number_json) ) {
         cJSON_Delete(root);
         // Missing or invalid JSON fields
         //httpd_resp_send_err(req, HTTP_STATUS_BAD_REQUEST, "Missing or invalid JSON data fields");
         ESP_LOGI(TAG, "Missing or invalid JSON data fields");
         return ESP_FAIL;
     }
 
 
     reg_str = strdup(reg_number_json->valuestring);
     
 
     ESP_LOGI(TAG, "received reg: %s", reg_str);
 
  
     
     char str_to_save[12];
     //str_to_save[11]=0x00;
     memset(str_to_save, 0x00, 12);
     
 
 
     // Iterate over each element in the array
     
     
     //strcat(str_to_save, reg_str);
     strcat(str_to_save, "99");
     strcat(str_to_save, "99");
     strcat(str_to_save, "0000000");
     
     printf("%s\n", str_to_save);
     save_reg_data(atoi(reg_str), str_to_save);
     update_register(atoi(reg_str));
     // Process the selected days array
    // cJSON* day_item;
     //cJSON_ArrayForEach(day_item, selectedDays_json) {
        // if (cJSON_IsString(day_item)) {
          //   const char* day_str = day_item->valuestring;
            // ESP_LOGI(TAG, "Selected Day: %s", day_str);
 
             // Perform any additional actions based on the selected day
             // ...
 
             // Release memory when no longer needed
         //}
         
     //}
     //free(day_item);
 
     //free(selectedDays_json);
 
     // Send a success response to the client
     // Cerrar la conexion
     free(reg_str);	
     cJSON_Delete(root);
     httpd_resp_set_hdr(req, "Connection", "close");
     httpd_resp_send(req, NULL, 0);
     
 
     return ESP_OK;
 }
 
 


//MARK: HANDLER FOR FILE(HTML, CSS, JS)
esp_err_t index_handler(httpd_req_t *req) {
    extern const unsigned char index_html_start[] asm("_binary_index_html_start");
    extern const unsigned char index_html_end[] asm("_binary_index_html_end");
    const size_t index_html_size = (index_html_end - index_html_start);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_size);
    return ESP_OK;
}

esp_err_t styles_handler(httpd_req_t *req) {
    extern const unsigned char styles_css_start[] asm("_binary_styles_css_start");
    extern const unsigned char styles_css_end[] asm("_binary_styles_css_end");
    const size_t styles_css_size = (styles_css_end - styles_css_start);
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)styles_css_start, styles_css_size);
    return ESP_OK;
}

esp_err_t script_handler(httpd_req_t *req) {
    extern const unsigned char script_js_start[] asm("_binary_script_js_start");
    extern const unsigned char script_js_end[] asm("_binary_script_js_end");
    const size_t script_js_size = (script_js_end - script_js_start);
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)script_js_start, script_js_size);
    return ESP_OK;
}


//MARK: URI GET VALUE ADC
// Definir la URI para la solicitud de obtener el valor de ADC
httpd_uri_t uri_get_adc = {
    .uri = "/get_adc",
    .method = HTTP_GET,
    .handler = adc_handler,
    .user_ctx = NULL
};

//MARK: URI TEMPERATURE ADC
// Definir la URI para la solicitud de obtener el valor de Temperature
httpd_uri_t uri_get_tem_adc = {
    .uri = "/turn_on_temp",
    .method = HTTP_GET,
    .handler = adc_tem_handler,
    .user_ctx = NULL
};

//MARK: URI TOOGLE LED
// register toogle_led handler
httpd_uri_t uri_toogle_led = {
    .uri = "/toogle_led",
    .method = HTTP_POST,
    .handler = http_server_toogle_led_handler,
    .user_ctx = NULL
};

//MARK: URI CHANGE COLOR READ ADC
httpd_uri_t uri_change_color = {
    .uri = "/change_color",
    .method = HTTP_POST,
    .handler = change_color_handler,
    .user_ctx = NULL
};

//MARK: URI SET COLOR 
httpd_uri_t uri_set_rgb = {
    .uri = "/set_rgb",
    .method = HTTP_POST,
    .handler = set_rgb_handler,
    .user_ctx = NULL
};

//MARK: URI CONNECT WIFI
httpd_uri_t uri_connect_wifi = {
    .uri = "/connect",
    .method = HTTP_POST,
    .handler = connect_wifi_handler,
    .user_ctx = NULL
};

//MARK: URI RGB CHROMATIC CIRCLE
httpd_uri_t uri_rgb_crhomatic_circle = {
    .uri = "/rgb_crhomatic_circle",
    .method = HTTP_POST,
    .handler = rgb_crhomatic_circle_handler,
    .user_ctx = NULL
}; 

//MARK: URI RGB CHROMATIC CIRCLE
httpd_uri_t uri_slider_crhomatic_circle = {
    .uri = "/slider_crhomatic_circle",
    .method = HTTP_POST,
    .handler = slider_crhomatic_circle_handler,
    .user_ctx = NULL
}; 

//MARK: URI SET COLOR 
httpd_uri_t uri_time_rgb = {
    .uri = "/set_time",
    .method = HTTP_POST,
    .handler = set_time_handler,
    .user_ctx = NULL
};

//--------------------------------------------------------------------------------------------------

// register OTAupdate handler
httpd_uri_t OTA_update = {
    .uri = "/OTAupdate",
    .method = HTTP_POST,
    .handler = http_server_OTA_update_handler,
    .user_ctx = NULL
};


// register OTAstatus handler
httpd_uri_t OTA_status = {
    .uri = "/OTAstatus",
    .method = HTTP_POST,
    .handler = http_server_OTA_status_handler,
    .user_ctx = NULL
};


//--------------------------------------------------------------------------------------------------

// register OTAstatus handler
httpd_uri_t register_change = {
    .uri = "/regchange.json",
    .method = HTTP_POST,
    .handler = http_server_register_change_handler,
    .user_ctx = NULL
};


// register erase handler
httpd_uri_t register_erase = {
    .uri = "/regerase.json",
    .method = HTTP_POST,
    .handler = http_server_register_erase_handler,
    .user_ctx = NULL
};




//MARK: UPDATE START WEB SERVER FOR INCLUDE HANDLERS
// Actualizar la función de inicio del servidor web para incluir estos manejadores
void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;


    config.max_uri_handlers = 20;

    // The core that the HTTP server will run on
	  config.core_id = HTTP_SERVER_TASK_CORE_ID;

	  // Adjust the default priority to 1 less than the wifi application task
	  config.task_priority = HTTP_SERVER_TASK_PRIORITY;

	  // Bump up the stack size (default is 4096)
	  config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;
  
    config.uri_match_fn = httpd_uri_match_wildcard;

    http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));
    // Create HTTP server monitor task
    xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_MONITOR_PRIORITY, &task_http_server_monitor, HTTP_SERVER_MONITOR_CORE_ID);


    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get_adc);
        httpd_register_uri_handler(server, &uri_get_tem_adc);
        httpd_register_uri_handler(server, &uri_toogle_led);
        httpd_register_uri_handler(server, &uri_set_rgb);
        httpd_register_uri_handler(server, &uri_change_color);
        httpd_register_uri_handler(server, &uri_connect_wifi);
        httpd_register_uri_handler(server, &uri_rgb_crhomatic_circle);
        httpd_register_uri_handler(server, &uri_slider_crhomatic_circle);
        httpd_register_uri_handler(server, &uri_time_rgb);
        httpd_register_uri_handler(server, &OTA_update);
        httpd_register_uri_handler(server, &OTA_status);
        httpd_register_uri_handler(server, &register_change);
        httpd_register_uri_handler(server, &register_erase);

        // Registrar los nuevos manejadores
        httpd_uri_t uri_index = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = index_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_index);

        httpd_uri_t uri_styles = {
            .uri      = "/styles.css",
            .method   = HTTP_GET,
            .handler  = styles_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_styles);

        httpd_uri_t uri_script = {
            .uri      = "/script.js",
            .method   = HTTP_GET,
            .handler  = script_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_script);

        ESP_LOGI(TAG, "Web server started");
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
	http_server_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}


void http_server_fw_update_reset_callback(void *arg)
{
	ESP_LOGI(TAG, "http_server_fw_update_reset_callback: Timer timed-out, restarting the device");
	esp_restart();
}

//MARK: INIT QUEUE
void comandos_init_server(void) {
    read_pot                          =  xQueueCreate(5, sizeof(float));
    change_current_color              = xQueueCreate(5, sizeof(float));
    rgb_crhomatic_circle_red_queue    = xQueueCreate(10, sizeof(int));
    rgb_crhomatic_circle_green_queue  = xQueueCreate(10, sizeof(int));
    rgb_crhomatic_circle_blue_queue   = xQueueCreate(10, sizeof(int));
    slider_crhomatic_circle_queue = xQueueCreate(5, sizeof(float));
    rgb_time_on_queue    = xQueueCreate(10, sizeof(int));
    rgb_time_off_queue    = xQueueCreate(10, sizeof(int));
} 

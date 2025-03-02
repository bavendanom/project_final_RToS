#include "wifi_library.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/netdb.h"
#include "esp_sntp.h"
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// netif objects for the station and access point
esp_netif_t* esp_netif_sta = NULL;
esp_netif_t* esp_netif_ap  = NULL;

bool time_was_synchronized;

static const char *TAG = "wifi station";

SemaphoreHandle_t mySemaphore;

// Used for returning the WiFi configuration
wifi_config_t *wifi_config_sta = NULL;

bool get_state_time_was_synchronized( void ){
	return time_was_synchronized;
}

static void obtain_time(void)
{   
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    ESP_LOGI(TAG, "Initializing SNTP");

    // Usar las funciones recomendadas en lugar de las deprecadas
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "0.co.pool.ntp.org");
    esp_sntp_init();

    // Esperar a que se sincronice el tiempo con el servidor SNTP
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (retry < retry_count)
    {
        ESP_LOGI(TAG, "System time is set!");
        time_was_synchronized = true;
    }
    else
    {
        ESP_LOGE(TAG, "Unable to set system time. Check your SNTP configuration.");
    }
}
static int s_retry_num = 0;




//--------------------------//------------------------------------//----------------------------//

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
} 

/**
 * Initializes the WiFi application event handler for WiFi and IP events.
 */
static void wifi_app_event_handler_init(void)
{
	// Event loop for the WiFi driver
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	// Event handler for the connection
	esp_event_handler_instance_t instance_wifi_event;
	esp_event_handler_instance_t instance_ip_event;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_wifi_event));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_ip_event));
}


/**
 * Initializes the TCP stack and default WiFi configuration.
 */
static void wifi_app_default_wifi_init(void)
{
	// Initialize the TCP stack
	ESP_ERROR_CHECK(esp_netif_init());

	// Default WiFi config - operations must be in this order!
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    // Crear la interfaz de red STA y AP
	esp_netif_sta = esp_netif_create_default_wifi_sta();
	esp_netif_ap = esp_netif_create_default_wifi_ap();
}


static void wifi_init_apsta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    


    /* wifi_config_t wifi_config_sta = {
        .sta = {
            .ssid = EXAMPLE_STA_SSID,
            .password = EXAMPLE_STA_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .sae_h2e_identifier = "",
        },
    };  */

    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = EXAMPLE_AP_SSID,
            .ssid_len = strlen(EXAMPLE_AP_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_AP_PASS,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = WIFI_AP_SSID_HIDDEN,
            .max_connection = WIFI_AP_MAX_CONNECTIONS,
            .beacon_interval = WIFI_AP_BEACON_INTERVAL,
            .pmf_cfg = {
                .required = true,
            },
        },
    };

    if (strlen(EXAMPLE_AP_PASS) == 0) {
        wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
    } 

    // Configure DHCP for the AP
	esp_netif_ip_info_t ap_ip_info;
	memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));

	esp_netif_dhcps_stop(esp_netif_ap);					///> must call this first
	inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip);		///> Assign access point's static IP, GW, and netmask
	inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
	inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);
	ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));			///> Statically configure the network interface
	ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));						///> Start the AP DHCP server (for connecting stations e.g. your mobile device)

    //WIFI_MODE_APSTA para habilitar ambos modos
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    //ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_AP_BANDWIDTH));		///> Our default bandwidth 20 MHz
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));	

    

    ESP_LOGI(TAG, "WiFi AP + STA mode started.");
}

wifi_config_t* wifi_app_get_wifi_config(void)
{
	return wifi_config_sta;
}


static void wifi_app_connect_sta(void)
{
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_app_get_wifi_config()));
	ESP_ERROR_CHECK(esp_wifi_connect());
}




//--------------------------//------------------------------------//----------------------------//

void save_wifi_credentials(const char *ssid, const char *password) {
    printf("save wifi");
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "wifi_ssid", ssid));
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "wifi_password", password));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);
}

void load_wifi_credentials(char *ssid, char *password) {
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READONLY, &nvs_handle));

    size_t required_size;

    // Get the size of wifi_ssid
    ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "wifi_ssid", NULL, &required_size));
    // Allocate memory for wifi_ssid
    char *ssid_buffer = malloc(required_size);
    if (ssid_buffer == NULL) {
        // Handle memory allocation error
        ESP_LOGE(TAG, "Failed to allocate memory for wifi_ssid");
        nvs_close(nvs_handle);
        return;
    }
    // Get wifi_ssid
    ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "wifi_ssid", ssid_buffer, &required_size));
    // Copy wifi_ssid to the output parameter
    strncpy(ssid, ssid_buffer, required_size);

    // Repeat the process for wifi_password
    ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "wifi_password", NULL, &required_size));
    char *password_buffer = malloc(required_size);
    if (password_buffer == NULL) {
        // Handle memory allocation error
        ESP_LOGE(TAG, "Failed to allocate memory for wifi_password");
        free(ssid_buffer);
        nvs_close(nvs_handle);
        return;
    }
    ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "wifi_password", password_buffer, &required_size));
    strncpy(password, password_buffer, required_size);

    // Free the allocated memory
    free(ssid_buffer);
    free(password_buffer);

    nvs_close(nvs_handle);
}

bool nvs_credentials_exist() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return false;
    }

    size_t ssid_size, password_size;
    err = nvs_get_str(nvs_handle, "wifi_ssid", NULL, &ssid_size);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return false;
    }

    err = nvs_get_str(nvs_handle, "wifi_password", NULL, &password_size);
    nvs_close(nvs_handle);

    return err == ESP_OK;
}



void connect_to_wifi(void) {

	if (xSemaphoreTake(mySemaphore, portMAX_DELAY)) { // helpus to not allow multiple calls
 
    	char ssid[32];
    	char password[64];

        // Usar la variable global wifi_config_sta
        if (wifi_config_sta == NULL) {
            ESP_LOGE(TAG, "wifi_config_sta no está inicializado");
            xSemaphoreGive(mySemaphore);
            return;
        }

    	load_wifi_credentials(ssid, password);
		wifi_config_t* wifi_config_sta = wifi_app_get_wifi_config();
		ESP_LOGI(TAG, "saved SSID: %s", ssid);
    	ESP_LOGI(TAG, "saved password: %s", password);

		//memset(wifi_config->sta.ssid, 0x00, sizeof(wifi_config->sta.ssid));
		//memset(wifi_config->sta.password, 0x00, sizeof(wifi_config->sta.password));
		memset(wifi_config_sta, 0x00, sizeof(wifi_config_t));
   		strncpy((char*)wifi_config_sta->sta.ssid, ssid, sizeof(wifi_config_sta->sta.ssid));
    	strncpy((char*)wifi_config_sta->sta.password, password, sizeof(wifi_config_sta->sta.password));
   		esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config_sta);
   		wifi_app_connect_sta();

	       // Do some work
        // Release the semaphore (give it back)
        xSemaphoreGive(mySemaphore);		

    }
}

void check_sta_connection_state( void ) {
	wifi_ap_record_t ap_info;
	esp_err_t ret;
	while(true){
		ret = esp_wifi_sta_get_ap_info(&ap_info);
		 ESP_LOGI(TAG, "Checking sta info");
		    if (ret == ESP_OK) {
				
        		if (ap_info.authmode != WIFI_AUTH_MAX) {
        		    ESP_LOGI(TAG, "Connected to SSID: %s", ap_info.ssid);
					if (get_state_time_was_synchronized() == false)
						obtain_time();
					
        		} else {
        		    ESP_LOGI(TAG, "Not connected to any WiFi network");
					if (nvs_credentials_exist()) {
						// Credentials exist, try to connect
						connect_to_wifi();
						ESP_LOGI(TAG, "CHECKING CONNECTION TO STA_BEFORE_SAVED");
					}

					//return false;
        		}
    		} else {
					if (nvs_credentials_exist()) {
							// Credentials exist, try to connect
							connect_to_wifi();
							ESP_LOGI(TAG, "CHECKING CONNECTION TO STA_BEFORE_SAVED");
						}
       			 ESP_LOGI(TAG, "Failed to get connection info");
					//return false;
        		}
		vTaskDelay(20000 / portTICK_PERIOD_MS);
	}
    // Get the connection info

}
void wifi_app_start(void)
{
    mySemaphore = xSemaphoreCreateMutex();
    wifi_app_event_handler_init();
    wifi_app_default_wifi_init();
    wifi_init_apsta();
    ESP_ERROR_CHECK(esp_wifi_start());

    // Inicializar wifi_config_sta
    wifi_config_sta = malloc(sizeof(wifi_config_t));
    if (wifi_config_sta == NULL) {
        ESP_LOGE(TAG, "Error al asignar memoria para wifi_config_sta");
        return;
    }
    memset(wifi_config_sta, 0, sizeof(wifi_config_t)); // Inicializar a cero
    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_config_sta = malloc(sizeof(wifi_config_t));
    memset(wifi_config_sta, 0, sizeof(wifi_config_t));

    // Cargar credenciales guardadas al iniciar
    if (nvs_credentials_exist()) {
        char ssid[32] = {0};
        char password[64] = {0};
        load_wifi_credentials(ssid, password);
        strncpy((char*)wifi_config_sta->sta.ssid, ssid, sizeof(wifi_config_sta->sta.ssid));
        strncpy((char*)wifi_config_sta->sta.password, password, sizeof(wifi_config_sta->sta.password));
        esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config_sta);
        esp_wifi_connect(); // Intentar conexión inmediata al iniciar
    }

}
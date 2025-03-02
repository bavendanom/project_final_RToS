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
#include "lwip/netdb.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "esp_wifi_types.h"

#include "lwip/err.h"
#include "lwip/sys.h"



//#define EXAMPLE_STA_SSID       "Kraken_PLUS"//"Marcela 2.4"
//#define EXAMPLE_STA_PASS       "Rhl418yga"//"35490562M"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5
//sta
#define EXAMPLE_AP_SSID        "100 de cilantro"
#define EXAMPLE_AP_PASS        "123456789"
#define EXAMPLE_ESP_WIFI_CHANNEL   1//CONFIG_ESP_WIFI_CHANNEL
//#define EXAMPLE_MAX_STA_CONN       1//CONFIG_ESP_MAX_STA_CONN

#define WIFI_AP_SSID_HIDDEN			0					// AP visibility
#define WIFI_AP_MAX_CONNECTIONS		5					// AP max clients
#define WIFI_AP_BEACON_INTERVAL		100					// AP beacon: 100 milliseconds recommended
#define WIFI_AP_IP					"192.168.0.1"		// AP default IP
#define WIFI_AP_GATEWAY				"192.168.0.1"		// AP default Gateway (should be the same as the IP)
#define WIFI_AP_NETMASK				"255.255.255.0"		// AP netmask
#define WIFI_AP_BANDWIDTH			WIFI_BW_HT40		// AP bandwidth 20 MHz (40 MHz is the other option)
#define WIFI_STA_POWER_SAVE			WIFI_PS_NONE		// Power save not used
#define MAX_SSID_LENGTH				32					// IEEE standard maximum
#define MAX_PASSWORD_LENGTH			64					// IEEE standard maximum
#define MAX_CONNECTION_RETRIES		5					// Retry number on disconnect

// netif object for the Station and Access Point
extern esp_netif_t* esp_netif_sta;
extern esp_netif_t* esp_netif_ap;

void save_wifi_credentials(const char *ssid, const char *password);

void connect_to_wifi(void);

void wifi_app_start(void);

/**
 * Gets the wifi configuration
 */
wifi_config_t* wifi_app_get_wifi_config(void);
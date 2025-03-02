#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

// Definir la cola para temperatura
extern QueueHandle_t temp_queue;
extern QueueHandle_t min_red_queue;
extern QueueHandle_t max_red_queue;
extern QueueHandle_t min_green_queue;
extern QueueHandle_t max_green_queue;
extern QueueHandle_t min_blue_queue;
extern QueueHandle_t max_blue_queue;

// Tipo de función para manejar comandos
typedef void (*command_handler_t)(char *value);

// Estructura para la tabla de comandos
typedef struct {
    const char *cmd;           // Nombre del comando
    command_handler_t handler; // Función que maneja el comando
} command_entry_t;

// Declaración de funciones públicas
void queue_comandos_init(void);                     // Inicializa la librería de comandos
void process_command(char *cmd);              // Procesa un comando recibido
void handle_get_tem(void);                    // Maneja el comando GET_TEM

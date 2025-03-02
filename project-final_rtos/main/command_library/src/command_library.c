#include "command_library.h"
#include "uart_library.h"
#include "server_library.h"


// Definir la cola para temperatura
QueueHandle_t temp_queue;
QueueHandle_t min_red_queue;
QueueHandle_t max_red_queue;
QueueHandle_t min_green_queue;
QueueHandle_t max_green_queue;
QueueHandle_t min_blue_queue;
QueueHandle_t max_blue_queue;



int min_red = 0, max_red = 100;
int min_green = 0, max_green = 100;
int min_blue = 0, max_blue = 100;
int ONOFF = 0;
int on_time = 0, off_time = 0;


void task_get_tem_print(void *arg) {
    float temperatura;
    sendData("CMD_HANDLER","prueba 2");
    while (1) {
        if (xQueueReceive(temp_queue, &temperatura, pdMS_TO_TICKS(100)) == pdTRUE) {
            char mensaje[50];
            snprintf(mensaje, sizeof(mensaje), "Temperatura: %.2f°C\n", temperatura);
            sendData("CMD_HANDLER", mensaje);
          // Esperar 2 segundos antes de la siguiente lectura
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


void handle_get_tem_wrapper(char *value) {
    handle_get_tem();  // Llama a la función real sin usar "value"
}


void handle_get_tem(void) {
    float temperatura;
    if (xQueueReceive(temp_queue, &temperatura, pdMS_TO_TICKS(100)) == pdTRUE) {
        char mensaje[50];
        snprintf(mensaje, sizeof(mensaje), "Temperatura: %.2f°C\n", temperatura);
        sendData("CMD_HANDLER", mensaje);
    } else {
        ESP_LOGW("CMD_HANDLER", "No se pudo recibir la temperatura");
    }
}

void handle_min_red(char *value) {
    min_red = atoi(value);  // Convierte el valor a int (entero)
    char mensaje[50];

    // Enviar el nuevo valor a la cola
    if (xQueueSend(min_red_queue, &min_red, portMAX_DELAY)== pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "MIN_RED  configurado en %i°C\n", min_red);
        sendData("CMD_HANDLER", mensaje);
    } else {
        sendData("CMD_HANDLER", "Error al enviar MIN_RED a la cola");
    }

}

void handle_max_red(char *value) {
    max_red = atoi(value);
    char mensaje[50];

    // Enviar el nuevo valor a la cola
    if (xQueueSend(max_red_queue, &max_red, portMAX_DELAY) == pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "MAX_RED  configurado en %i°C\n", max_red);
        sendData("CMD_HANDLER", mensaje);
    } else {
        sendData("CMD_HANDLER", "Error al enviar MAX_RED a la cola");
    }
}

void handle_min_green(char *value) {
    min_green = atoi(value);
    char mensaje[50];

    // Enviar el nuevo valor a la cola
    if (xQueueSend(min_green_queue, &min_green, portMAX_DELAY) == pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "MIN_GREEN  configurado en %i°C\n", min_green);
        sendData("CMD_HANDLER", mensaje);
    } else {
        sendData("CMD_HANDLER", "Error al enviar MIN_GREEN a la cola");
    }
}

void handle_max_green(char *value) {
    max_green = atoi(value);
    char mensaje[50];
    
    // Enviar el nuevo valor a la cola
    if (xQueueSend(max_green_queue, &max_green, portMAX_DELAY) == pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "MAX_GREEN configurado en %i°C\n", max_green);
        sendData("CMD_HANDLER", mensaje);
    } else {
        sendData("CMD_HANDLER", "Error al enviar MAX_GREEN a la cola");
    }
}

void handle_min_blue(char *value) {
    min_blue = atoi(value);
    char mensaje[50];

    // Enviar el nuevo valor a la cola
    if (xQueueSend(min_blue_queue, &min_blue, portMAX_DELAY) == pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "MIN_BLUE  configurado en %i°C\n", min_blue);
        sendData("CMD_HANDLER", mensaje);
    } else {
        sendData("CMD_HANDLER", "Error al enviar MIN_BLUE a la cola");
    }
}

void handle_max_blue(char *value) {
    max_blue = atoi(value);
    char mensaje[50];
    
    // Enviar el nuevo valor a la cola
    if (xQueueSend(max_blue_queue, &max_blue, portMAX_DELAY) == pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "MAX_BLUE configurado en %i°C\n", max_blue);
        sendData("CMD_HANDLER", mensaje);
    } else {
        sendData("CMD_HANDLER", "Error al enviar MAX_BLUE a la cola");
    }
}
void handle_print_tem(char *value) {
    ONOFF  = atoi(value);
    static TaskHandle_t task_handle = NULL;  // Para manejar la tarea

    if (ONOFF   == 1) {
        if (task_handle == NULL) {
            BaseType_t result = xTaskCreate(task_get_tem_print, "task_get_tem_print", 8192, NULL, 6, &task_handle);
            if (result == pdPASS) {
                sendData("CMD_HANDLER", "Tarea PRINT_TEM iniciada correctamente");
                ESP_LOGI("CMD_HANDLER", "Tarea PRINT_TEM iniciada correctamente");
            } else {
                sendData("CMD_HANDLER", "Error al crear la tarea PRINT_TEM");
                ESP_LOGE("CMD_HANDLER", "Error al crear la tarea PRINT_TEM");
            }
        } else {
            sendData("CMD_HANDLER", "Tarea PRINT_TEM ya está en ejecución");
            ESP_LOGW("CMD_HANDLER", "Tarea PRINT_TEM ya está en ejecución");
        }
    } else if (ONOFF  == 0) {
        if (task_handle != NULL) {
            vTaskDelete(task_handle);
            task_handle = NULL;
            sendData("CMD_HANDLER", "Tarea PRINT_TEM detenida");
            ESP_LOGI("CMD_HANDLER", "Tarea PRINT_TEM detenida");
        } else {
            sendData("CMD_HANDLER", "No hay tarea PRINT_TEM en ejecución");
            ESP_LOGW("CMD_HANDLER", "No hay tarea PRINT_TEM en ejecución");
        }
    } else {
        sendData("CMD_HANDLER", "Parámetro no reconocido");
        ESP_LOGE("CMD_HANDLER", "Parámetro no reconocido: %s", value);
    }
}


void handle_on_time(char *value) {
    on_time = atoi(value);
    char mensaje[50];

    // Enviar el nuevo valor a la cola
    if (xQueueSend(rgb_time_on_queue, &on_time, portMAX_DELAY) == pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "ON_TIME  configurado en %i segundos\n", on_time);
        sendData("CMD_HANDLER", mensaje);
    } else {
        sendData("CMD_HANDLER", "Error al enviar ON_TIME a la cola");
    }
}
void handle_off_time(char *value) {
    off_time = atoi(value);
    char mensaje[50];

    // Enviar el nuevo valor a la cola
    if (xQueueSend(rgb_time_off_queue, &off_time, portMAX_DELAY) == pdTRUE) {
        snprintf(mensaje, sizeof(mensaje), "MIN_TIME  configurado en %i segundos\n", off_time);
        sendData("CMD_HANDLER", mensaje);
    } else {
        sendData("CMD_HANDLER", "Error al enviar OFF_TIME a la cola");
    }
}




typedef void (*command_handler_t)(char *value);
//MARK: Tabla de comandos
// Tabla de comandos
command_entry_t command_table[] = {
    {"GET_TEM", handle_get_tem_wrapper},
    {"MIN_RED", handle_min_red},
    {"MAX_RED", handle_max_red},
    {"MIN_GREEN", handle_min_green},
    {"MAX_GREEN", handle_max_green},
    {"MIN_BLUE", handle_min_blue},
    {"MAX_BLUE", handle_max_blue},
    {"PRINT_TEM", handle_print_tem},
    {"ON_TIME", handle_on_time},
    {"OFF_TIME", handle_off_time}
};

#define NUM_COMMANDS (sizeof(command_table) / sizeof(command_table[0]))

void process_command(char *cmd) {
    ESP_LOGI("CMD_HANDLER", "Comando recibido: %s", cmd);

    char *delimiter = strchr(cmd, '$');  // Busca el delimitador '$'
    char *param = NULL;

    if (delimiter != NULL) {
        *delimiter = '\0';  // Separa el comando y el valor
        param = delimiter + 1;
    }

    for (int i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(cmd, command_table[i].cmd) == 0) {
            if (command_table[i].handler != NULL) {
                command_table[i].handler(param);  // Llama a la función con el valor
            } else {
                ESP_LOGI("CMD_HANDLER", "Comando sin parámetro ejecutado: %s", cmd);
            }
            return;
        }
    }

    ESP_LOGI("CMD_HANDLER", "Comando no reconocido: %s", cmd);
}


/* void process_command(char *cmd) {
    ESP_LOGI("CMD_HANDLER", "Comando recibido: %s", cmd);
    for (int i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(cmd, command_table[i].cmd) == 0) {
            command_table[i].handler();
            return;
        }
    }
    ESP_LOGI("CMD_HANDLER", "Comando no reconocido: %s", cmd);
} */

void queue_comandos_init(void) {
    temp_queue        =  xQueueCreate(5, sizeof(float));
    min_red_queue     =  xQueueCreate(5, sizeof(float));
    max_red_queue     =  xQueueCreate(5, sizeof(float));
    min_blue_queue    =  xQueueCreate(5, sizeof(float));
    max_blue_queue    =  xQueueCreate(5, sizeof(float));
    min_green_queue   =  xQueueCreate(5, sizeof(float));
    max_green_queue   =  xQueueCreate(5, sizeof(float));
}

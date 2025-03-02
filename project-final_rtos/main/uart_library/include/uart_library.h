
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"


#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)

#define RX_BUF_SIZE 1024  // Definir RX_BUF_SIZE aquí

extern QueueHandle_t uart_rx_queue;  // Declarar la cola UART

void uart_init(void);
void rx_task(void *arg);  // Declarar rx_task
int sendData(const char* logName, const char* data);


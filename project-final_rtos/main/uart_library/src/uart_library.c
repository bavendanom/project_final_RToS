#include "uart_library.h"



#define CMD_DELIMITER '\n'

// Definir la cola global para recibir datos por UART
QueueHandle_t uart_rx_queue;

void uart_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    uart_driver_install(UART_NUM_0, RX_BUF_SIZE * 2, RX_BUF_SIZE * 2, 10, &uart_rx_queue, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

int sendData(const char* logName, const char* data) {
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_0, data, len);
    ESP_LOGI(logName, "Wrote %d bytes: '%s'", txBytes, data);
    return txBytes;
}

void rx_task(void *arg) {
    static const char *RX_TASK_TAG = "RX_TASK";
    uint8_t data[RX_BUF_SIZE + 1];
    char received_cmd[RX_BUF_SIZE];
    int cmd_index = 0;

    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = '\0';
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);

            // Procesar cada byte recibido
            for (int i = 0; i < rxBytes; i++) {
                if (data[i] == CMD_DELIMITER) {
                    received_cmd[cmd_index] = '\0';  // Terminar la cadena
                    xQueueSend(uart_rx_queue, received_cmd, portMAX_DELAY); // Enviar a la cola
                    cmd_index = 0;  // Reiniciar el índice
                } else {
                    if (cmd_index < RX_BUF_SIZE - 1) {
                        received_cmd[cmd_index++] = data[i];
                    }
                }
            }
        }
    }
}

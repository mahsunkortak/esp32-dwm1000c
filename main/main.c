
// void printList() {
//    struct locations *locations = head;
//    printf("\n[ ");

//    //start from the beginning
//    while(locations != NULL) {
//       printf("(%d,%d) ",locations->key,locations->data);
//       locations = locations->next;
//    }

//    printf(" ]");
// }
#define comment
#ifndef comment0

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include <stdint.h>

#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)

#define DATA_LENGTH 52
static const int RX_BUF_SIZE = 1024;
char press_enter[6] = "\r\n\r";     //send enter command to switches shell mode
char CR[2] = "\r";     //send enter command to switches shell mode
char NL[2] = "\n";     //send enter command to switches shell mode
char CRR[2] = {'\r','\n'};
uint16_t CR_HEX = 0x0D;

char shell_cmd[7] = "les\r\n";    // send "les" command to get location data.
char generic_cmd[8] = "quit\r\n"; // send "quit" command to switches generic mode

typedef struct locations
{
    char *log_data;
    char *dist;
    char *dev_num;
    char *an0;
    char *an0_id;
    char *an0_x;
    char *an0_y;
    char *an0_z;

    char *an1;
    char *an1_id;
    char *an1_x;
    char *an1_y;
    char *an1_z;

    struct locations *next;
};

struct locations *log_datass = NULL;
struct locations *root = NULL;

void init(void)
{ //setting communication parameters.
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    //setting communication pins.
    uart_driver_install(UART_NUM_0, RX_BUF_SIZE * 2, RX_BUF_SIZE * 2, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_mode(UART_NUM_0, UART_MODE_UART);
}

int sendData(const char *logName, const char *data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_0, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1)
    {
        sendData(TX_TASK_TAG, "******Disconnected!!*******");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    while (1)
    {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0)
        {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

/* !<bu write_bytes fonksiyonlarını payloadi parse ettikten sonra kaldırmam lazım,
    cunku konsola yazarken aynı zamanda dwm komut gıdıyor. o sebeple veri iletisimi saglıksız
    olur !> */
void switch_to_generic_mode(void)
{

    uart_write_bytes(UART_NUM_0, (const char *)generic_cmd, 4); // strlen((const int *)press_enter)
    //vTaskDelay(500 / portTICK_PERIOD_MS);
    uart_wait_tx_done(UART_NUM_0, 1000 / portTICK_RATE_MS);
}

void switch_to_shell_mode(void)
{
    int rxBytes ;
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1); //uint8_t olmazsa error.
        do
        {   rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 50 / portTICK_RATE_MS);
            vTaskDelay(350 / portTICK_PERIOD_MS);
            uart_write_bytes(UART_NUM_0, (const char *)CR, 1); // strlen((const int *)press_enter)
           // vTaskDelay(35 / portTICK_PERIOD_MS);
            //uart_wait_tx_done(UART_NUM_0, 1000 / portTICK_RATE_MS);
            //uart_write_bytes(UART_NUM_0, (const char *)CR, 2); 
            //vTaskDelay(500 / portTICK_PERIOD_MS);
            //uart_wait_tx_done(UART_NUM_0, 1000 / portTICK_RATE_MS);
           
        }
        
        
        while (strcmp((const char*)data,"dwm> ") != 0); // if rxBytes<0 => error 0 olursa da sikinti
    
        
        //vTaskDelay(35 / portTICK_PERIOD_MS);
        //uart_wait_tx_done(UART_NUM_0, 1000 / portTICK_RATE_MS);
            
}

static void get_location_data(void *arg)
{
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1); //uint8_t olmazsa error.
    while (1)
    {
        int rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 50 / portTICK_RATE_MS);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        if (rxBytes > (DATA_LENGTH - 1)) // if rxBytes<0 => error 0 olursa da sikinti
        {
            data[rxBytes] = 0; // bu satır olmazsa sacma sapan karekterler alıyorum.
            uart_write_bytes(UART_NUM_0, (const char *)data, strlen((const char *)data));
            vTaskDelay(50 / portTICK_PERIOD_MS);
            //uart_wait_tx_done(UART_NUM_0, 1000 / portTICK_RATE_MS);
        }
    }
    free(data);
}

void app_main(void)
{
    init();
    switch_to_shell_mode();
    xTaskCreate(get_location_data, "get_location_data", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
    //xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
}

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define GPIO_OUTPUT_IO_0    18
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_IO_0)
#define GPIO_INPUT_IO_0     4
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)   // (12)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL); // (13)
}

static void gpio_task_example(void* arg)    // (14)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) { // (15)
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num)); // (16)
        }
    }
}

void app_main(void)
{
    gpio_config_t io_conf;  // (1)
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);  // (2)

    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);  // (2)

    //change gpio intrrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_POSEDGE); // (3)

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));    // (4)
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);  // (5)

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);    // (6)
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);   // (7)

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size()); // (8)

    int cnt = 0;
    while(1) {
        printf("cnt: %d\n", cnt++); // (9)
        vTaskDelay(1000 / portTICK_RATE_MS);    // (10)
        gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);  // (11)
    }
}

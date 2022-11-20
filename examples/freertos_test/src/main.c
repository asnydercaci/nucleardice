#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// If not defined in the platformio.ini then define it here
#ifndef LED
#define LED (2) 
#endif

void led_blink(void *pvParams) {
   TickType_t t = (*(int *) pvParams) / portTICK_RATE_MS;
    while (1) {
        gpio_set_level(LED,0);
        vTaskDelay(t);
        gpio_set_level(LED,1);
        vTaskDelay(t);
    }
}

void led_blinky2(void *pvParams) {
   TickType_t t = (*(int *) pvParams) / portTICK_RATE_MS;
    while (1) {
        gpio_set_level(LED,0);
        vTaskDelay(t);
    }
}

void led_blinky3(void *pvParams) {
   TickType_t t = (*(int *) pvParams) / portTICK_RATE_MS;
    while (1) {
        gpio_set_level(LED,1);
        vTaskDelay(t);
    }
}


int app_main() {
    // Configure pin
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    int t = 1000;
    xTaskCreate(&led_blink,"LED_BLINK",512,&t,5,NULL);

    t = 888;
    xTaskCreate(&led_blinky2,"LED_BLINK2",512,&t,7,NULL);

    t = 1111;
    xTaskCreate(&led_blinky3,"LED_BLINK3",512,&t,5,NULL);

    return 0;
}
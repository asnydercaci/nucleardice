#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "esp_log.h"

// If not defined in the platformio.ini then define it here
#ifndef LED
#define LED 2 
#endif

void led_blink(void *pvParams) {
   TickType_t t = (int) pvParams / portTICK_RATE_MS;
    while (1) {
        gpio_set_level(LED,0);
        vTaskDelay(t);
        gpio_set_level(LED,1);
        vTaskDelay(t);
    }
}

void led_blinky2(void *pvParams) {
   TickType_t t = (int) pvParams / portTICK_RATE_MS;
    while (1) {
        ESP_LOGV("app_main","blinky2 loop start");
        gpio_set_level(LED,0);
        vTaskDelay(t);
    }
}

void led_blinky3(void *pvParams) {
   TickType_t t = (int) pvParams / portTICK_RATE_MS;
    while (1) {
        ESP_LOGV("app_main","blinky3 loop start");
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

    ESP_LOGI("app_main","About to create Tasks");

//    xTaskCreate(&led_blink,"LED_BLINK",512,(void*)1000,5,NULL);
    xTaskCreate(&led_blinky2,"LED_BLINK2",512,(void*)888,5,NULL);
    xTaskCreate(&led_blinky3,"LED_BLINK3",512,(void*)1111,1,NULL);

    ESP_LOGI("app_main","Done creating Tasks");
    return 0;
}
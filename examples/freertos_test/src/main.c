#include <driver/gpio.h>
// Include FreeRTOS for delay
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LED 2 // LED connected to GPIO2

void led_blink(void *pvParams) {
    while (1) {
        gpio_set_level(LED,0);
        vTaskDelay(1000/portTICK_RATE_MS);
        gpio_set_level(LED,1);
        vTaskDelay(1000/portTICK_RATE_MS);
    }
}

void led_blinky2(void *pvParams) {
    while (1) {
        gpio_set_level(LED,0);
        vTaskDelay(888/portTICK_RATE_MS);
    }
}

void led_blinky3(void *pvParams) {
    while (1) {
        gpio_set_level(LED,1);
        vTaskDelay(1111/portTICK_RATE_MS);
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

    xTaskCreate(&led_blink,"LED_BLINK",512,NULL,5,NULL);
    xTaskCreate(&led_blinky2,"LED_BLINK2",512,NULL,7,NULL);
    xTaskCreate(&led_blinky3,"LED_BLINK3",512,NULL,5,NULL);

    return 0;
}
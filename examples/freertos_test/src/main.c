//Example of using PlatformIO with ESP32

// If not defined in the platformio.ini then define it here
#ifndef LED
#define LED 2 
#endif

#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>

// Setup for wifi
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/apps/sntp.h"

#define EXAMPLE_WIFI_SSID "SSIDP5"
#define EXAMPLE_WIFI_PASS "PASSP5"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;
static const char *TAG = "wifi";

void timeprint() {
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    setenv("TZ", "EST-5", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void obtain_time(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    ESP_ERROR_CHECK( esp_wifi_stop() );

    if (timeinfo.tm_year > (2016 - 1900)) {
      ESP_LOGI(TAG,"got time...");
      timeprint();
    }
}


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

void timetask(void *pvParams) {
   TickType_t t = (int) pvParams / portTICK_RATE_MS;
    while (1) {
        timeprint();
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

    esp_log_level_set("*", ESP_LOG_INFO);  

    // Set system time via NTP
    obtain_time();

    ESP_LOGI("app_main","About to create Tasks");

//    xTaskCreate(&led_blink,"LED_BLINK",512,(void*)1000,5,NULL);
    xTaskCreate(&led_blinky2,"LED_BLINK2",2048,(void*)888,5,NULL);
    xTaskCreate(&led_blinky3,"LED_BLINK3",2048,(void*)1111,1,NULL);

    xTaskCreate(&timetask,"TimePrint",2048,(void*)5000,1,NULL);

    ESP_LOGI("app_main","Done creating Tasks");
 
    return 0;
}
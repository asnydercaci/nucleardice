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

//----------------------------------------------------------------------------
// Setup for wifi
//----------------------------------------------------------------------------
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/apps/sntp.h"

#define EXAMPLE_WIFI_SSID "xxxx"
#define EXAMPLE_WIFI_PASS "xxxx"
#define EXAMPLE_ESP_MAXIMUM_RETRY  20

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

static const char *TAG = "setupWifi";
static int s_retry_num = 0;
static int reconnect = 1;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (reconnect) {
          if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
              esp_wifi_connect();
              s_retry_num++;
              ESP_LOGI(TAG, "retry to connect to the AP");
          } else {
              xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
          }
          ESP_LOGI(TAG,"connect to the AP fail");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_done() {
    // Dont reconnect back
    reconnect = 0;
    // Stop Wifi
    ESP_ERROR_CHECK( esp_wifi_stop() );
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s", EXAMPLE_WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s", EXAMPLE_WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

//----------------------------------------------------------------------------
// Time Printing
//----------------------------------------------------------------------------
void timeprint() {
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI("timeprint", "The current date/time is: %s", strftime_buf);
}

//----------------------------------------------------------------------------
// SNTP
//----------------------------------------------------------------------------
static void initialize_sntp(void)
{
    ESP_LOGI("sntp", "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

static void obtain_time(void)
{
    // Wait until We are connected to WIFI
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI("sntp", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year > (2016 - 1900)) {
      ESP_LOGI("sntp", "got time...");
      timeprint();
    }

    ESP_LOGI("sntp", "Stopping Wifi");
    wifi_done();
}

//----------------------------------------------------------------------------
// Tasks
//----------------------------------------------------------------------------
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
        ESP_LOGV("blinky2","loop start");
        gpio_set_level(LED,0);
        vTaskDelay(t);
    }
}

void led_blinky3(void *pvParams) {
   TickType_t t = (int) pvParams / portTICK_RATE_MS;
    while (1) {
        ESP_LOGV("blinky3","loop start");
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

//----------------------------------------------------------------------------
// App main
//----------------------------------------------------------------------------
int app_main() {
    // Setup default log level
    esp_log_level_set("*", ESP_LOG_INFO);  
    esp_log_level_set("wifi", ESP_LOG_ERROR);  

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Configure pin
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // Setup our timezone
    setenv("TZ", "EST5EDT", 1);
    tzset();

    // Setup Wifi
    ESP_LOGI("app_main", "Setting up wifi sta");
    wifi_init_sta();

    // Set system time via NTP
    ESP_LOGI("app_main", "Getting Time");
    obtain_time();

    ESP_LOGI("app_main","About to create Tasks");
//    xTaskCreate(&led_blink,"LED_BLINK",512,(void*)1000,5,NULL);
    xTaskCreate(&led_blinky2,"LED_BLINK2",2048,(void*)888,5,NULL);
    xTaskCreate(&led_blinky3,"LED_BLINK3",2048,(void*)1111,1,NULL);
    xTaskCreate(&timetask,"TimePrint",2048,(void*)5000,1,NULL);
    ESP_LOGI("app_main","Done creating Tasks");
 
    ESP_LOGI("app_main","Leaving app_main");
    return 0;
}
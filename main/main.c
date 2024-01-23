#include <stdio.h>
#include <string.h>

/*includes FreeRTOS*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*includes do ESP32*/
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"

/*includes do Projeto*/
#include "wifi_app.h"

/*define*/
#define LED 2 //LED do kit
#define BOMBA 19 //Pino da bomba
#define BUTTON 21 //Pino do sensor
#define VASO1 27 //Pino da solenoide 1
#define VASO2 26 //Pino da solenoide 2
#define VASO3 25 //Pino da solenoide 3
#define VASO4 33 //Pino da solenoide 4
#define VASO5 32 //Pino da solenoide 5
#define VASO6 35 //Pino da solenoide 6
#define VASO7 34 //Pino da solenoide 7
#define VASO8 39 //Pino da solenoide 8

void app_main(void)
{
    /*Conexão com WIFI*/
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_softap();
    vTaskDelay(pdMS_TO_TICKS(5000));
    wifi_stop();
    wifi_init_sta("APSamsung", "12345f");
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
    
    }
    

}
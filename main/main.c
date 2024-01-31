#include <stdio.h>
#include <string.h>
#include <inttypes.h>

/*includes FreeRTOS*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

/*includes do ESP32*/
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_sntp.h"

/*includes do Projeto*/
#include "wifi_app.h"
#include "http_server.h"
#include "nvs_app.h"
#include "mqtt_app.h"
#include "sntp_app.h"

/*define*/
#define BUTTON  13 //Pino do botão
#define SENSOR  21 //Pino do sensor
#define LED     4  //LED do kit
#define BOMBA   32 //Pino da bomba 19
#define VASO1   27 //Pino da solenoide 1
#define VASO2   26 //Pino da solenoide 2
#define VASO3   25 //Pino da solenoide 3
#define VASO4   33 //Pino da solenoide 4
#define VASO5   23 //Pino da solenoide 5
#define VASO6   22 //Pino da solenoide 6
#define VASO7   19 //Pino da solenoide 7
#define VASO8   18 //Pino da solenoide 8

#define MQTT_TOPIC       "irrigacao/Aju" // Topico do mqtt
#define TAG_VASOS        "a" //Tag da chave para os estados dos 8 solenoides
#define TAG_BOMBA        "b" //Tag da chave para o estado da bomba
#define TAG_AGEN_VASOS   "v" //Tag da chave para os estados dos 8 solenoides
#define TAG_AGEN_HORARIO "h" //Tag do agendamento de horário (h:m)
#define TAG_AGEN_TEMPO   "t" //Tag do tempo de água aberto em minutos
#define TAG_VASOS_STATUS "s" //Tag para de status dos vasos
#define TAG_BOMBA_STATUS "o" //Tag para de status da bomba

#define TEMPO_VERIF     2   //Tempo de vefiricação do horario em minutos
#define SEM_CREDENCIAL  0
#define DESCONECTADO    1
#define CONECTADO       2

static const char *TAG = "IRRIGACAO";
static bool credencial_wifi = false;
static char *ssid;
static char *pass;
static int vazao;
static bool status_registro;
static bool status_bomba;
static int status_conexao = DESCONECTADO;

TaskHandle_t xHandleSntp, xHandleSensor, xHandleBomba, xHandleIndicador;
SemaphoreHandle_t xHandleSemphSensor;

char* extractJson(char *json, char *name)
{
    if ((json != NULL) && (name != NULL) && (json[0] == '{')){
        int pos, tam;
        char *aux;
        tam = strlen(json);
        aux = malloc(sizeof(char)*tam);
        memcpy(aux, json, tam);
        tam = strlen(name);
        while (1){
            aux = strchr(aux, '\"');
            if (aux==NULL){
                ESP_LOGI(TAG, "Não encontrou");
                return NULL;
            }
            aux++;
            pos = strcspn(aux+1, "\"")+1;
            if ((pos == tam )&&(!strncmp(aux, name, tam))){
                aux = aux+tam+3;
                pos = strcspn(aux, "\"");
                aux[pos] = '\0';
                return aux;
            }
        }
    }
    ESP_LOGI(TAG, "Parametro nulo");
    return NULL;
}

void mqtt_app_event_connected(void)
{
    // char topic[33];
    // sprintf(topic, "irrigacao/Aju");
    mqtt_app_subscribe(MQTT_TOPIC);
    status_conexao = CONECTADO;
}

void mqtt_app_event_disconnected(void)
{
    // status_conexao = DESCONECTADO;
}

void set_solenoides(char *tag, char *dado)
{
    char payload[20];
    if ((!strcmp(tag, TAG_AGEN_VASOS)) || (!strcmp(tag, TAG_VASOS))){
        vazao = 0;
        sprintf(payload, "{\"%s\":\"%.*s\"}", TAG_VASOS_STATUS, 8, dado);
        mqtt_app_publish(MQTT_TOPIC, payload);
        gpio_set_level(VASO1, dado[0] == '1');
        gpio_set_level(VASO2, dado[1] == '1');
        gpio_set_level(VASO3, dado[2] == '1');
        gpio_set_level(VASO4, dado[3] == '1');
        gpio_set_level(VASO5, dado[4] == '1');
        gpio_set_level(VASO6, dado[5] == '1');
        gpio_set_level(VASO7, dado[6] == '1');
        gpio_set_level(VASO8, dado[7] == '1');
        if(!strcmp(dado, "00000000")){
            status_registro = pdFALSE;
            vTaskSuspend(xHandleSensor);
            vTaskSuspend(xHandleBomba);
        } else {
            status_registro = pdTRUE;
            vTaskResume(xHandleSensor);
            vTaskResume(xHandleBomba);
        }
        return;
    }     
    if(!strcmp(tag, TAG_BOMBA)){
        if (status_registro){
            sprintf(payload, "{\"%s\":\"%.*s\"}", TAG_BOMBA_STATUS, 1, dado);
            mqtt_app_publish(MQTT_TOPIC, payload);
            status_bomba = (*dado == '1');
            gpio_set_level(BOMBA, status_bomba);
        }
        return;
    }
}

void mqtt_app_event_data(char *publish_string, int tam)
{
    // ESP_LOGW(TAG, "payload: %.*s", tam, publish_string);
    publish_string[tam] = '\0';
    char aux[tam+1];
    strcpy(aux, publish_string);
    char *dado;
    //Comando para ligar os vasos
    dado = extractJson(aux, TAG_VASOS);
    if (dado != NULL){
        set_solenoides(TAG_VASOS, dado);
    }
    //Comando para ligar a bomba
    dado = extractJson(aux, TAG_BOMBA);
    if (dado != NULL){
        // nvs_app_set(TAG_BOMBA, dado, 's');
        set_solenoides(TAG_BOMBA, dado);
    }
    //Comando para agendar os vasos
    dado = extractJson(aux, TAG_AGEN_VASOS);
    if (dado != NULL){
        nvs_app_set(TAG_AGEN_VASOS, dado, 's');
        // ESP_LOGW(TAG, "Agendamento vasos:%.*s", 8, dado);
    }
    //Comando para salvar horario
    dado = extractJson(aux, TAG_AGEN_HORARIO);
    if (dado != NULL){
        nvs_app_set(TAG_AGEN_HORARIO, dado, 's');
        // ESP_LOGW(TAG, "Agendamento horario:%.*s", 6, dado);
    }
    //Comando para salvar tempo
    dado = extractJson(aux, TAG_AGEN_TEMPO);
    if (dado != NULL){
        nvs_app_set(TAG_AGEN_TEMPO, dado, 's');
        // ESP_LOGW(TAG, "Agendamento tempo:%.*s", 2, dado);
    }
}

void fechaAgua(void)
{
    set_solenoides(TAG_BOMBA, "0");
    set_solenoides(TAG_VASOS, "00000000");
}

static void IRAM_ATTR sensor_isr_handler(void* arg)
{
    xSemaphoreGiveFromISR(xHandleSemphSensor, pdFALSE);
}

void vTaskIndicador(void *pvParameters)
{
    while (1)
    {     
        switch (status_conexao){
        case SEM_CREDENCIAL:
            gpio_set_level(LED, !gpio_get_level(LED));
            vTaskDelay(pdMS_TO_TICKS(330));
            break;
        case DESCONECTADO:
            gpio_set_level(LED, !gpio_get_level(LED));
            vTaskDelay(pdMS_TO_TICKS(1000));
            break;
        case CONECTADO:
            gpio_set_level(LED, pdTRUE);
            vTaskDelay(pdMS_TO_TICKS(10000));
            break;
        }
    }
}


void vTaskSensor(void *pvParameters)
{
    while (1){
        xSemaphoreTake( xHandleSemphSensor, portMAX_DELAY );
        vazao++;
        // ESP_LOGW(TAG, "vazão:%d", vazao);
    }    
}

void vTaskBomba(void *pvParameters)
{
    int cont;
    while(1){
        cont = 0;
        for(int i = 0; i<10; i++){
            vazao = 0;
            vTaskDelay(pdMS_TO_TICKS(1000));
            if(!vazao) cont++;
        }
        if ((cont > 7) && (!status_bomba)){
            set_solenoides(TAG_BOMBA, "1");
        } else if ((cont > 7) && (status_bomba)){
            mqtt_app_publish(MQTT_TOPIC, "Parar irrigação por estar sem agua");
            ESP_LOGW(TAG, "Medida de proteção");
            fechaAgua();
        }
    }
}

void acionamento_agendado(void){
    char *dado;
    dado = malloc(9*sizeof(char));
    nvs_app_get(TAG_AGEN_VASOS, dado, 's');
    // mqtt_app_publish("{"a":\"%s\"}", dado);
    set_solenoides(TAG_AGEN_VASOS, dado);
    dado = malloc(3*sizeof(char));
    nvs_app_get(TAG_AGEN_TEMPO, dado, 's');
    // ESP_LOGW(TAG, "Liga agua");
    for (int i=0; i<atoi(dado) && status_registro; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(1000*60));
    }
    
    fechaAgua();
}

void vTaskSntp (void *pvParameters){
    static time_t now;
    char aux[3];
    int minutesNow, minutesScheduling, minutesDif;
    int delay;
    char horario[6];
    struct tm timeinfo;
    while (1)
    {
        time(&now);
        //Set timezone to Brazil Standard Time
        setenv("TZ", "UTC+3", 1);
        tzset();
        localtime_r(&now, &timeinfo);
        // Is time set? If not, tm_year will be (1970 - 1900).
        if (timeinfo.tm_year < (2023 - 1900)) {
            ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
            obtain_time();
            // update 'now' variable with current time
            time(&now);
        } else {
            localtime_r(&now, &timeinfo);
            strftime(aux, sizeof(aux), "%H", &timeinfo);
            minutesNow = atoi(aux)*60;
            strftime(aux, sizeof(aux), "%M", &timeinfo);
            minutesNow += atoi(aux);
            minutesScheduling = -1;

            // ESP_LOGI(TAG, "Tempo em minutos: %d", minutesNow);
            
            if (nvs_app_get(TAG_AGEN_HORARIO, horario, 's')){
                ESP_LOGI(TAG, "%s", horario);
                minutesScheduling = atoi(&horario[3]);
                horario[2] = '\0';
                minutesScheduling += atoi(horario)*60;
                // ESP_LOGI(TAG, "Tempo em minutos agendado: %d", minutesScheduling);
            } else {
                ESP_LOGI(TAG, "Sem horário salvo");
            }

            delay = TEMPO_VERIF;
            if (minutesScheduling > -1){
                minutesDif = minutesScheduling - minutesNow;
                // ESP_LOGW(TAG, "Diferença do tempo: %d", minutesDif);
                if ((minutesDif < 2) && (minutesDif > -2)){
                    acionamento_agendado();
                } else if ((minutesDif > 0) && (minutesDif <= TEMPO_VERIF)){
                    delay = minutesDif;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(1000*60*delay));
        }
    }
    
}

void wifi_app_connected(void)
{
    mqtt_app_start();
    xTaskCreate(vTaskSntp, "SNTP", 2048, NULL, 1, &xHandleSntp);
    xTaskCreate(vTaskSensor, "Cont Sensor", 2048, NULL, 1, &xHandleSensor);
    vTaskSuspend(xHandleSensor);
    xTaskCreate(vTaskBomba, "Bomba", 2048, NULL, 1, &xHandleBomba);
    vTaskSuspend(xHandleBomba);
    //habilitar interrupção
    xHandleSemphSensor = xSemaphoreCreateCounting(5, 0);
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(SENSOR, sensor_isr_handler, NULL);
}

void http_server_receive_post(int tam, char *data)
{
    // ESP_LOGI(TAG, "=========== RECEIVED DATA MAIN ==========");
    // ESP_LOGI(TAG, "%.*s", tam, data);
    // ESP_LOGI(TAG, "====================================");
    
    // extraindo SSID
    ssid = extractJson(data, "ssid");

    // extraindo PASS
    pass = extractJson(data, "pass");

    ESP_LOGI(TAG, "=========== SSID e PASS ==========");
    ESP_LOGI(TAG, "SSID:%s", ssid);
    ESP_LOGI(TAG, "SSID:%s", pass);
    credencial_wifi = true;
}

static void initialise_gpio(void)
{
    gpio_config_t io_conf = {0};
    
    //GPIO INPUT
    io_conf.pin_bit_mask = BIT64(SENSOR);
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = BIT64(BUTTON);
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    //GPIO OUTPUT
    io_conf.pin_bit_mask = BIT64(LED);
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = BIT64(BOMBA);
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = BIT64(VASO1);
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = BIT64(VASO2);
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = BIT64(VASO3);
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = BIT64(VASO4);
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = BIT64(VASO5);
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = BIT64(VASO6);
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = BIT64(VASO7);
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = BIT64(VASO8);
    gpio_config(&io_conf);
}

bool check_button(void)
{
    if (!gpio_get_level(BUTTON)){
        return true;
    }
    return false;
}

void clear_credencial_wifi(void)
{
    nvs_app_clear("ssid");
    nvs_app_clear("pass");
}

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

    initialise_gpio();

    xTaskCreate(vTaskIndicador, "Indicador", 2048, NULL, 1, &xHandleIndicador);

    if (check_button()) {
       clear_credencial_wifi(); 
    }
    
    ssid = malloc(50*sizeof(char));
    pass = malloc(20*sizeof(char));
    if (!nvs_app_get("ssid", ssid, 's') || !nvs_app_get("pass", pass, 's')){
        status_conexao = SEM_CREDENCIAL;
        wifi_init_softap();
        start_http_server();

        while (1){
            vTaskDelay(pdMS_TO_TICKS(500));
            if (credencial_wifi){
                wifi_stop();
                nvs_app_set("ssid", ssid, 's');
                nvs_app_set("pass", pass, 's');                
                break;
            }
        
        }
    }
    status_conexao = DESCONECTADO;
    wifi_init_sta(ssid, pass);
}
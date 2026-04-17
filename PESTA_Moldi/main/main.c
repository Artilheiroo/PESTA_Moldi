#include "includes_defines.h"

static const char *TAG = "SISTEMA_MONITORIZACAO"; // Tag para os logs (ajuda a identificar quem enviou a mensagem para o PC)

/*void init(void)
{
    gpio_reset_pin(USER_LED_GPIO); //reinicia o pino do led
    gpio_set_direction(USER_LED_GPIO, GPIO_MODE_OUTPUT); //pino do led como saída

    TickType_t xLastWakeTime= xTaskGetTickCount(); //define o tempo de inicio
    const TickType_t xFrequencia = pdMS_TO_TICKS(PERIODO_LEeENVIA_MS); //traduz tempo para ticks

    int estado_led = 0;
}*/

esp_err_t init_cartao_sd()
{
    ESP_LOGI(TAG, "A iniciar o barramento SDMMC...");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true, //se o cartão não for formatado, formata o cartão
        .max_files = 5, //numero de arquivos maximo abertos simultaneamente
        .allocation_unit_size = 16 * 1024 //tamanho da unidade de alocação
    };

    sdmmc_card_t *card;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    //Virtual File System (VFS) para o cartão SD
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao montar o cartão SD: %s", ret);
        return ret;
    }
    ESP_LOGI(Tag, "Cartão SD montado com sucesso!");
    
    //criar cabeçalho do ficheiro
    FILE *f = fopen("/sdcard/teste.csv", "r");
    if (f == NULL) { //ficheiro não existe
        f = fopen("/sdcard/teste.csv", "w"); //criar ficheiro
        if (f != NULL) {
            fprintf(f, "LED, ESTADO\n"); //titulos
            fclose(f);
        }
    }else{
        fclose(f);
    }

    return ESP_OK;
}

void app_main(void)
{
    gpio_reset_pin(USER_LED_GPIO); //reinicia o pino do led
    gpio_set_direction(USER_LED_GPIO, GPIO_MODE_OUTPUT); //pino do led como saída

    TickType_t xLastWakeTime= xTaskGetTickCount(); //define o tempo de inicio
    const TickType_t xFrequencia = pdMS_TO_TICKS(PERIODO_LEeENVIA_MS); //traduz tempo para ticks

    int estado_led = 0;

    if(init_cartao_sd()!= ESP_OK)
    {
        ESP_LOGE(TAG, "Sem armazenamento local!");
    }

    while (1) //loop infinito
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequencia); //começa a contagem de tempo (garante a periocidade exata de 1 minuto)

        estado_led = !estado_led; //inverte o estado do led
        gpio_set_level(USER_LED_GPIO, estado_led); //liga ou desliga o led

        FILE *f = fopen("/sdcard/teste.csv", "a"); // "a" (append) adiciona novas informações ao fim do ficheiro
        if(f != NULL){
            fprintf(f, "USER_LED, %s\n", estado_led ? "ON" : "OFF");
            fclose(f);

            ESP_LOGI(TAG,"Dados guardados no ficheiro teste.csv!");
        }else {
            ESP_LOGE(TAG,"Erro abrir o ficheiro para escrita!");
        }

        //ESP_LOGI("Teste_Pisca", "LED %s", estado_led ? "ON" : "OFF"); //Mostra estado do led
    }
}

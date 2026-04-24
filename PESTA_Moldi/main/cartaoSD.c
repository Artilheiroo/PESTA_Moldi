#include "includes_defines.h"

static const char *TAG = "CARTAO SD";

/*===============================
            CARTAO_SD
===============================*/

esp_err_t init_cartao_sd()
{
    ESP_LOGI(TAG, "A iniciar o barramento SDMMC...");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true, //se o cartão não for formatado, formata o cartão
        .max_files = 5, //numero de arquivos maximo abertos simultaneamente
        .allocation_unit_size = 16 * 1024 //tamanho da unidade de alocação (16KB)
    };

    sdmmc_card_t *card; //estrutura que guarda as informações fisicas do cartao
    //define o host e os pinos padrão para SDMMC
    sdmmc_host_t host = SDMMC_HOST_DEFAULT(); 
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    //Virtual File System (VFS) para o cartão SD
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao montar o cartão SD: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Cartão SD montado com sucesso!");
    
    //criar cabeçalho do ficheiro
    FILE *f = fopen("/sdcard/teste.csv", "r");
    if (f == NULL) { //ficheiro não existe
        f = fopen("/sdcard/teste.csv", "w"); //criar ficheiro
        if (f != NULL) {
            fprintf(f, "   DATA     |   HORA   \r\n"); //titulos
         // fprintf(f, " --/--/----   --:--:-- \r\n"); PARA VIZUALIZAR A TABELA
            fclose(f);
        }
    }else{
        fclose(f);
    }

    return ESP_OK;
}

/*===============================
        MARCADORES DO SD
===============================*/

long ler_ponteiro()// as funções fseek e ftell pedem variaveis do tipo long
{
    FILE *f_pont = fopen("/sdcard/ponteiro.txt", "r");
    if(f_pont == NULL) return 0; //Se o ficheiro não existir, começa a ler do 0

    long posicao = 0; //variavel para guardar o num que vamos ler

    fscanf(f_pont, "%ld", &posicao); //lê o numero dentro do ficheiro e guarda em posicao
    fclose(f_pont);

    return posicao; //devolve o valor exato do byte aonde ficamos
}

void guardar_ponteiro (long posicao)
{
    FILE *f_pont = fopen("/sdcard/ponteiro.txt", "w");
    if(f_pont != NULL)
    {
        fprintf(f_pont, "%ld", posicao); //escreve no ficheiro o num da nova posicao dentro do ficheiro
        fclose(f_pont);
    }
}
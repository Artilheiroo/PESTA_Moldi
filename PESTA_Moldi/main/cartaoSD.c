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
    FILE *f = fopen("/sdcard/dados.csv", "r");
    if (f == NULL) { //ficheiro não existe
        f = fopen("/sdcard/dados.csv", "w"); //criar ficheiro
        if (f != NULL) {
            fprintf(f, "   DATA     |    HORA    | CORRENTE | ESTADO | TEMPERATURA | HUMIDADE \r\n"); //titulos
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

/*===============================
     LIMPEZA DO CARTÃO SD (24h)
===============================*/

bool limpar_sd(void)
{
    // 1. Verificar o tamanho atual do ficheiro CSV
    FILE *f = fopen("/sdcard/dados.csv", "r");
    if (f == NULL) {
        ESP_LOGW(TAG, "Ficheiro dados.csv não existe, nada a limpar.");
        return true; // não há nada a limpar
    }

    fseek(f, 0, SEEK_END);
    long tamanho_ficheiro = ftell(f);
    fclose(f);

    // 2. Comparar com o ponteiro de envio (posição até onde os dados já foram enviados)
    long ponteiro_envio = ler_ponteiro();

    if (ponteiro_envio < tamanho_ficheiro) {
        // Ainda há dados por enviar — NÃO limpar para não perder dados
        ESP_LOGW(TAG, "Limpeza adiada: faltam %ld bytes por enviar ao servidor.", tamanho_ficheiro - ponteiro_envio);
        return false;
    }

    // 3. Todos os dados foram enviados — apagar e recriar o ficheiro
    remove("/sdcard/dados.csv");
    remove("/sdcard/ponteiro.txt");

    // 4. Recriar ficheiro com cabeçalho
    f = fopen("/sdcard/dados.csv", "w");
    if (f != NULL) {
        fprintf(f, "   DATA     |    HORA    | CORRENTE | ESTADO | TEMPERATURA | HUMIDADE \r\n");
        fclose(f);
    }

    // 5. Resetar ponteiro de envio para 0
    guardar_ponteiro(0);

    ESP_LOGI(TAG, "Cartão SD limpo com sucesso! Ficheiro CSV resetado.");
    return true;
}
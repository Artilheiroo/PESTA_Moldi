#include "includes_defines.h"
#include "rede.h"

static const char *TAG = "SISTEMA_MONITORIZACAO"; // Tag para os logs ( identificar quem enviou a mensagem para o PC)
TaskHandle_t handle_tarefa_tcp = NULL; //o nosso sinal de comunicação entre as tasks


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
          Enviar Dados
===============================*/

esp_err_t enviar_linha(const char* linha)
{
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(IP_SERVIDOR); //converte o IP para binário
    dest_addr.sin_family = AF_INET; //define IPv4
    dest_addr.sin_port = htons(PORTA_SERVIDOR); //converte a porta para o formato de rede (big-endian)

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    //confirmações da socket (fecha sempre a sock caso falhe para n gastar a memória)
    if (sock < 0) return ESP_FAIL; //não foi possível criar o socket

    if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) //ligação ao servidor falhou
    { 
        close(sock);
        return ESP_FAIL;
    }

    if (send(sock, linha, strlen(linha), 0) < 0) //erro ao enviar os dados
    {
        close(sock);
        return ESP_FAIL;
    }

    char buffer_resposta[16];// sting provisoria para receber resposta
    int tamanho = recv(sock, buffer_resposta, sizeof(buffer_resposta) - 1, 0); //fica a espera de resposta e mete o numero de carateres em tamanho
    esp_err_t resultado = ESP_FAIL;

    if(tamanho > 0)
    {
        buffer_resposta[tamanho] = 0; // anula string temporaria
        if(strstr(buffer_resposta,"OK") != NULL) //verifica se o servidor enviou "OK" (fez a ligação)
        {
            resultado = ESP_OK;
        }
    }else {
        ESP_LOGW(TAG,"O servidor não respondeu a tempo! Timeout.");
    }

    close(sock); // fechar a socket
    return resultado;
}


/*===============================
            Task TCP
===============================*/

void task_sincro_tcp(void *pvParameters)
{
    char linha_a_enviar[256];
    
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //espera que a app_main diga que pode enviar

        //confimar se tem conetividade antes de abrir o ficheiro
        if(!rede_disponivel) continue; //se não está ligado a net, volta a adormecer

        FILE *f_dados = fopen("/sdcard/teste.csv", "r");
        if (f_dados == NULL)
        {
            continue; //senão consegue abrir ficheiro volta a adormecer
        }

        long ult_pos = ler_ponteiro(); //salta para o ponto onde ficou na ult vez
        fseek(f_dados, ult_pos, SEEK_SET);

        while (fgets(linha_a_enviar, sizeof(linha_a_enviar), f_dados) != NULL) //lê e envia todas as linhas pendentes
        {
            if(enviar_linha(linha_a_enviar) == ESP_OK)
            {
                ult_pos = ftell(f_dados);
                guardar_ponteiro(ult_pos);
                ESP_LOGI(TAG, "Dados enviados com sucesso.");
            }else{
                ESP_LOGW(TAG, "Falha ao enviar dados.");
                break; //a rede caiu a meio, quebra o ciclo e volta ao inicio (dormir)
            }
        }

        fclose(f_dados);
    }
}

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
            fprintf(f, "LED, ESTADO\r\n\r\n"); //titulos
            fclose(f);
        }
    }else{
        fclose(f);
    }

    return ESP_OK;
}


/*===============================
             MAIN
===============================*/

void app_main(void) 
{
    gpio_reset_pin(USER_LED_GPIO); //reinicia o pino do led
    gpio_set_direction(USER_LED_GPIO, GPIO_MODE_OUTPUT); //pino do led como saída

    if(init_cartao_sd()!= ESP_OK)
    {
        ESP_LOGE(TAG, "Sem armazenamento local!");
    }

    init_ethernet();

    if (init_i2c() == ESP_OK) {
        ESP_LOGI(TAG, "I2C arrancou com sucesso!");
    } else {
        ESP_LOGE(TAG, "Falha ao arrancar o I2C!");
    }

    xTaskCreatePinnedToCore(task_sincro_tcp, "TaskTCP", 4096, NULL, 5, &handle_tarefa_tcp, 1); //cria a task TCP no Core 1 
  
    esperar_start();

    //acertar_rel(2026, 4, 24, 12, 44, 1); APENAS PARA ACERTAR O RELOGIO

    TickType_t xLastWakeTime= xTaskGetTickCount(); //define o tempo de inicio
    const TickType_t xFrequencia = pdMS_TO_TICKS(PERIODO_LEITURA_MS); //traduz tempo para ticks

    int estado_led = 0; //ESTADO LED PARA TESTE
    int contador_mandar_BD = 0;

    while (1) //loop infinito
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequencia); //começa a contagem de tempo (garante a periocidade exata de 1 minuto)

        contador_mandar_BD ++;

        char hora_atual[16];
        ler_relogio(hora_atual);
        ESP_LOGI(TAG, "Hora atual RTC: %s", hora_atual);

        estado_led = !estado_led; //inverte o estado do led
        gpio_set_level(USER_LED_GPIO, estado_led); //liga ou desliga o led

        FILE *f = fopen("/sdcard/teste.csv", "a"); // "a" (append) adiciona novas informações ao fim do ficheiro
        if(f != NULL){
            fprintf(f, "USER_LED, %s\n", estado_led ? "ON" : "OFF");
            fclose(f);

            ESP_LOGI(TAG,"Dados guardados no ficheiro teste.csv!");

            if(contador_mandar_BD == 12)
            {
                if(handle_tarefa_tcp != NULL)
                {
                    xTaskNotifyGive(handle_tarefa_tcp); //diz a Task TCP que novos dados foram guardados no SD
                    contador_mandar_BD = 0;
                }
            }

        }else {
            ESP_LOGE(TAG,"Erro abrir o ficheiro para escrita!");
        }
    }
}

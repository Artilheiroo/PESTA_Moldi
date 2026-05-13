#include "includes_defines.h"

static const char *TAG = "TCP";

TaskHandle_t handle_tarefa_tcp = NULL;

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
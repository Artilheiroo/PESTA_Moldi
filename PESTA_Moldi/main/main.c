#include "includes_defines.h"

static const char *TAG = "SISTEMA_MONITORIZACAO"; // Tag para os logs ( identificar quem enviou a mensagem para o PC)
static const char *TAG2 = "I2C";

/*===============================
             MAIN
===============================*/

void app_main(void) 
{

    if(init_cartao_sd()!= ESP_OK)
    {
        ESP_LOGE(TAG, "Sem armazenamento local!");
    }

    init_ethernet();

    if (init_i2c() == ESP_OK) {
        ESP_LOGI(TAG2, "I2C arrancou com sucesso!");
    } else {
        ESP_LOGE(TAG2, "Falha ao arrancar o I2C!");
    }

    xTaskCreatePinnedToCore(task_sincro_tcp, "TaskTCP", 4096, NULL, 5, &handle_tarefa_tcp, 1); //cria a task TCP no Core 1 

    esperar_start();

    //acertar_rel(2026, 4, 24, 12, 44, 1); APENAS PARA ACERTAR O RELOGIO

    TickType_t xLastWakeTime= xTaskGetTickCount(); //define o tempo de inicio
    const TickType_t xFrequencia = pdMS_TO_TICKS(PERIODO_LEITURA_MS); //traduz tempo para ticks

    int contador_mandar_BD = 0;

    while (1) //loop infinito
    {



            vTaskDelayUntil(&xLastWakeTime, xFrequencia); //começa a contagem de tempo (garante a periocidade exata de 1 minuto)

            contador_mandar_BD ++;

            char hora_atual[16];
            char data_atual[16];

            ler_relogio(data_atual, hora_atual);

            FILE *f = fopen("/sdcard/teste.csv", "a"); // "a" (append) adiciona novas informações ao fim do ficheiro
            if(f != NULL){
                fprintf(f, "%s    %s  \n", data_atual, hora_atual);
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

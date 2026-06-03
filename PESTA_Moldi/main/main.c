#include "includes_defines.h"

static const char *TAG = "SISTEMA_MONITORIZACAO"; // Tag para os logs ( identificar quem enviou a mensagem para o PC)
static const char *TAG2 = "I2C";

// 24h em ciclos: (24 * 60 * 60 * 1000) / PERIODO_LEITURA_MS
#define CICLOS_24H  (24 * 60 * 60 * 1000 / PERIODO_LEITURA_MS)

/*===============================
             MAIN
===============================*/

void app_main(void) 
{

    if(init_cartao_sd()!= ESP_OK)
    {
        ESP_LOGE(TAG, "Sem armazenamento local!");
    }

    //init_ethernet();
    init_wifi();

    if (init_i2c() == ESP_OK) {
        ESP_LOGI(TAG2, "I2C arrancou com sucesso!");
    } else {
        ESP_LOGE(TAG2, "Falha ao arrancar o I2C!");
    }

    init_ADC(); // inicializar o leitor de sensores (ADC)
    init_contacto_aux(); // inicializar o contacto auxiliar da máquina
    init_am2320(); // inicializar o sensor de humidade/temperatura (partilha I2C com DS3231)

    xTaskCreatePinnedToCore(task_sincro_tcp, "TaskTCP", 4096, NULL, 5, &handle_tarefa_tcp, 1); //cria a task TCP no Core 1 

    init_botao(); // inicializar o botão toggle (START/STOP) — cria task de background

    sincronizar_ntp(); //acerta o DS3231 automaticamente via NTP

    TickType_t xLastWakeTime= xTaskGetTickCount(); //define o tempo de inicio
    const TickType_t xFrequencia = pdMS_TO_TICKS(PERIODO_LEITURA_MS); //traduz tempo para ticks

    int contador_mandar_BD = 0;
    int ciclos_24h = 0; // contador para limpeza do SD
    bool ntp_acertado = false; //flag para saber se o relógio já foi acertado por NTP

    while (1) //loop infinito
    {
            vTaskDelayUntil(&xLastWakeTime, xFrequencia); //começa a contagem de tempo (garante a periocidade exata de 1 minuto)

            // Se o sistema está pausado (botão), não faz nada — mas mantém o timing
            if (!sistema_ativo) {
                continue;
            }

            // Se o NTP ainda não acertou o relógio, tenta novamente a cada ciclo
            if (!ntp_acertado) {
                ntp_acertado = sincronizar_ntp();
            }

            contador_mandar_BD ++;
            ciclos_24h++;

            char hora_atual[16];
            char data_atual[16];

            ler_relogio(data_atual, hora_atual);

            float corrente_atual = ler_corrente_rms(); // ler corrente do sensor
            int estado_maquina = ler_estado_maquina() ? 1 : 0; // aqui testa se true estado_maquina=1 se false estado_maquina=0

            // Ler sensor de humidade e temperatura
            float temperatura = 0.0f;
            float humidade = 0.0f;
            bool am2320_ok = (ler_am2320(&temperatura, &humidade) == ESP_OK);

            FILE *f = fopen("/sdcard/teste.csv", "a"); // "a" (append) adiciona novas informações ao fim do ficheiro
            if(f != NULL){
                if (am2320_ok) {
                    fprintf(f, "%s    %s    %.2f    %d    %.1f    %.1f\n", data_atual, hora_atual, corrente_atual, estado_maquina, temperatura, humidade);
                } else {
                    fprintf(f, "%s    %s    %.2f    %d    --    --\n", data_atual, hora_atual, corrente_atual, estado_maquina);
                }
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

            // Limpeza do cartão SD a cada 24h (só se todos os dados já foram enviados)
            if (ciclos_24h >= CICLOS_24H)
            {
                if (limpar_cartao_sd()) {
                    ciclos_24h = 0; // resetar contador após limpeza bem sucedida
                } else {
                    // Limpeza adiada — tentar novamente no próximo ciclo
                    // Não resetar contador para tentar em cada ciclo até conseguir
                    ESP_LOGW(TAG, "Limpeza do SD adiada, dados pendentes por enviar.");
                }
            }
    }
}

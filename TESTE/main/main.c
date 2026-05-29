
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"
 
static const char *TAG = "ADC";
 
#define ADC_UNIT            ADC_UNIT_1
#define ADC_CHANNEL         ADC_CHANNEL_7   /* GPIO35 (Placa Olimex) */
#define ADC_ATTEN           ADC_ATTEN_DB_12 
#define ADC_BITWIDTH        ADC_BITWIDTH_DEFAULT

#define PERIODO_LEITURA_MS 1000 //tempo de envio de mensagem
#define TEMPO_AMOSTRAS_US 100000 //us (5 ciclos a 50Hz)


// STC013-030 A
#define  FATOR_CONVERSAO_TC   30.0f

adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t cali_handle = NULL;

void init_ADC()
{
    //iniciar a unidade ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    //configurar o canal do ADC
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    //configuração da calibração
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
        .default_vref= 1100,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle));
}


void app_main()
{
    init_ADC();

    TickType_t xLastWakeTime= xTaskGetTickCount(); //define o tempo de inicio
    const TickType_t xFrequencia = pdMS_TO_TICKS(PERIODO_LEITURA_MS); //traduz tempo para ticks

    while(1)
    {
        uint64_t tempo_inicio = esp_timer_get_time();

        float soma_quadrados = 0;
        float soma_tensao = 0;
        int num_amostras= 0;
        int adc_cru = 0;
        int tensao_real = 0;

        float tensao_rms= 0;
        float corrente_rms = 0;
        float tensao_real_V =0;

        while((esp_timer_get_time()- tempo_inicio) < TEMPO_AMOSTRAS_US) //se ainda n passou 100ms faz as leituras
        {
            adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_cru); //le o valor bruto

            adc_cali_raw_to_voltage(cali_handle, adc_cru, &tensao_real);

            tensao_real_V = tensao_real / 1000.0f; // converter para V

            soma_tensao += tensao_real_V;                       // acumular para calcular media (offset DC real)
            soma_quadrados += (tensao_real_V * tensao_real_V);  // acumular quadrados
            
            num_amostras++;

        }

        if(num_amostras > 0)
        {
            float media = soma_tensao / num_amostras;               // offset DC real do circuito
            float media_quadrados = soma_quadrados / num_amostras;   // media dos quadrados

            // RMS AC = desvio padrao = sqrt(mean(x^2) - mean(x)^2)
            float variancia = media_quadrados - (media * media);
            if(variancia < 0.0f) variancia = 0.0f;  // protecao contra erro numerico

            tensao_rms = sqrtf(variancia);
            
            corrente_rms = tensao_rms * FATOR_CONVERSAO_TC; //obter a corrente

            if(corrente_rms < 0.15f) corrente_rms = 0.0f;

            ESP_LOGI(TAG, "Amostras: %d | Vrms: %.4f V | Corrente: %.2f A", num_amostras, tensao_rms, corrente_rms);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequencia); //começa a contagem de tempo (garante a periodicidade exata de 1000ms)
    }
}
 
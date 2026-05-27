
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
#define TEMPO_AMOSTRAS_US 100000 //us (5 ciclos a 5Hz)
#define CENTRO_VIRTUAL 1.65f //mV (offset)


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
        int num_amostras= 0;
        int adc_cru = 0;
        int tensao_real = 0;

        float media_quadrados = 0;
        float tensao_rms= 0;
        float corrente_rms = 0;
        float tensao_real_V =0;
        float sinal_real= 0;

        while((esp_timer_get_time()- tempo_inicio) < TEMPO_AMOSTRAS_US) //se ainda n passou 100ms faz as leituras
        {
            adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_cru); //le o valor bruto

            //ESP_LOGW(TAG, "adc_cru= %d", adc_cru); 
            adc_cali_raw_to_voltage(cali_handle, adc_cru, &tensao_real);

            tensao_real_V = tensao_real / 1000.0f; // converter para V
            //ESP_LOGW(TAG, "tensao_real= %.2f", tensao_real_V);

            sinal_real = tensao_real_V - CENTRO_VIRTUAL; //remover o offset para recuperar a onda centrda em 0
            //ESP_LOGW(TAG, "sinal_real= %.2f", sinal_real);

            soma_quadrados += (sinal_real * sinal_real);
            //ESP_LOGW(TAG, "soma_quadrados= %.2f", soma_quadrados);
            
            num_amostras++;

        }

        if(num_amostras > 0)
        {
            media_quadrados = (float)soma_quadrados / num_amostras;

            tensao_rms = sqrtf(media_quadrados);//raiz qudrada para obter o rms (em mV)
            
            corrente_rms= tensao_rms * FATOR_CONVERSAO_TC; //obter a corrente

            if(corrente_rms<0.15f) corrente_rms=0.0f;
        }

        ESP_LOGI(TAG, "Amostras: %d | Vrms: %.2f V | Corrente: %.2f A", 
            num_amostras, tensao_rms , corrente_rms);

        vTaskDelayUntil(&xLastWakeTime, xFrequencia); //começa a contagem de tempo (garante a periocidade exata de 5ms)
    }
}
 
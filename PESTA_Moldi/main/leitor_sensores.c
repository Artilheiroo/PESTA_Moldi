#include "includes_defines.h"

/*===============================
     LEITOR DE SENSORES (ADC)
===============================*/

static const char *TAG_ADC = "ADC";

#define ADC_UNIT            ADC_UNIT_1
#define ADC_CHANNEL         ADC_CHANNEL_7   /* GPIO35 (Placa Olimex) */
#define ADC_ATTEN           ADC_ATTEN_DB_12 
#define ADC_BITWIDTH        ADC_BITWIDTH_DEFAULT

#define TEMPO_AMOSTRAS_US   100000  // 100ms (5 ciclos a 50Hz)

// STC013-030 A
#define FATOR_CONVERSAO_TC  28.6f

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle = NULL;

void init_ADC(void)
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

    ESP_LOGI(TAG_ADC, "ADC inicializado com sucesso!");
}

float ler_corrente_rms(void)
{
    uint64_t tempo_inicio = esp_timer_get_time();

    float soma_quadrados = 0;
    float soma_tensao = 0;
    int num_amostras = 0;
    int adc_cru = 0;
    int tensao_real = 0;

    float tensao_rms = 0;
    float corrente_rms = 0;
    float tensao_real_V = 0;

    while ((esp_timer_get_time() - tempo_inicio) < TEMPO_AMOSTRAS_US) //se ainda n passou 100ms faz as leituras
    {
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_cru); //le o valor bruto

        adc_cali_raw_to_voltage(cali_handle, adc_cru, &tensao_real);

        tensao_real_V = tensao_real / 1000.0f; // converter para V

        soma_tensao += tensao_real_V;                       // acumular para calcular media (offset DC real)
        soma_quadrados += (tensao_real_V * tensao_real_V);  // acumular quadrados
        
        num_amostras++;
    }

    if (num_amostras > 0)
    {
        float media = soma_tensao / num_amostras;               // offset DC real do circuito
        float media_quadrados = soma_quadrados / num_amostras;   // media dos quadrados

        // RMS AC = desvio padrao = sqrt(mean(x^2) - mean(x)^2)
        float variancia = media_quadrados - (media * media);
        if (variancia < 0.0f) variancia = 0.0f;  // protecao contra erro numerico

        tensao_rms = sqrtf(variancia);
        
        corrente_rms = tensao_rms * FATOR_CONVERSAO_TC; //obter a corrente

        if (corrente_rms < 0.15f) corrente_rms = 0.0f;

        ESP_LOGI(TAG_ADC, "Amostras: %d | Vrms: %.4f V | Corrente: %.2f A", num_amostras, tensao_rms, corrente_rms);
    }

    return corrente_rms;
}

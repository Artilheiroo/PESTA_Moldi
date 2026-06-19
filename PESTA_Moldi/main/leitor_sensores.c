#include "includes_defines.h"

/*===============================
     LEITOR DE SENSORES (ADC)
===============================*/

static const char *TAG_ADC = "ADC";

#define ADC_UNIT            ADC_UNIT_1
#define ADC_CHANNEL         ADC_CHANNEL_7   /* GPIO35 (Placa Olimex) 5  a contar do fim */
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

/*===============================
   CONTACTO AUXILIAR (ON/OFF)
===============================*/

#define CONTACTO_AUX_GPIO   GPIO_NUM_36  // GPIO36 (4 a contar do fim)

void init_contacto_aux(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONTACTO_AUX_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,    // GPIO36 não suporta pull-up interno
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // GPIO36 não suporta pull-down interno
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

bool ler_estado_maquina(void)
{
    return gpio_get_level(CONTACTO_AUX_GPIO) == 1;  //testa valor lógico e retorna true ou false
}



/*===============================
   SENSOR AM2320B (TEMP/HUM)
===============================*/

static const char *TAG_AM = "AM2320";

#define AM2320_MAX_TENTATIVAS  3  // tentativas de wakeup antes de desistir

static i2c_master_dev_handle_t am2320_dev = NULL;

// CRC-16 Modbus — valida integridade dos dados recebidos do AM2320
static uint16_t crc16_modbus(const uint8_t *dados, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= dados[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void init_am2320(void)
{
    i2c_master_bus_handle_t bus = obter_i2c_bus();
    if (bus == NULL) {
        ESP_LOGE(TAG_AM, "Barramento I2C não inicializado!");
        return;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AM2320_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &am2320_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_AM, "Falha ao adicionar AM2320 ao I2C: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG_AM, "Sensor AM2320B inicializado no barramento I2C (addr=0x%02X)", AM2320_ADDR);
}

esp_err_t ler_temp_hum(float *temperatura, float *humidade)
{
    if (am2320_dev == NULL) {
        ESP_LOGE(TAG_AM, "Sensor não inicializado! Chamar init_am2320() primeiro.");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = ESP_FAIL;

    i2c_lock(); // proteger barramento partilhado com DS3231

    // 1. Acordar o sensor com retentativas (o AM2320 entra em sleep após 3s)
    //    O primeiro transmit dá quase sempre NACK — é comportamento normal
    uint8_t wakeup = 0x00;
    bool acordou = false;

    for (int tentativa = 0; tentativa < AM2320_MAX_TENTATIVAS; tentativa++) {
        i2c_master_transmit(am2320_dev, &wakeup, 1, 50); // NACK esperado
        esp_rom_delay_us(1000); // esperar >800µs para o sensor acordar

        // Verificar se o sensor respondeu enviando o comando
        uint8_t cmd[] = {0x03, 0x00, 0x04};
        ret = i2c_master_transmit(am2320_dev, cmd, sizeof(cmd), 100);
        if (ret == ESP_OK) {
            acordou = true;
            break;
        }
        ESP_LOGD(TAG_AM, "Tentativa %d de wakeup falhou, a tentar novamente...", tentativa + 1);
        vTaskDelay(pdMS_TO_TICKS(2)); // breve pausa entre tentativas
    }

    if (!acordou) {
        i2c_unlock();
        ESP_LOGW(TAG_AM, "Sensor não respondeu após %d tentativas", AM2320_MAX_TENTATIVAS);
        return ESP_ERR_TIMEOUT;
    }

    // 2. Esperar que o sensor prepare os dados (>1.5ms conforme datasheet)
    vTaskDelay(pdMS_TO_TICKS(3));

    // 3. Ler resposta: func_code(1) + data_len(1) + hum_h + hum_l + temp_h + temp_l + crc_l + crc_h
    uint8_t resp[8] = {0};
    ret = i2c_master_receive(am2320_dev, resp, sizeof(resp), 100);

    i2c_unlock(); // libertar barramento o mais cedo possível

    if (ret != ESP_OK) {
        ESP_LOGW(TAG_AM, "Erro ao ler dados: %s", esp_err_to_name(ret));
        return ret;
    }

    // 4. Verificar cabeçalho da resposta
    if (resp[0] != 0x03 || resp[1] != 0x04) {
        ESP_LOGW(TAG_AM, "Resposta inválida: func=0x%02X len=0x%02X", resp[0], resp[1]);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // 5. Verificar CRC-16 Modbus (sobre os primeiros 6 bytes)
    uint16_t crc_recebido = (resp[7] << 8) | resp[6]; // CRC vem em little-endian
    uint16_t crc_calculado = crc16_modbus(resp, 6);

    if (crc_recebido != crc_calculado) {
        ESP_LOGW(TAG_AM, "CRC inválido! Recebido=0x%04X Calculado=0x%04X", crc_recebido, crc_calculado);
        return ESP_ERR_INVALID_CRC;
    }

    // 6. Converter valores brutos para float
    uint16_t hum_raw = (resp[2] << 8) | resp[3];
    uint16_t temp_raw = (resp[4] << 8) | resp[5];

    float hum = hum_raw / 10.0f;
    float temp;

    if (temp_raw & 0x8000) { // bit de sinal (temperatura negativa)
        temp = -(float)(temp_raw & 0x7FFF) / 10.0f;
    } else {
        temp = temp_raw / 10.0f;
    }

    // 7. Validação de gama (AM2320: -40~80°C, 0~99.9% RH)
    if (temp < -40.0f || temp > 80.0f || hum < 0.0f || hum > 99.9f) {
        ESP_LOGW(TAG_AM, "Valores fora de gama: T=%.1f°C H=%.1f%%", temp, hum);
        return ESP_ERR_INVALID_RESPONSE;
    }

    *temperatura = temp;
    *humidade = hum;

    //ESP_LOGI(TAG_AM, "Temperatura: %.1f°C | Humidade: %.1f%%", *temperatura, *humidade);

    return ESP_OK;
}

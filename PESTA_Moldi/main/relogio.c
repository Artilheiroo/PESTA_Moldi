#include "includes_defines.h"
#include "esp_netif_sntp.h"
#include <time.h>
#include <sys/time.h>

static SemaphoreHandle_t i2c_mutex = NULL;

/*===============================
        RELÓGIO (DS3231)
===============================*/

//conversões BCD <-> DEC 
uint8_t BCD2DEC(uint8_t valor)
{
    return ((valor/16*10)+ (valor % 16));
}
uint8_t DEC2BCD(uint8_t valor)
{
    return ((valor/10*16)+ (valor % 10));
}

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t rtc_dev = NULL;

i2c_master_bus_handle_t obter_i2c_bus(void)
{
    return i2c_bus;
}

void i2c_lock(void)
{
    if (i2c_mutex) xSemaphoreTake(i2c_mutex, portMAX_DELAY);
}

void i2c_unlock(void)
{
    if (i2c_mutex) xSemaphoreGive(i2c_mutex);
}

esp_err_t init_i2c(void)
{
    // Criar mutex para proteger acessos concorrentes ao barramento I2C
    i2c_mutex = xSemaphoreCreateMutex();
    if (i2c_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }

    i2c_master_bus_config_t conf = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = RTC_SDA_IO, //pino aonde vão andar as informações
        .scl_io_num = RTC_SCL_IO, //pino aonde vai ser marcado o ritmo
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, //segurança do esp32
    };

    esp_err_t ret = i2c_new_master_bus(&conf, &i2c_bus);
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };

    return i2c_master_bus_add_device(i2c_bus, &dev_cfg, &rtc_dev);
}
 
void ler_relogio(char *buffer_data, char *buffer_hora)
{
    if (rtc_dev == NULL) //verifica se o motor I2C foi efetivamente inicializado
    {
        //se nao -> preenche buffer com "-" e sai
        if(buffer_hora) snprintf(buffer_hora, 16, "--:--:--"); //se nao -> preenche buffer com "-" e sai
        if(buffer_data) snprintf(buffer_data, 16, "--/--/----");
        return;
    }

    uint8_t reg_inicio = 0x00; // o primeiro registro do tempo (segundos)
    uint8_t dados[7] = {0}; // segundos, minutos, horas, dia, mes, ano

    i2c_lock();
    esp_err_t ret = i2c_master_transmit_receive(rtc_dev, &reg_inicio, 1, dados, sizeof(dados), 1000); //envia o reg_inicio e le 7 bytes para "dados" 
    i2c_unlock();

    if (ret != ESP_OK) //verifica se a comunicação falhou a meio ou tem ruído
    {
        if(buffer_hora) snprintf(buffer_hora, 16, "--:--:--"); //se nao -> preenche buffer com "-" e sai
        if(buffer_data) snprintf(buffer_data, 16, "--/--/----");
        return;
    }

    //converter de BCD para DEC para o micro imprimir os numeros corretos
    //O "& 0x7F" e "& 0x3F" servem para ignorar bits de configuração que o chip mistura com os numeros

    //hora
    uint8_t seg = BCD2DEC(dados[0] & 0x7F); 
    uint8_t min = BCD2DEC(dados[1] & 0x7F);
    uint8_t hora = BCD2DEC(dados[2] & 0x3F);

    //data
    uint8_t dia = BCD2DEC(dados[4] & 0x3F); 
    uint8_t mes = BCD2DEC(dados[5] & 0x1F);
    uint8_t ano = BCD2DEC(dados[6]);

    snprintf(buffer_hora, 16, "%02u:%02u:%02u", hora, min, seg); //escreve os numeros formatados para dentro do buffer_tempo com limite de 16 carac.
    snprintf(buffer_data, 16, "%02u/%02u/%04u", dia, mes, ano + 2000);
}


void acertar_rel(int ano, int mes, int dia, int hora, int min, int seg)
{
    if (rtc_dev == NULL) 
    {
        ESP_LOGE("DS3231", "I2C nao inicializado.");
        return;
    }

    uint8_t dados[7];
    dados[0] = DEC2BCD(seg);
    dados[1] = DEC2BCD(min);
    dados[2] = DEC2BCD(hora);
    dados[3] = 1; 
    dados[4] = DEC2BCD(dia);
    dados[5] = DEC2BCD(mes);
    dados[6] = DEC2BCD(ano - 2000);

    uint8_t tx_data[8];
    tx_data[0] = 0x00; // endereço do primeiro registo (segundos)
    memcpy(&tx_data[1], dados, sizeof(dados));

    i2c_lock();
    esp_err_t ret = i2c_master_transmit(rtc_dev, tx_data, sizeof(tx_data), 1000);
    i2c_unlock();
    if (ret == ESP_OK) 
    {
        ESP_LOGI("DS3231", "Relógio atualizado com sucesso!");
    } else {
        ESP_LOGE("DS3231", "Falha ao atualizar relogio: %s", esp_err_to_name(ret));
    }
}

/*===============================
        NTP → DS3231
===============================*/

bool sincronizar_ntp(void)
{
    // 1. Verificar se a rede está disponível (sem esperar, pois será chamada ciclicamente)
    if (!rede_disponivel) {
        ESP_LOGW("NTP", "Rede não disponível, a usar hora do DS3231.");
        return false;
    }

    // 2. Configurar e iniciar o SNTP
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);

    ESP_LOGI("NTP", "A sincronizar com pool.ntp.org...");

    // 3. Esperar pela sincronização (máx 15 segundos)
    esp_err_t ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000));

    if (ret != ESP_OK) {
        ESP_LOGW("NTP", "Timeout na sincronização NTP. A usar hora atual do DS3231.");
        esp_netif_sntp_deinit();
        return false;
    }

    // 4. Definir o fuso horário de Portugal continental (WET/WEST com horário de verão)
    setenv("TZ", "WET-0WEST,M3.5.0/1,M10.5.0", 1);
    tzset();

    // 5. Obter a hora local do sistema (já sincronizada pelo NTP)
    time_t agora;
    struct tm info_tempo;
    time(&agora);
    localtime_r(&agora, &info_tempo);

    // 6. Acertar o DS3231 com a hora NTP
    acertar_rel(
        info_tempo.tm_year + 1900,  // ano completo
        info_tempo.tm_mon + 1,      // mês (tm_mon é 0-11)
        info_tempo.tm_mday,         // dia
        info_tempo.tm_hour,         // hora
        info_tempo.tm_min,          // minuto
        info_tempo.tm_sec           // segundo
    );

    ESP_LOGI("NTP", "DS3231 acertado por NTP: %02d/%02d/%04d %02d:%02d:%02d",
             info_tempo.tm_mday, info_tempo.tm_mon + 1, info_tempo.tm_year + 1900,
             info_tempo.tm_hour, info_tempo.tm_min, info_tempo.tm_sec);

    // 7. Libertar recursos do SNTP (já não é necessário)
    esp_netif_sntp_deinit();
    return true;
}
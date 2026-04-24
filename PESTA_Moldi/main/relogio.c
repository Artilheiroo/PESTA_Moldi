#include "includes_defines.h"

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

static i2c_master_bus_handle_t rtc_i2c_bus = NULL;
static i2c_master_dev_handle_t rtc_dev = NULL;

esp_err_t init_i2c(void)
{
    i2c_master_bus_config_t conf = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = RTC_SDA_IO, //pino aonde vão andar as informações
        .scl_io_num = RTC_SCL_IO, //pino aonde vai ser marcado o ritmo
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, //segurança do esp32
    };

    esp_err_t ret = i2c_new_master_bus(&conf, &rtc_i2c_bus);
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };

    return i2c_master_bus_add_device(rtc_i2c_bus, &dev_cfg, &rtc_dev);
}
 
void ler_relogio(char *buffer_tempo)
{
    if (rtc_dev == NULL) {
        snprintf(buffer_tempo, 16, "--:--:--");
        return;
    }

    uint8_t reg_inicio = 0x00;
    uint8_t dados[3] = {0}; // segundos, minutos, horas
    esp_err_t ret = i2c_master_transmit_receive(rtc_dev, &reg_inicio, 1, dados, sizeof(dados), 1000);
    if (ret != ESP_OK) {
        snprintf(buffer_tempo, 16, "--:--:--");
        return;
    }

    uint8_t seg = BCD2DEC(dados[0] & 0x7F);
    uint8_t min = BCD2DEC(dados[1] & 0x7F);
    uint8_t hora = BCD2DEC(dados[2] & 0x3F);
    snprintf(buffer_tempo, 16, "%02u:%02u:%02u", hora, min, seg);
}


void acertar_rel(int ano, int mes, int dia, int hora, int min, int seg)
{
    if (rtc_dev == NULL) {
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

    esp_err_t ret = i2c_master_transmit(rtc_dev, tx_data, sizeof(tx_data), 1000);
    if (ret == ESP_OK) {
        ESP_LOGI("DS3231", "Relógio atualizado com sucesso!");
    } else {
        ESP_LOGE("DS3231", "Falha ao atualizar relogio: %s", esp_err_to_name(ret));
    }
}
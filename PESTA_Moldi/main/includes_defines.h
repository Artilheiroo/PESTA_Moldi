#ifndef INCLUDES_DEFINES_H
#define INCLUDES_DEFINES_H

//-------Bibliotecas standard C---------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

//-------Bibliotecas FREERTOS---------
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// -------Bibliotecas ESP-IDF DRIVERS---------
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_attr.h"
#include "esp_timer.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// -------Bibliotecas REDE E WI-FI---------
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_eth_phy_lan87xx.h"
#include "esp_rom_sys.h"

//-------Bibliotecas TCP/IP PURO---------
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

//-------DEFINES---------
#define USER_BUTTON_GPIO    GPIO_NUM_34
#define USER_LED_GPIO       GPIO_NUM_33

#define PERIODO_LEITURA_MS  5000 // 2,5seg (tirar 3 zeros para seg)
#define CICLOS_24H  (24 * 60 * 60 * 1000 / PERIODO_LEITURA_MS)

#define RTC_SCL_IO  GPIO_NUM_16 //11 na placa
#define RTC_SDA_IO  GPIO_NUM_32 //13 na placa
#define I2C_MASTER_NUM 0
#define I2C_FREQ_HZ 100000 //100kHz
#define DS3231_ADDR 0x68 //binário no datasheet para o endereço
#define AM2320_ADDR 0x5C //endereço I2C do sensor de humidade/temperatura

//-------FUNÇÕES FORA DA MAIN---------
//--- RELÓGIO e I2C ---
esp_err_t init_i2c(void);
i2c_master_bus_handle_t obter_i2c_bus(void);
void i2c_lock(void);
void i2c_unlock(void);
void ler_relogio(char *buffer_data, char *buffer_hora);
void acertar_rel(int ano, int mes, int dia, int hora, int min, int seg);
bool sincronizar_ntp(void);

//--- BOTÃO TOGGLE (START/STOP) ---
void init_botao(void);
extern volatile bool sistema_ativo;

//--- CARTÃO SD ---
esp_err_t init_cartao_sd();
long ler_ponteiro();
void guardar_ponteiro(long posicao);
bool limpar_sd(void);

//--- COMUNICAÇÃO TCP --- 
extern TaskHandle_t handle_tarefa_tcp; //o nosso sinal de comunicação entre as tasks

void task_sincro_tcp(void *pvParameters);
esp_err_t conexao_servidor(const char* linha);

//--- LEITOR DE SENSORES (ADC) ---
void init_ADC(void);
float ler_corrente_rms(void);

//--- CONTACTO AUXILIAR ---
void init_contacto_aux(void);
bool ler_estado_maquina(void);

//--- SENSOR AM2320B (HUMIDADE/TEMPERATURA) ---
void init_am2320(void);
esp_err_t ler_temp_hum(float *temperatura, float *humidade);

#endif

//-------------------DEFINES DE REDE-------------
#ifndef REDE_H
#define REDE_H

#define ETH_PHY_ADDR         0          // endereço padrão na Olimex
#define ETH_PHY_RST_GPIO    -1          // não usa pino de reset dedicado por GPIO
#define ETH_MDC_GPIO        GPIO_NUM_23         // pino MDC
#define ETH_MDIO_GPIO       GPIO_NUM_18         // pino MDIO
#define PHY_PWR_GPIO        GPIO_NUM_5  // da enregia ao chip da ethernet

#define IP_SERVIDOR "192.168.10.167" // IP do PC/Servidor que vai receber os dados
#define PORTA_SERVIDOR 5000
#define DB_NAME "machine_monitor"
#define DB_USER "flexoflow"
#define DB_PASS "FlexoTeste@1234."

//extern volatile bool ethernet_on;
extern volatile bool rede_disponivel;
//void init_ethernet(void);
void init_wifi(void);

#endif
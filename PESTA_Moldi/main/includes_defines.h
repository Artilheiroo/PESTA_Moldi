#ifndef INCLUDES_DEFINES_H
#define INCLUDES_DEFINES_H

//-------Bibliotecas standard C---------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

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

// -------Bibliotecas REDE E WI-FI---------
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_eth_phy_lan87xx.h"

//-------Bibliotecas TCP/IP PURO---------
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

//-------DEFINES---------
#define USER_BUTTON_GPIO GPIO_NUM_34
#define USER_LED_GPIO GPIO_NUM_33

#define PERIODO_LEITURA_MS 5000 // 5segundos (tirar 3 zeros para seg)

#define RTC_SCL_IO GPIO_NUM_16
#define RTC_SDA_IO GPIO_NUM_32
#define I2C_MASTER_NUM 0
#define I2C_FREQ_HZ 100000 //100kHz
#define DS3231_ADDR 0x68 //binário no datasheet para o endereço

//-------FUNÇÕES FORA DA MAIN---------
//--- RELÓGIO ---
esp_err_t init_i2c(void);
void ler_relogio(char *buffer_data, char *buffer_hora);
void acertar_rel(int ano, int mes, int dia, int hora, int min, int seg);

//--- BOTAO START ---
void esperar_start(void);

//Cartão SD
esp_err_t init_cartao_sd();
long ler_ponteiro();
void guardar_ponteiro(long posicao);


#endif

//-------------------DEFINES DE REDE-------------
#ifndef REDE_H
#define REDE_H

#define ETH_PHY_ADDR         0          // endereço padrão na Olimex
#define ETH_PHY_RST_GPIO    -1          // não usa pino de reset dedicado por GPIO
#define ETH_MDC_GPIO        GPIO_NUM_23         // pino MDC
#define ETH_MDIO_GPIO       GPIO_NUM_18         // pino MDIO
#define PHY_PWR_GPIO        GPIO_NUM_5  // da enregia ao chip da ethernet

#define WIFI_SSID "NOME_DA_TUA_REDE_WIFI"
#define WIFI_PASS "PASSWORD_DO_WIFI"

#define IP_SERVIDOR "192.168.1.100" // IP do PC/Servidor que vai receber os dados
#define PORTA_SERVIDOR 8080

extern volatile bool rede_disponivel;
void init_ethernet(void);

#endif
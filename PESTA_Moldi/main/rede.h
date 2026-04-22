#include "includes_defines.h"

#ifndef REDE_H
#define REDE_H

#define ETH_PHY_ADDR         0          // endereço padrão na Olimex
#define ETH_PHY_RST_GPIO    -1          // não usa pino de reset dedicado por GPIO
#define ETH_MDC_GPIO        GPIO_NUM_23         // pino MDC
#define ETH_MDIO_GPIO       GPIO_NUM_18          // pino MDIO
#define PHY_PWR_GPIO        GPIO_NUM_5  // da enregia ao chip da ethernet

#define WIFI_SSID "NOME_DA_TUA_REDE_WIFI"
#define WIFI_PASS "PASSWORD_DO_WIFI"

#define IP_SERVIDOR "192.168.1.100" // IP do PC/Servidor que vai receber os dados
#define PORTA_SERVIDOR 8080

#endif
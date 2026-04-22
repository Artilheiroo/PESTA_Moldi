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
//#include "driver/i2c.h"
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
#define PERIODO_LEITURA_MS 5000

#endif
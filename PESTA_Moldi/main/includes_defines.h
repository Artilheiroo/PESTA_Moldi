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
#include "driver/i2c.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "esp_err.h"

//-------DEFINES---------
#define USER_LED_GPIO GPIO_NUM_33
#define PERIODO_LEeENVIA_MS 3000

#endif
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define USER_LED_GPIO GPIO_NUM_33
#define BLINK_PERIOD_MS 10

static const char *TAG = "blink_test";

void app_main(void)
{
    
}

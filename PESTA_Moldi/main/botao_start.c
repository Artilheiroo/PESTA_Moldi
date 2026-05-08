#include "includes_defines.h"

static const char *TAG = "START";

void esperar_start(void)
{
    gpio_reset_pin(USER_LED_GPIO); //reinicia o pino do led
    gpio_set_direction(USER_LED_GPIO, GPIO_MODE_OUTPUT); //pino do led como saída

    gpio_reset_pin(USER_BUTTON_GPIO);
    gpio_set_direction(USER_BUTTON_GPIO, GPIO_MODE_INPUT);

    vTaskDelay(pdMS_TO_TICKS(2400));

    ESP_LOGI(TAG, "PRESSIONAR O BOTAO PARA INICIAR");

    // evita arranque falso quando o botao arranca ja pressionado
    while (gpio_get_level(USER_BUTTON_GPIO) == 0) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    while (1)
    {
        if (gpio_get_level(USER_BUTTON_GPIO) == 0) {
            vTaskDelay(pdMS_TO_TICKS(30)); // tempo de carregar
            if (gpio_get_level(USER_BUTTON_GPIO) == 0) {
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    ESP_LOGI(TAG, "Sistema de monitorização ligado!");
    gpio_set_level(USER_LED_GPIO, 1); //liga o led
}
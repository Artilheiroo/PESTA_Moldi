#include "includes_defines.h"

static const char *TAG = "BOTAO";

/*===============================
     BOTÃO TOGGLE (START/STOP)
===============================*/

volatile bool sistema_ativo = false; // flag global — main verifica a cada ciclo

// Task de background que monitoriza o botão continuamente
static void botao_on_off(void *pvParameters)
{
    // Esperar que o botão esteja solto antes de aceitar o primeiro press
    while (gpio_get_level(USER_BUTTON_GPIO) == 0) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    ESP_LOGI(TAG, "PRESSIONAR O BOTAO PARA INICIAR");

    while (1)
    {
        // Esperar que o botão seja pressionado (nível 0 = pressionado)
        if (gpio_get_level(USER_BUTTON_GPIO) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(100)); // debounce

            // Confirmar que continua pressionado (não foi ruído)
            if (gpio_get_level(USER_BUTTON_GPIO) == 0)
            {
                // Toggle do estado
                sistema_ativo = !sistema_ativo;

                if (sistema_ativo) {
                    gpio_set_level(USER_LED_GPIO, 1); // LED acende
                    ESP_LOGI(TAG, "Sistema LIGADO!");
                } else {
                    gpio_set_level(USER_LED_GPIO, 0); // LED apaga
                    ESP_LOGW(TAG, "Sistema PAUSADO!");
                }

                // Esperar que o botão seja SOLTO antes de aceitar novo press
                // Isto evita toggle múltiplo se o utilizador mantiver pressionado
                while (gpio_get_level(USER_BUTTON_GPIO) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
                vTaskDelay(pdMS_TO_TICKS(200)); // pausa anti-bounce no soltar
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // polling a cada 20ms (não sobrecarrega o CPU)
    }
}

void init_botao(void)
{
    // Configurar GPIO do botão (GPIO34 é input-only no ESP32)
    gpio_reset_pin(USER_BUTTON_GPIO);
    gpio_set_direction(USER_BUTTON_GPIO, GPIO_MODE_INPUT);

    // Configurar GPIO do LED
    gpio_reset_pin(USER_LED_GPIO);
    gpio_set_direction(USER_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(USER_LED_GPIO, 0); // LED começa apagado

    vTaskDelay(pdMS_TO_TICKS(2500)); // tempo para estabilizar no arranque

    // Criar task de background para monitorizar o botão (core 0, prioridade alta)
    xTaskCreatePinnedToCore(botao_on_off, "TaskBotao", 2048, NULL, 10, NULL, 0);
}
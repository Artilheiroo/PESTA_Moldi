#include "rede.h"

static const char *TAG = "REDE";
volatile bool rede_disponivel = false; // bloqueia ou liberta a tarefa TCP

static void eth_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if(event_base == ETH_EVENT && event_id == ETHERNET_EVENT_CONNECTED)
    {
        // O hardware detectou que o cabo foi inserido e há sinal elétrico
        ESP_LOGI(TAG, "Cabo Ethernet conectado!");
    }else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_DISCONNECTED)
    {
        // O cabo foi removido e para o envio TCP imediatamente
        ESP_LOGW(TAG,"Cabo Ethernet desligado!");
        rede_disponivel = false;// diz que não ha conexão
    }else if(event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP)
    {
        //o router atribui IP ao ESP32.
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;// luz verde para a tarefa envio
        ESP_LOGI(TAG, "Conexão com IP: " IPSTR, IP2STR(&event->ip_info.ip));
        rede_disponivel = true; // diz que ha conexão
    }
}

void init_ethernet(void)
{
    gpio_reset_pin(PHY_PWR_GPIO); //reseta ao pino
    gpio_set_direction(PHY_PWR_GPIO, GPIO_MODE_OUTPUT); //define como saida
    gpio_set_level(PHY_PWR_GPIO, 1); //alimenta o chip ethernet
    vTaskDelay(pdMS_TO_TICKS(100)); // pausa para o chip estabilizar a energia

    //inicializar as camadas de rede base (LwIP e Event Loop)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 3. Criar a interface lógica de rede para Ethernet
    esp_netif_config_t cfg_netif = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg_netif);

    // mac (media acess control) é o "cérebro" da rede dentro do ESP32. Aqui dizemos quais os pinos de dados (MDC/MDIO).
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    esp32_emac_config.smi_gpio.mdc_num = ETH_MDC_GPIO;   // Pino 23
    esp32_emac_config.smi_gpio.mdio_num = ETH_MDIO_GPIO; // Pino 18
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);

    // phy (physical layer) é o chip externo que transforma os bits em sinais elétricos para o cabo.
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = ETH_PHY_ADDR; // Geralmente 0 na Olimex
    phy_config.reset_gpio_num = -1;     // Reset é feito via Power (GPIO 5), não por pino dedicado

    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);// driver para a damilia LAN87xx

    // driver completo (MAC + PHY)
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

    //registar as funções que vão tratar os eventos de rede e de IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &eth_event_handler, NULL, NULL));

    // arrancar o driver Ethernet (inicia a "negociação" com o Router)
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}

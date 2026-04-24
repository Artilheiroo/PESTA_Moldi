idf_component_register(
    SRCS "main.c" "relogio.c" "botao_start.c" "rede.c" "cartaoSD.c"
    INCLUDE_DIRS "."
    REQUIRES
        fatfs
        sdmmc
        esp_netif
        esp_eth
        esp_event
        esp_driver_i2c
        esp_wifi
        nvs_flash
        lwip
)

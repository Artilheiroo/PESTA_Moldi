idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        fatfs
        sdmmc
        esp_netif
        esp_eth
        esp_event
        esp_wifi
        nvs_flash
        lwip
)

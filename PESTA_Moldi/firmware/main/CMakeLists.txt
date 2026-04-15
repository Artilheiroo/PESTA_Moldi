idf_component_register(
  SRCS "main.c" "wifi_manager.c" "http_client.c"
  INCLUDE_DIRS "."
  REQUIRES esp_wifi esp_event esp_netif nvs_flash esp_http_client esp_timer
)

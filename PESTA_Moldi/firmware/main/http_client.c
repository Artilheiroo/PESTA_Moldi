#include "http_client.h"

#include <string.h>

#include "esp_http_client.h"
#include "esp_log.h"

static const char *TAG = "HTTP_CLIENT";

esp_err_t http_client_post_reading(const char *url, const char *json_payload, int timeout_ms) {
  esp_http_client_config_t config = {
    .url = url,
    .method = HTTP_METHOD_POST,
    .timeout_ms = timeout_ms
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL) {
    return ESP_FAIL;
  }

  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_set_header(client, "Content-Type", "application/json"));
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_set_post_field(client, json_payload, (int)strlen(json_payload)));

  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "POST status=%d", status);
    if (status < 200 || status >= 300) {
      err = ESP_FAIL;
    }
  } else {
    ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);
  return err;
}

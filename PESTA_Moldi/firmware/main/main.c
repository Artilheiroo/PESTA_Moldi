#include <stdio.h>
#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "http_client.h"
#include "wifi_manager.h"

static const char *TAG = "MAIN";

static void start_sntp(void) {
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_init();
}

static void format_timestamp_iso8601(char *out, size_t out_len) {
  time_t now = time(NULL);
  struct tm time_info = { 0 };
  gmtime_r(&now, &time_info);
  strftime(out, out_len, "%Y-%m-%dT%H:%M:%SZ", &time_info);
}

static float get_simulated_temperature(void) {
  int64_t ms = esp_timer_get_time() / 1000;
  return 24.0f + (float)(ms % 7000) / 1000.0f;
}

void app_main(void) {
  if (wifi_manager_start() != ESP_OK) {
    ESP_LOGE(TAG, "Cannot start without Wi-Fi");
    return;
  }

  start_sntp();
  vTaskDelay(pdMS_TO_TICKS(2000));

  for (;;) {
    if (!wifi_manager_is_connected()) {
      ESP_LOGW(TAG, "Wi-Fi disconnected, waiting...");
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    char ts[32];
    char payload[256];
    format_timestamp_iso8601(ts, sizeof(ts));

    const float temp = get_simulated_temperature();
    snprintf(
      payload,
      sizeof(payload),
      "{\"device_id\":\"%s\",\"metric\":\"temperature\",\"value\":%.2f,\"unit\":\"C\",\"ts\":\"%s\"}",
      CONFIG_DEVICE_ID,
      temp,
      ts
    );

    esp_err_t err = http_client_post_reading(CONFIG_API_URL, payload, CONFIG_HTTP_TIMEOUT_MS);
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "Send failed, retrying in 3s");
      vTaskDelay(pdMS_TO_TICKS(3000));
      err = http_client_post_reading(CONFIG_API_URL, payload, CONFIG_HTTP_TIMEOUT_MS);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "Retry failed");
      }
    }

    ESP_LOGI(TAG, "Payload sent: %s", payload);
    vTaskDelay(pdMS_TO_TICKS(CONFIG_SEND_INTERVAL_SECONDS * 1000));
  }
}

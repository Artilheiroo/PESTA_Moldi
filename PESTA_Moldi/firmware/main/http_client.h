#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "esp_err.h"

esp_err_t http_client_post_reading(const char *url, const char *json_payload, int timeout_ms);

#endif

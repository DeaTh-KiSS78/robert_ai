#pragma once
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

void chat_register_routes(httpd_handle_t server);

#ifdef __cplusplus
}
#endif

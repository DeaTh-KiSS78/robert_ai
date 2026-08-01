#pragma once
#include "driver/gpio.h"
#include "esp_err.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sd_card_mount(void);

#ifdef __cplusplus
}
#endif

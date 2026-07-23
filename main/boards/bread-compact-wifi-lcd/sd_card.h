#pragma once
#include "driver/gpio.h"
#include "esp_err.h"

// Lafvin ESP32-S3-CAM folosește SDMMC pe 1-bit
#define CONFIG_SOC_SDMMC_USE_GPIO_MATRIX 1
#define CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4 0   // doar 1-bit

// Pinii SDMMC conform pinout-ului LAFVIN ESP32-S3-CAM
#define CONFIG_EXAMPLE_PIN_CLK  GPIO_NUM_39   // SD_CLK
#define CONFIG_EXAMPLE_PIN_CMD  GPIO_NUM_38   // SD_CMD
#define CONFIG_EXAMPLE_PIN_D0   GPIO_NUM_40   // SD_DATA0

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sd_card_mount(void);

#ifdef __cplusplus
}
#endif

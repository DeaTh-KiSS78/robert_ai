#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "sd_card.h"

static const char *TAG = "sd_card";

esp_err_t sd_card_mount(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card = NULL;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4 ? 4 : 1;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

#if CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
    slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
    slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
    slot_config.d0  = CONFIG_EXAMPLE_PIN_D0;
#if CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    slot_config.d1  = CONFIG_EXAMPLE_PIN_D1;
    slot_config.d2  = CONFIG_EXAMPLE_PIN_D2;
    slot_config.d3  = CONFIG_EXAMPLE_PIN_D3;
#endif
#endif

    ESP_LOGI(TAG, "Mounting SD card...");
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SD mounted OK");
    sdmmc_card_print_info(stdout, card);

    return ESP_OK;
}

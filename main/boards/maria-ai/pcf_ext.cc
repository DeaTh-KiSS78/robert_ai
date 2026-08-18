#include "pcf_ext.h"
#include "application.h"
#include "display/lcd_display.h"
#include "config.h"
#include <esp_log.h>

#define TAG "PCF_EXT"

// Instanța driverului PCF8574
static i2c_dev_t pcf_dev;

// Inițializare PCF8574
void InitializePcfExt(MariaAi* board)
{
    ESP_LOGI(TAG, "Initializing PCF8574...");

    memset(&pcf_dev, 0, sizeof(i2c_dev_t));

    // Inițializare descriptor I2C
    ESP_ERROR_CHECK(pcf8574_init_desc(
        &pcf_dev,
        0x27,                        // Adresa PCF8574
        AUDIO_CODEC_I2C_NUM,
        AUDIO_CODEC_I2C_SDA_PIN,
        AUDIO_CODEC_I2C_SCL_PIN
    ));

    // Setăm toți pinii HIGH (input cu pull-up)
    ESP_ERROR_CHECK(pcf8574_port_write(&pcf_dev, 0xFF));

    // Test de comunicare
    uint8_t val = 0;
    esp_err_t err = pcf8574_port_read(&pcf_dev, &val);

    if (err == ESP_OK)
        ESP_LOGI(TAG, "PCF8574 OK, port=0x%02X", val);
    else
        ESP_LOGE(TAG, "PCF8574 NOT RESPONDING (err=%d)", err);
}


// Task-ul care citește butoanele PCF8574
static void PcfExtTask(void* arg)
{
    auto* board = static_cast<MariaAi*>(arg);
    uint8_t port;

    while (true)
    {
        if (pcf8574_port_read(&pcf_dev, &port) == ESP_OK)
        {
            bool up     = !(port & (1 << PCF_BTN_UP));
            bool down   = !(port & (1 << PCF_BTN_DOWN));
            bool left   = !(port & (1 << PCF_BTN_LEFT));
            bool right  = !(port & (1 << PCF_BTN_RIGHT));
            bool middle = !(port & (1 << PCF_BTN_MIDDLE));
            bool rst    = !(port & (1 << PCF_BTN_RST));

            auto& app = Application::GetInstance();

            if (up) {
                auto codec = board->GetAudioCodec();
                int v = codec->output_volume() + 10;
                if (v > 100) v = 100;
                codec->SetOutputVolume(v);
                board->GetDisplay()->ShowNotification("Volum: " + std::to_string(v / 10));
            }

            if (down) {
                auto codec = board->GetAudioCodec();
                int v = codec->output_volume() - 10;
                if (v < 0) v = 0;
                codec->SetOutputVolume(v);
                board->GetDisplay()->ShowNotification("Volum: " + std::to_string(v / 10));
            }

            if (left) {
                auto bl = board->GetBacklight();
                int b = bl->brightness() - 10;
                if (b < 1) b = 1;
                bl->SetBrightness(b);
                board->GetDisplay()->ShowNotification("Luminozitate: " + std::to_string(b) + "%");
            }

            if (right) {
                auto bl = board->GetBacklight();
                int b = bl->brightness() + 10;
                if (b > 100) b = 100;
                bl->SetBrightness(b);
                board->GetDisplay()->ShowNotification("Luminozitate: " + std::to_string(b) + "%");
            }

            if (middle) {
                app.ToggleChatState();
            }

            if (rst) {
                esp_restart();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


// Pornirea task-ului
void StartPcfExtTask(MariaAi* board)
{
    xTaskCreatePinnedToCore(PcfExtTask, "pcf_ext_task", 4096, board, 5, nullptr, 0);
}

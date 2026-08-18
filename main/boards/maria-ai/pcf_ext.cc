#include "pcf_ext.h"
#include "application.h"
#include "display/lcd_display.h"
#include <esp_log.h>

#define TAG "PCF_EXT"

static i2c_master_dev_handle_t pcf_dev;

void InitializePcfExt(WifiBoard* board)
{
    // Folosește bus-ul I2C deja creat în MariaAi
    i2c_master_bus_handle_t bus = board->GetCodecI2cBus();

    // EXACT ca TouchDriver
    i2c_master_dev_config_t cfg = {
        .device_address = 0x27,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &cfg, &pcf_dev));
}

static void PcfExtTask(void* arg)
{
    auto* board = static_cast<WifiBoard*>(arg);
    uint8_t port;

    while (true)
    {
        // Citire simplă, fără scriere de pull-up
        if (i2c_master_transmit_receive(pcf_dev, NULL, 0, &port, 1, 50) == ESP_OK)
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

void StartPcfExtTask(WifiBoard* board)
{
    xTaskCreatePinnedToCore(PcfExtTask, "pcf_ext_task", 4096, board, 5, nullptr, 0);
}

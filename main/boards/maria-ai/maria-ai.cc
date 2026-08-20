#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>

#include <wifi_station.h>
#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "lamp_controller.h"
#include "assets/lang_config.h"
#include "adc_battery_monitor.h"
#include "sd_card.h"
#include "http_server.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <wifi_manager.h>

#include "led/single_led.h"
#include "system_reset.h"
#include "esp_lcd_ili9341.h"

#define TAG "MariaAi"


class TouchDriver {
public:
    TouchDriver() : dev_(nullptr) {}

    bool Init(i2c_master_bus_handle_t bus, uint8_t addr) {
        i2c_device_config_t cfg = {
            .device_address = addr,
            .scl_speed_hz = 400000,
            .scl_wait_us = 0,
        };
        return i2c_master_bus_add_device(bus, &cfg, &dev_) == ESP_OK;
    }

    bool Read(bool &touched, uint16_t &x, uint16_t &y) {
        touched = false;
        x = y = 0;
        if (!dev_) return false;

        uint8_t reg = 0x02;
        uint8_t buf[5];
        if (i2c_master_transmit_receive(dev_, &reg, 1, buf, 5, 50) != ESP_OK) return false;

        uint8_t points = buf[0] & 0x0F;
        if (points == 0) return true;

        touched = true;
        x = ((buf[1] & 0x0F) << 8) | buf[2];
        y = ((buf[3] & 0x0F) << 8) | buf[4];
        return true;
    }

private:
    i2c_master_dev_handle_t dev_;
};

static i2c_master_dev_handle_t pcf_dev = nullptr;
static uint8_t pcf_state = 0xFE;   // P0 LOW (lampă), restul HIGH
class MariaAi : public WifiBoard {
private:
    Button boot_button_;
    Button volume_up_button_;
    Button volume_down_button_;
    Button backlight_up_button_;
    Button backlight_down_button_;
    LcdDisplay *display_;
    i2c_master_bus_handle_t codec_i2c_bus_;
    TouchDriver touch_;
    AdcBatteryMonitor* adc_battery_monitor_;
    bool web_server_started_ = false;

    // ---------------- PCF WRITE ----------------
    void PcfWriteState() {
        i2c_master_transmit(pcf_dev, &pcf_state, 1, 50);
    }

    void PcfSetPin(int pin, bool level) {
        if (level)
            pcf_state |=  (1 << pin);
        else
            pcf_state &= ~(1 << pin);

        PcfWriteState();
    }

    uint8_t PcfRead() {
        uint8_t val = 0xFF;
        i2c_master_receive(pcf_dev, &val, 1, 50);
        return val;
    }

    // ---------------- PCF BUTTON HANDLERS ----------------
    void HandlePcfClick(int pin) {
        switch(pin) {
            case PCF_BTN_UP: volume_up_button_.TriggerClick(); break;
            case PCF_BTN_DOWN: volume_down_button_.TriggerClick(); break;
            case PCF_BTN_LEFT: backlight_up_button_.TriggerClick(); break;
            case PCF_BTN_RIGHT: backlight_down_button_.TriggerClick(); break;
            case PCF_BTN_MIDDLE: boot_button_.TriggerClick(); break;
            case PCF_BTN_RST: SystemReset::Restart(); break;
        }
    }

    void HandlePcfLongPress(int pin) {
        switch(pin) {
            case PCF_BTN_UP: volume_up_button_.TriggerLongPress(); break;
            case PCF_BTN_DOWN: volume_down_button_.TriggerLongPress(); break;
            case PCF_BTN_LEFT: backlight_up_button_.TriggerLongPress(); break;
            case PCF_BTN_RIGHT: backlight_down_button_.TriggerLongPress(); break;
            case PCF_BTN_MIDDLE: boot_button_.TriggerLongPress(); break;
            case PCF_BTN_RST: SystemReset::FactoryReset(); break;
        }
    }

    // ---------------- PCF BUTTON TASK ----------------
    static void PcfButtonTask(void *arg)
    {
        auto *self = static_cast<MariaAi*>(arg);
        uint8_t last = pcf_state;
        uint32_t press_start[8] = {0};
        bool pressed[8] = {false};

        while (true) {
            uint8_t cur = self->PcfRead();
            uint32_t now = esp_timer_get_time() / 1000;

            uint8_t changed = last ^ cur;
            if (changed) {
                for (int pin = 1; pin <= 6; pin++) {
                    bool is_low = !(cur & (1 << pin));

                    if (is_low && !pressed[pin]) {
                        pressed[pin] = true;
                        press_start[pin] = now;
                    }

                    if (!is_low && pressed[pin]) {
                        uint32_t duration = now - press_start[pin];
                        pressed[pin] = false;

                        if (duration < 700)
                            self->HandlePcfClick(pin);
                        else
                            self->HandlePcfLongPress(pin);
                    }
                }

                last = cur;
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

public:
    MariaAi():
        boot_button_(BOOT_BUTTON_GPIO),
        volume_up_button_(VOLUME_UP_BUTTON_GPIO),
        volume_down_button_(VOLUME_DOWN_BUTTON_GPIO),
        backlight_up_button_(BACKLIGHT_UP_BUTTON_GPIO),
        backlight_down_button_(BACKLIGHT_DOWN_BUTTON_GPIO)
    {
        InitializeI2c();
        InitializePcf();
        InitializeBatteryMonitor();
        InitializeSpi();
        InitializeLcdDisplay();
        InitializeTouch();
        InitializeButtons();
        InitializeTools();
        GetBacklight()->SetBrightness(100);

        // Setare stare inițială PCF
        pcf_state = 0xFE;
        PcfWriteState();

        // Task PCF
        xTaskCreatePinnedToCore(PcfButtonTask, "pcf_buttons", 4096, this, 5, nullptr, 1);

        // SD card
        vTaskDelay(pdMS_TO_TICKS(3000));
        sd_card_mount();

        // Webserver
        if (!web_server_started_) {
            web_server_started_ = true;
            xTaskCreate(WebServerTask, "webserver_task", 4096, this, 5, nullptr);
        }
    }

    // ---------------- INITIALIZĂRI ----------------

    void InitializePcf() {
        i2c_device_config_t pcf_cfg = {
            .device_address = 0x27,
            .scl_speed_hz = 100000,
            .scl_wait_us = 0,
        };
        ESP_ERROR_CHECK(i2c_master_bus_add_device(codec_i2c_bus_, &pcf_cfg, &pcf_dev));
        ESP_LOGI("PCF", "PCF8574 initialized OK");
    }

    void InitializeBatteryMonitor() {
        adc_battery_monitor_ = new AdcBatteryMonitor(ADC_UNIT_1, ADC_CHANNEL_8, 200000, 200000, GPIO_NUM_NC);
    }

    void InitializeTouch() {
        if (!touch_.Init(codec_i2c_bus_, 0x38)) return;
        xTaskCreatePinnedToCore(TouchTask, "touch_task", 4096, this, 5, nullptr, 0);
    }

    void InitializeI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = AUDIO_CODEC_I2C_NUM,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = { .enable_internal_pullup = 1 },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_MOSI_PIN;
        buscfg.miso_io_num = DISPLAY_MIS0_PIN;
        buscfg.sclk_io_num = DISPLAY_SCK_PIN;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });

        volume_up_button_.OnClick([this]() {
            auto codec = GetAudioCodec();
            int volume = codec->output_volume() + 10;
            if (volume > 100) volume = 100;
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification("Volum: " + std::to_string(volume/10));
        });

        volume_up_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(100);
            GetDisplay()->ShowNotification("Volum MAX");
        });

        volume_down_button_.OnClick([this]() {
            auto codec = GetAudioCodec();
            int volume = codec->output_volume() - 10;
            if (volume < 0) volume = 0;
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification("Volum: " + std::to_string(volume/10));
        });

        volume_down_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(0);
            GetDisplay()->ShowNotification("Mut");
        });

        backlight_up_button_.OnClick([this]() {
            auto backlight = GetBacklight();
            int b = backlight->brightness() + 10;
            if (b > 100) b = 100;
            backlight->SetBrightness(b);
            GetDisplay()->ShowNotification("Luminozitate: " + std::to_string(b) + "%");
        });

        backlight_up_button_.OnLongPress([this]() {
            auto backlight = GetBacklight();
            backlight->SetBrightness(100);
            GetDisplay()->ShowNotification("Luminozitate MAX");
        });

        backlight_down_button_.OnClick([this]() {
            auto backlight = GetBacklight();
            int b = backlight->brightness() - 10;
            if (b < 1) b = 1;
            backlight->SetBrightness(b);
            GetDisplay()->ShowNotification("Luminozitate: " + std::to_string(b) + "%");
        });

        backlight_down_button_.OnLongPress([this]() {
            auto backlight = GetBacklight();
            backlight->SetBrightness(1);
            GetDisplay()->ShowNotification("Luminozitate MIN");
        });
    }

    void InitializeLcdDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = DISPLAY_SPI_SCLK_HZ;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_SPI_HOST, &io_config, &panel_io));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(panel_io, &panel_config, &panel));

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);

        display_ = new SpiLcdDisplay(panel_io, panel,
            DISPLAY_WIDTH, DISPLAY_HEIGHT,
            DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
            DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    void InitializeTools() {
        static LampController lamp(LAMP_GPIO);
    }

    // ---------------- OVERRIDES ----------------

    virtual Led *GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(codec_i2c_bus_, AUDIO_CODEC_I2C_NUM,
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE, AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN, AUDIO_CODEC_PA_PIN,
            AUDIO_CODEC_ES8311_ADDR, true, true);
        return &audio_codec;
    }

    virtual Display *GetDisplay() override { 
        return display_; 
    }

    virtual Backlight *GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

    virtual bool GetBatteryLevel(int &level, bool& charging, bool& discharging) override {
        charging = adc_battery_monitor_->IsCharging();
        discharging = adc_battery_monitor_->IsDischarging();
        level = adc_battery_monitor_->GetBatteryLevel();
        return true;
    }
};

DECLARE_BOARD(MariaAi);


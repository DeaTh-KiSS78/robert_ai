#include "dht_sensor.h"
#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "DHT_SENSOR";

// 🔥 TASK PERIODIC (10 minute)
static void DhtBackgroundTask(void* arg) {
    DhtSensor* self = static_cast<DhtSensor*>(arg);

    while (true) {
        bool ok = self->dht_.ReadData(3);

        if (ok) {
            ESP_LOGI(TAG,
                     "Periodic reading -> Temp: %d°C, Humidity: %d%%",
                     self->dht_.GetTemperature(),
                     self->dht_.GetHumidity());
        } else {
            ESP_LOGW(TAG, "Periodic reading failed");
        }

        // 10 minute = 600000 ms
        vTaskDelay(pdMS_TO_TICKS(600000));
    }
}

DhtSensor::DhtSensor(gpio_num_t pin)
    : dht_(pin)
{
    auto& mcp = McpServer::GetInstance();

    // 🔥 TOOL PRINCIPAL: citire temperatură + umiditate
    mcp.AddTool(
        "self.sensor.dht.read",
        "Read temperature and humidity from DHT sensor",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {

            bool ok = dht_.ReadData(3);

            if (!ok) {
                // fallback dacă avem date proaspete (<30 sec)
                if (dht_.IsDataFresh(30000)) {
                    char buf[128];
                    snprintf(buf, sizeof(buf),
                        "{\"cached\": true, \"age_ms\": %lu, "
                        "\"temperature\": %d, \"humidity\": %d}",
                        (unsigned long)dht_.GetDataFreshness(),
                        dht_.GetTemperature(),
                        dht_.GetHumidity());
                    return buf;
                }

                return "{\"error\": \"Failed to read DHT sensor\"}";
            }

            char buf[64];
            snprintf(buf, sizeof(buf),
                "{\"temperature\": %d, \"humidity\": %d}",
                dht_.GetTemperature(),
                dht_.GetHumidity());

            return buf;
        }
    );

    // 🔥 TOOL DE TEST
    mcp.AddTool(
        "self.sensor.dht.test",
        "Test if the DHT sensor is working properly",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {

            ESP_LOGI(TAG, "Testing DHT sensor...");

            bool ok = dht_.ReadData(3);

            if (ok) {
                int temp = dht_.GetTemperature();
                int hum  = dht_.GetHumidity();

                ESP_LOGI(TAG,
                         "DHT test success! Temp: %d°C, Humidity: %d%%",
                         temp, hum);

                char buf[128];
                snprintf(buf, sizeof(buf),
                    "{\"ok\": true, \"temperature\": %d, \"humidity\": %d}",
                    temp, hum);
                return buf;
            }

            ESP_LOGE(TAG, "DHT test failed!");

            return "{\"ok\": false, \"error\": \"Sensor test failed. Check wiring and GPIO.\"}";
        }
    );

    // 🔥 PORNIM TASK-UL PERIODIC
    xTaskCreatePinnedToCore(
        DhtBackgroundTask,
        "dht_bg_task",
        4096,
        this,
        5,
        nullptr,
        0
    );
}

#include "dht_sensor.h"
#include "application.h"
#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "DHT_SENSOR";

// 🔥 TASK PERIODIC (10 minute)
void DhtSensor::BackgroundTask(void* arg) {
    DhtSensor* self = static_cast<DhtSensor*>(arg);

    while (true) {
        bool ok = self->dht_.ReadData(3);

        if (ok) {
            int temp = self->dht_.GetTemperature();
            int hum  = self->dht_.GetHumidity();

            ESP_LOGI(TAG,
                     "Periodic reading -> Temp: %d°C, Humidity: %d%%",
                     temp, hum);

            // 🔥 Afișare pe display prin MCP JSONRPC
            auto& app = Application::GetInstance();

            char msg[64];
            snprintf(msg, sizeof(msg),
                     "Temp: %d°C  Hum: %d%%",
                     temp, hum);

            char json[256];
            snprintf(json, sizeof(json),
                "{\"jsonrpc\":\"2.0\",\"method\":\"tools/call\","
                "\"params\":{\"name\":\"self.display.show_notification\","
                "\"arguments\":{\"text\":\"%s\"}},\"id\":999}",
                msg);

            app.SendMcpMessage(json);
        } else {
            ESP_LOGW(TAG, "Periodic reading failed");
        }

        vTaskDelay(pdMS_TO_TICKS(600000)); // 10 minute
    }
}

DhtSensor::DhtSensor(gpio_num_t pin)
    : dht_(pin)
{
    auto& mcp = McpServer::GetInstance();

    // 🔥 TOOL PRINCIPAL
    mcp.AddTool(
        "self.sensor.dht.read",
        "Read temperature and humidity from DHT sensor",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {

            bool ok = dht_.ReadData(3);

            if (!ok) {
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
        BackgroundTask,
        "dht_bg_task",
        4096,
        this,
        5,
        nullptr,
        0
    );
}

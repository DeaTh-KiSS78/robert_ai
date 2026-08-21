#include "dht_sensor.h"
#include <stdio.h>
#include <esp_log.h>

static const char* TAG = "DHT_SENSOR";

DhtSensor::DhtSensor(gpio_num_t pin)
    : dht_(pin)   // inițializezi driverul direct
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
                // Dacă citirea a eșuat, dar avem date proaspete (<30 sec)
                if (dht_.IsDataFresh(30000)) {
                    char buf[128];
                    snprintf(buf, sizeof(buf),
                        "{\"cached\": true, \"age_ms\": %u, "
                        "\"temperature\": %.1f, \"humidity\": %.1f}",
                        dht_.GetDataFreshness(),
                        dht_.GetTemperature(),
                        dht_.GetHumidity());
                    return buf;
                }

                return "{\"error\": \"Failed to read DHT sensor\"}";
            }

            // Citire reușită
            char buf[64];
            snprintf(buf, sizeof(buf),
                "{\"temperature\": %.1f, \"humidity\": %.1f}",
                dht_.GetTemperature(),
                dht_.GetHumidity());

            return buf;
        }
    );

    // 🔥 TOOL DE TEST: verifică dacă senzorul funcționează
    mcp.AddTool(
        "self.sensor.dht.test",
        "Test if the DHT sensor is working properly",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {

            ESP_LOGI(TAG, "Testing DHT sensor...");

            bool ok = dht_.ReadData(3);

            if (ok) {
                float temp = dht_.GetTemperature();
                float hum  = dht_.GetHumidity();

                ESP_LOGI(TAG, "DHT test success! Temp: %.1f°C, Humidity: %.1f%%",
                         temp, hum);

                char buf[128];
                snprintf(buf, sizeof(buf),
                    "{\"ok\": true, \"temperature\": %.1f, \"humidity\": %.1f}",
                    temp, hum);
                return buf;
            }

            ESP_LOGE(TAG, "DHT test failed!");

            return "{\"ok\": false, \"error\": \"Sensor test failed. Check wiring and GPIO.\"}";
        }
    );
}

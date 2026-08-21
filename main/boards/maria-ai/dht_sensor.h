#pragma once

#include "dht11.h"
#include "config.h"
#include "mcp_server.h"

class DhtSensor {
public:
    explicit DhtSensor(gpio_num_t pin);

private:
    xiaozhi::DHT11 dht_;

    static void BackgroundTask(void* arg);
};

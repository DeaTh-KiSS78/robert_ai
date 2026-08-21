#pragma once

#include "dht11.h"
#include "config.h"
#include "mcp_server.h"

class DhtSensor {
public:
    // Constructorul primește pinul (DHT11_PIN din config.h)
    explicit DhtSensor(gpio_num_t pin);

private:
    xiaozhi::DHT11 dht_;
};

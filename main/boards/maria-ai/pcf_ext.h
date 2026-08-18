#pragma once

#include <driver/i2c_master.h>
#include "config.h"
#include "wifi_board.h"

void InitializePcfExt(i2c_master_bus_handle_t bus);
void StartPcfExtTask(WifiBoard* board);

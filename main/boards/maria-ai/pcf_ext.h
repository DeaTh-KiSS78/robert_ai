#pragma once

#include <driver/i2c_master.h>
#include "config.h"
#include "wifi_board.h"

void InitializePcfExt(WifiBoard* board);
void StartPcfExtTask(WifiBoard* board);

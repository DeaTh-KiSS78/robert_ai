#pragma once

#include <driver/i2c_master.h>
#include "pcf8574.h"
#include "config.h"
#include "wifi_board.h"   // 🔥 avem definiția completă a clasei

void InitializePcfExt(WifiBoard* board);
void StartPcfExtTask(WifiBoard* board);

#pragma once

#include <driver/i2c_master.h>
#include "pcf8574.h"
#include "config.h"     // 🔥 necesar pentru PCF_BTN_UP etc.

// Forward declaration ca să evităm includerea MariaAi.cc
class MariaAi;

// Inițializează PCF8574 (I2C + test)
void InitializePcfExt(MariaAi* board);

// Pornește task-ul care citește butoanele PCF8574
void StartPcfExtTask(MariaAi* board);

#ifndef LOW_POWER_SLEEP_H
#define LOW_POWER_SLEEP_H

#include <Arduino.h>

typedef void (*LowPowerHookFn)();

void lowPowerSleepMs(uint32_t ms, LowPowerHookFn beforeSleep = nullptr, LowPowerHookFn afterWake = nullptr);

#endif

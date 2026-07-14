#include "LowPowerSleep.h"

#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

struct WdtStep {
  uint8_t  prescaler;
  uint16_t ms;
};

static const WdtStep WDT_STEPS[] = {
  { WDTO_8S,    8000 },
  { WDTO_4S,    4000 },
  { WDTO_2S,    2000 },
  { WDTO_1S,    1000 },
  { WDTO_500MS,  500 },
  { WDTO_250MS,  250 },
  { WDTO_120MS,  120 },
  { WDTO_60MS,    60 },
  { WDTO_30MS,    30 },
  { WDTO_15MS,    15 },
};

static const uint8_t WDT_STEP_COUNT =
  (uint8_t)(sizeof(WDT_STEPS) / sizeof(WDT_STEPS[0]));

static const WdtStep PATTERN_15S[] = {
  { WDTO_8S, 8000 },
  { WDTO_4S, 4000 },
  { WDTO_2S, 2000 },
  { WDTO_1S, 1000 },
};

static const uint16_t PATTERN_15S_MS = 15000;

static volatile uint8_t g_wdtFired = 0;

ISR(WDT_vect) {
  g_wdtFired = 1;
  WDTCSR |= (1 << WDIF);
}

static void disableWatchdog() {
  wdt_reset();
  MCUSR &= ~(1 << WDRF);
  if (WDTCSR & (1 << WDIF)) {
    WDTCSR |= (1 << WDIF);
  }
  WDTCSR = (1 << WDCE) | (1 << WDE);
  WDTCSR = 0x00;
}

static void enableWatchdogInterrupt(uint8_t prescaler) {
  disableWatchdog();
  delayMicroseconds(16);
  WDTCSR = (1 << WDCE) | (1 << WDE);
  WDTCSR = (uint8_t)((1 << WDIE) | (prescaler & 0x17));
}

static void delayWithTimer0(uint16_t stepMs) {
  power_timer0_enable();
  delay(stepMs);
  power_timer0_disable();
}

static bool pickWdtForMs(uint16_t ms, uint8_t &prescaler, uint16_t &stepMs) {
  stepMs = 0;
  prescaler = WDTO_15MS;

  for (uint8_t i = 0; i < WDT_STEP_COUNT; i++) {
    if (WDT_STEPS[i].ms <= ms && WDT_STEPS[i].ms >= stepMs) {
      stepMs = WDT_STEPS[i].ms;
      prescaler = WDT_STEPS[i].prescaler;
    }
  }

  return stepMs > 0;
}

static bool deepSleepOnce(uint8_t prescaler, uint16_t napMs) {
  g_wdtFired = 0;
  enableWatchdogInterrupt(prescaler);

  if (WDTCSR & (1 << WDIF)) {
    WDTCSR |= (1 << WDIF);
    g_wdtFired = 0;
  }

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_bod_disable();
  sei();
  sleep_cpu();
  sleep_disable();

  disableWatchdog();

  if (g_wdtFired) {
    return true;
  }

  delayWithTimer0(napMs);
  return false;
}

static void sleepOneWdt(uint8_t prescaler, uint16_t stepMs) {
  uint16_t waited = 0;

  while (waited < stepMs) {
    uint16_t left = stepMs - waited;
    uint8_t chunkPrescaler = prescaler;
    uint16_t chunkMs = left;

    if (left >= 1000) {
      chunkPrescaler = WDTO_1S;
      chunkMs = 1000;
    } else if (!pickWdtForMs(left, chunkPrescaler, chunkMs)) {
      delayWithTimer0(left);
      return;
    }

    deepSleepOnce(chunkPrescaler, chunkMs);
    waited += chunkMs;
  }
}

static void sleepPattern15s() {
  for (uint8_t i = 0; i < 4; i++) {
    sleepOneWdt(PATTERN_15S[i].prescaler, PATTERN_15S[i].ms);
  }
}

static bool pickWdtStep(uint32_t ms, uint16_t lastStepMs, uint8_t &prescaler, uint16_t &stepMs) {
  stepMs = 0;
  prescaler = WDTO_15MS;

  for (uint8_t i = 0; i < WDT_STEP_COUNT; i++) {
    if (WDT_STEPS[i].ms <= ms && WDT_STEPS[i].ms >= stepMs) {
      stepMs = WDT_STEPS[i].ms;
      prescaler = WDT_STEPS[i].prescaler;
    }
  }

  if (stepMs == 8000 && lastStepMs == 8000 && ms >= 4000) {
    for (uint8_t i = 0; i < WDT_STEP_COUNT; i++) {
      if (WDT_STEPS[i].ms <= ms && WDT_STEPS[i].ms < stepMs) {
        stepMs = WDT_STEPS[i].ms;
        prescaler = WDT_STEPS[i].prescaler;
      }
    }
  }

  return stepMs > 0;
}

static void sleepRemainder(uint32_t ms) {
  uint16_t lastStepMs = 0;

  while (ms >= 15) {
    uint8_t prescaler = WDTO_15MS;
    uint16_t stepMs = 0;

    if (!pickWdtStep(ms, lastStepMs, prescaler, stepMs)) {
      break;
    }

    sleepOneWdt(prescaler, stepMs);
    ms -= stepMs;
    lastStepMs = stepMs;
  }

  if (ms > 0) {
    delayWithTimer0((uint16_t)ms);
  }
}

static void disablePeripherals() {
  ADCSRA &= ~(1 << ADEN);
  power_adc_disable();
  power_timer0_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_spi_disable();
#if defined(PRR0) && defined(PRTWI)
  power_twi_disable();
#endif
#if defined(PRR0) && defined(PRUSART0)
  power_usart0_disable();
#endif
}

static void enablePeripherals() {
  power_adc_enable();
  power_timer0_enable();
  power_timer1_enable();
  power_timer2_enable();
  power_spi_enable();
#if defined(PRR0) && defined(PRTWI)
  power_twi_enable();
#endif
#if defined(PRR0) && defined(PRUSART0)
  power_usart0_enable();
#endif
}

void lowPowerSleepMs(uint32_t ms, LowPowerHookFn beforeSleep, LowPowerHookFn afterWake) {
  if (ms == 0) {
    return;
  }

  if (beforeSleep) {
    beforeSleep();
  }
  disablePeripherals();

  while (ms >= PATTERN_15S_MS) {
    sleepPattern15s();
    ms -= PATTERN_15S_MS;
  }

  sleepRemainder(ms);

  enablePeripherals();
  disableWatchdog();

  if (afterWake) {
    afterWake();
  }
}

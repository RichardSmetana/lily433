/*
 * WT450-TH transmitter with SHT45 for LilyPad Arduino (8 MHz)
 *
 * SHT45 : soft I2C on A4/A5, address 0x44, command 0xFD (high precision)
 * TX    : D3 -> 433 MHz ASK transmitter
 * Serial: 57600 baud
 *
 * SHT45 is in the Sensirion SHT4x family (same protocol as SHT40/SHT41).
 * Uses bit-banged I2C — do not call Wire.begin() (conflicts with soft I2C).
 */

// #define SERIAL_DEBUG 0   // uncomment to disable all Serial debug output

#define SERIAL_DEBUG 0


#define TX_PIN 3
#define SHT45_SDA_PIN A4
#define SHT45_SCL_PIN A5
#define SHT45_I2C_ADDRESS 0x44
//#define SEND_INTERVAL_MS 15000
//#define SEND_INTERVAL_MS 60000
#define SEND_INTERVAL_MS 60000
#define TX_INVERT 0
#define SENSOR_READ_RETRIES 3
#define SENSOR_RETRY_MS 500

#define SENSOR_OK 0
#define SENSOR_ERR_INVALID 1

#define WT450_HOUSE_CODE 1
#define WT450_CHANNEL 1

#include "Dbg.h"
#include "Sht45Soft.h"
#include "Wt450Send.h"
#include "LowPowerSleep.h"

Sht45Soft sht45(SHT45_SDA_PIN, SHT45_SCL_PIN, SHT45_I2C_ADDRESS);
Wt450Sender wt450(TX_PIN);
static bool sht45Ready = false;
static float ii = 1;


static void prepareForSleep() {
#if SERIAL_DEBUG
  Serial.flush();
#endif
  digitalWrite(TX_PIN, LOW);
  pinMode(TX_PIN, OUTPUT);
  pinMode(SHT45_SDA_PIN, INPUT);
  pinMode(SHT45_SCL_PIN, INPUT);
}

static void wakeFromSleep() {
  sht45.begin();
  wt450.begin(TX_INVERT);
}

static void sleepMs(uint32_t ms) {
  lowPowerSleepMs(ms, prepareForSleep, wakeFromSleep);
}

static bool initSht45() {
  sht45.begin();
  //#if SERIAL_DEBUG
  //  sht45.printBusLevels();
  //  sht45.scanBus();
  //#endif

  const uint8_t candidates[] = { SHT45_I2C_ADDRESS, 0x44, 0x45 };
  const Sht45Soft::MeasureCmd cmds[] = {
    Sht45Soft::MEASURE_HIGH,
    Sht45Soft::MEASURE_MEDIUM,
    Sht45Soft::MEASURE_LOW,
  };

  for (uint8_t i = 0; i < 3; i++) {
    uint8_t addr = candidates[i];
    bool duplicate = false;
    for (uint8_t j = 0; j < i; j++) {
      if (candidates[j] == addr) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) {
      continue;
    }

    sht45 = Sht45Soft(SHT45_SDA_PIN, SHT45_SCL_PIN, addr);

    DBG_PRINT(F("SHT45 @ 0x"));
    DBG_PRINT(addr, HEX);
    DBG_PRINT(F(": "));

    for (uint8_t c = 0; c < 3; c++) {
      if (!sht45.init(cmds[c])) {
        DBG_PRINT(F("init fail cmd=0x"));
        DBG_PRINTLN((uint8_t)cmds[c], HEX);
        continue;
      }

      float tempC = 0.0f;
      float humidityPct = 0.0f;
      int err = 0;
      if (sht45.read(tempC, humidityPct, err)) {
        DBG_PRINT(F("OK cmd=0x"));
        DBG_PRINT((uint8_t)cmds[c], HEX);
        DBG_PRINT(F("  "));
        DBG_PRINT(tempC, 2);
        DBG_PRINT(F(" C  "));
        DBG_PRINT(humidityPct, 1);
        DBG_PRINTLN(F(" %"));
        return true;
      }

      DBG_PRINT(F("read err "));
      DBG_PRINT(err);
      DBG_PRINT(F(" cmd=0x"));
      DBG_PRINTLN((uint8_t)cmds[c], HEX);
    }
  }

  DBG_PRINTLN(F("SHT45 init FAILED"));
  return false;
}

static bool isValidReading(float tempC, float humidityPct) {
  if (tempC < -40.0f || tempC > 125.0f) {
    return false;
  }
  if (humidityPct < 0.0f || humidityPct > 100.0f) {
    return false;
  }
  return true;
}

static Wt450Data makeWt450Payload(uint8_t house, uint8_t channel, float tempC, uint8_t humidityPct, bool batteryOk) {
  Wt450Data data;
  data.house = house;
  data.channel = channel;
  data.tempC = tempC;
  data.humidity = humidityPct;
  data.batteryOk = batteryOk;
  return data;
}

void setup() {
  DBG_BEGIN(57600);
#if SERIAL_DEBUG
  for (char iii = 0; iii < 10; iii++) {
    delay(200);
    DBG_PRINTLN();
  }
  DBG_PRINTLN(F("Minicore 328  WT450-TH emulator SHT45 + STX882"));
  DBG_PRINTLN(F("----------------------------------------------"));
  DBG_PRINT(F("F_CPU="));
  DBG_PRINT(F_CPU);
  DBG_PRINT(F(" Hz "));
  DBG_PRINT(F("SDA=A4 SCL=A5 TX=D"));
  DBG_PRINTLN(TX_PIN);
  DBG_PRINTLN(F("----------------------------------------------"));
#endif

  sht45Ready = initSht45();
  wt450.begin(TX_INVERT);
}

long readVcc() {
// Read 1.1V reference against AVcc
// Set the reference to Vcc and the multiplexer to the 1.1V internal bandgap
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)  // For LilyPad USB
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#else  // For ATmega328P (LilyPad Main/Simple)
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2);             // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);  // Start conversion
  while (bit_is_set(ADCSRA, ADSC))
    ;  // Measuring

  uint8_t low = ADCL;   // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH;  // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result;  // Calculate Vcc (in mV); 1125300 = 1.1 * 1023 * 1000
  return result;               // Returns millivolts (e.g. 3700 for 3.7V)
}

void loop() {

  float batteryVoltage = readVcc() / 1000.0;
  DBG_PRINT(F("Battery Voltage: "));
  DBG_PRINTLN(batteryVoltage);

  if (!sht45Ready) {
    DBG_PRINTLN(F("retry SHT45 init..."));
    sht45Ready = initSht45();
    delay(5000);
    return;
  }

  float tempC = 0.0f;
  float humidityRaw = 0.0f;
  uint8_t humidityPct = 0;
  int sensorError = SENSOR_OK;
  int lastReadErr = 0;

  for (uint8_t attempt = 0; attempt < SENSOR_READ_RETRIES; attempt++) {
    if (!sht45.read(tempC, humidityRaw, lastReadErr)) {
      sensorError = lastReadErr;
      DBG_PRINT(F("SHT45 read error: "));
      DBG_PRINTLN(lastReadErr);
      delay(SENSOR_RETRY_MS);
      continue;
    }

    if (!isValidReading(tempC, humidityRaw)) {
      sensorError = SENSOR_ERR_INVALID;
      DBG_PRINT(F("SHT45 invalid: "));
      DBG_PRINT(tempC, 2);
      DBG_PRINT(F(" C  "));
      DBG_PRINT(humidityRaw, 1);
      DBG_PRINTLN(F(" %"));
      delay(SENSOR_RETRY_MS);
      continue;
    }

    sensorError = SENSOR_OK;
    humidityPct = (uint8_t)(humidityRaw + 0.5f);
    if (attempt > 0) {
      DBG_PRINT(F("SHT45 OK on retry "));
      DBG_PRINTLN(attempt + 1);
    }
    break;
  }

  if (sensorError != SENSOR_OK) {
    DBG_PRINT(F("no TX (sensorError="));
    DBG_PRINT(sensorError);
    DBG_PRINTLN(F(")"));
  } else {
    if (ii != 0) {
      Wt450Data payload = makeWt450Payload(WT450_HOUSE_CODE, WT450_CHANNEL, tempC, humidityPct, true);
      //Wt450Data payload = makeWt450Payload(batteryVoltage*10, humidityPct);
      uint8_t pkt[5];

      DBG_PRINT(F("sending: ("));
      DBG_PRINT(ii, 2);
      DBG_PRINT(F(") "));
      DBG_PRINT(tempC, 2);
      DBG_PRINT(F(" C "));
      DBG_PRINT(humidityPct);
      DBG_PRINT(F(" % "));

      for (uint8_t seq = 0; seq < 3; seq++) {
        if (wt450.buildPreview(payload, seq, pkt)) {
          DBG_PRINT(F("seq "));
          DBG_PRINT(seq);
          DBG_PRINT(F("  pkt="));
          for (uint8_t i = 0; i < 5; i++) {
            if (pkt[i] < 0x10) {
              DBG_PRINT('0');
            }
            DBG_PRINT(pkt[i], HEX);
            DBG_PRINT(' ');
          }
          DBG_PRINT(F(" pulses="));
          DBG_PRINT(Wt450Sender::expectedPulseCount(pkt));
          DBG_PRINT(F(" "));
        }
      }

      if (wt450.send(payload)) {
        DBG_PRINTLN(F(" sent OK"));
        ii++;
        if (ii >= 4) ii = 0;
      } else {
        DBG_PRINTLN(F(" SEND FAIL"));
      }
    } else {
      Wt450Data payload_b = makeWt450Payload(WT450_HOUSE_CODE, 4, batteryVoltage * 10, batteryVoltage * 10, true);
      uint8_t pkt[5];

      DBG_PRINT(F("sending: ("));
      DBG_PRINT(ii, 2);
      DBG_PRINT(F(") "));
      DBG_PRINT(batteryVoltage * 10, 2);
      DBG_PRINT(F(" V "));
      DBG_PRINT(int(batteryVoltage * 10));
      DBG_PRINT(F(" V "));

      for (uint8_t seq = 0; seq < 3; seq++) {
        if (wt450.buildPreview(payload_b, seq, pkt)) {
          DBG_PRINT(F("seq "));
          DBG_PRINT(seq);
          DBG_PRINT(F("  pkt="));
          for (uint8_t i = 0; i < 5; i++) {
            if (pkt[i] < 0x10) {
              DBG_PRINT('0');
            }
            DBG_PRINT(pkt[i], HEX);
            DBG_PRINT(' ');
          }
          DBG_PRINT(F(" pulses="));
          DBG_PRINT(Wt450Sender::expectedPulseCount(pkt));
          DBG_PRINT(F(" "));
        }
      }

      if (wt450.send(payload_b)) {
        DBG_PRINTLN(F(" sent OK"));
        ii++;
        if (ii >= 4) ii = 0;
      } else {
        DBG_PRINTLN(F(" SEND FAIL"));
      }
    }
    sleepMs(SEND_INTERVAL_MS);
  }
}

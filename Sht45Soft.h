#ifndef SHT45_SOFT_H
#define SHT45_SOFT_H

#include <Arduino.h>

// Sensirion SHT45 (SHT4x family) — soft I2C for LilyPad 8 MHz
// Datasheet: 0x44 addr, command 0xFD = high-precision measure

class Sht45Soft {
public:
  enum MeasureCmd : uint8_t {
    MEASURE_HIGH   = 0xFD,  // high precision, ~8.3 ms
    MEASURE_MEDIUM = 0xF6,  // medium precision, ~4.3 ms
    MEASURE_LOW    = 0xE0,  // low precision, ~1.7 ms
  };

  Sht45Soft(uint8_t sdaPin, uint8_t sclPin, uint8_t address = 0x44);

  void begin();
  void scanBus();
  void printBusLevels() const;
  bool probe() const;
  bool init(MeasureCmd cmd = MEASURE_HIGH);
  bool read(float &tempC, float &humidityPct, int &errorCode);

  uint8_t address() const { return address_; }
  MeasureCmd measureCmd() const { return measureCmd_; }

private:
  uint8_t sdaPin_;
  uint8_t sclPin_;
  uint8_t address_;
  MeasureCmd measureCmd_;
  uint8_t measureDelayMs_;

  static void disableHardwareTwi();
  void busRecover() const;
  void delayHalfBit() const;
  void delayBetweenFrames() const;
  bool waitSclHigh(uint32_t timeoutUs) const;

  void releaseSda() const;
  void releaseScl() const;
  void driveSdaLow() const;
  void driveSclLow() const;
  bool readSda() const;

  bool i2cStart() const;
  void i2cStop() const;
  bool i2cWriteByte(uint8_t value) const;
  bool i2cReadByte(uint8_t &value, bool ack) const;

  bool writeBytes(const uint8_t *data, uint8_t len) const;
  bool readBytes(uint8_t *buf, uint8_t len) const;
  static uint8_t crc8(const uint8_t *data, uint8_t len);

  bool softReset() const;
  bool measureAndRead(float &tempC, float &humidityPct, int &errorCode);
  static uint8_t measureDelayMsFor(MeasureCmd cmd);
};

#endif

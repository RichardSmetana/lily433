#include "Sht45Soft.h"
#include "Dbg.h"

enum {
  SHT45_OK = 0,
  SHT45_ERR_START = 1,
  SHT45_ERR_CMD = 2,
  SHT45_ERR_READ = 3,
  SHT45_ERR_CRC_TEMP = 4,
  SHT45_ERR_CRC_HUM = 5,
};

static const uint8_t SHT45_CMD_RESET_0 = 0x94;
static const uint8_t SHT45_CMD_RESET_1 = 0x88;

Sht45Soft::Sht45Soft(uint8_t sdaPin, uint8_t sclPin, uint8_t address)
  : sdaPin_(sdaPin),
    sclPin_(sclPin),
    address_(address),
    measureCmd_(MEASURE_HIGH),
    measureDelayMs_(10) {}

uint8_t Sht45Soft::measureDelayMsFor(MeasureCmd cmd) {
  switch (cmd) {
    case MEASURE_HIGH:   return 10;
    case MEASURE_MEDIUM: return 5;
    case MEASURE_LOW:    return 2;
    default:             return 10;
  }
}

void Sht45Soft::disableHardwareTwi() {
#if defined(TWCR)
  TWCR = 0;
  delayMicroseconds(100);
#endif
}

void Sht45Soft::begin() {
  disableHardwareTwi();
  busRecover();
}

void Sht45Soft::delayHalfBit() const {
  delayMicroseconds(25);
}

void Sht45Soft::delayBetweenFrames() const {
  delay(3);
}

void Sht45Soft::releaseSda() const {
  pinMode(sdaPin_, INPUT_PULLUP);
}

void Sht45Soft::releaseScl() const {
  pinMode(sclPin_, INPUT_PULLUP);
}

void Sht45Soft::driveSdaLow() const {
  pinMode(sdaPin_, OUTPUT);
  digitalWrite(sdaPin_, LOW);
}

void Sht45Soft::driveSclLow() const {
  pinMode(sclPin_, OUTPUT);
  digitalWrite(sclPin_, LOW);
}

bool Sht45Soft::readSda() const {
  return digitalRead(sdaPin_) == HIGH;
}

bool Sht45Soft::waitSclHigh(uint32_t timeoutUs) const {
  uint32_t start = micros();
  while (digitalRead(sclPin_) == LOW) {
    if ((micros() - start) > timeoutUs) {
      return false;
    }
    delayMicroseconds(5);
  }
  return true;
}

void Sht45Soft::busRecover() const {
  disableHardwareTwi();
  releaseSda();
  releaseScl();
  delay(2);

  if (readSda()) {
    return;
  }

  driveSclLow();
  for (uint8_t i = 0; i < 9; i++) {
    releaseScl();
    delayHalfBit();
    driveSclLow();
    delayHalfBit();
  }

  driveSdaLow();
  releaseScl();
  delayHalfBit();
  releaseSda();
  delayHalfBit();
  releaseScl();
  delay(2);
}

void Sht45Soft::printBusLevels() const {
  disableHardwareTwi();
  releaseSda();
  releaseScl();
  delay(1);
  DBG_PRINT(F("bus levels SDA="));
  DBG_PRINT(readSda() ? '1' : '0');
  DBG_PRINT(F(" SCL="));
  DBG_PRINTLN(digitalRead(sclPin_) == HIGH ? '1' : '0');
}

void Sht45Soft::scanBus() {
  disableHardwareTwi();
  DBG_PRINTLN(F("Soft-I2C scan:"));
  uint8_t found = 0;
  uint8_t saved = address_;

  for (uint8_t addr = 1; addr < 127; addr++) {
    address_ = addr;
    if (probe()) {
      DBG_PRINT(F("  0x"));
      if (addr < 16) {
        DBG_PRINT('0');
      }
      DBG_PRINTLN(addr, HEX);
      found++;
    }
  }

  address_ = saved;
  if (found == 0) {
    DBG_PRINTLN(F("  (no devices)"));
  }
}

bool Sht45Soft::i2cStart() const {
  releaseSda();
  releaseScl();
  delayHalfBit();

  if (!readSda()) {
    return false;
  }
  if (!waitSclHigh(5000)) {
    return false;
  }

  driveSdaLow();
  delayHalfBit();
  driveSclLow();
  delayHalfBit();
  return true;
}

void Sht45Soft::i2cStop() const {
  driveSdaLow();
  delayHalfBit();
  if (!waitSclHigh(5000)) {
    driveSclLow();
  }
  releaseScl();
  delayHalfBit();
  releaseSda();
  delayHalfBit();
}

bool Sht45Soft::i2cWriteByte(uint8_t value) const {
  for (int8_t bit = 7; bit >= 0; bit--) {
    if (value & (1 << bit)) {
      releaseSda();
    } else {
      driveSdaLow();
    }
    delayHalfBit();
    driveSclLow();
    delayHalfBit();
    releaseScl();
    if (!waitSclHigh(5000)) {
      driveSclLow();
      return false;
    }
    delayHalfBit();
    driveSclLow();
    delayHalfBit();
  }

  releaseSda();
  delayHalfBit();
  driveSclLow();
  delayHalfBit();
  releaseScl();
  if (!waitSclHigh(25000)) {
    driveSclLow();
    return false;
  }
  delayHalfBit();
  bool ack = !readSda();
  driveSclLow();
  delayHalfBit();
  return ack;
}

bool Sht45Soft::i2cReadByte(uint8_t &value, bool ack) const {
  value = 0;
  releaseSda();

  for (uint8_t bit = 0; bit < 8; bit++) {
    delayHalfBit();
    driveSclLow();
    delayHalfBit();
    releaseScl();
    if (!waitSclHigh(25000)) {
      driveSclLow();
      return false;
    }
    delayHalfBit();
    if (readSda()) {
      value |= (1 << (7 - bit));
    }
    driveSclLow();
    delayHalfBit();
  }

  if (ack) {
    driveSdaLow();
  } else {
    releaseSda();
  }
  delayHalfBit();
  driveSclLow();
  delayHalfBit();
  releaseScl();
  if (!waitSclHigh(5000)) {
    driveSclLow();
    return false;
  }
  delayHalfBit();
  driveSclLow();
  delayHalfBit();
  releaseSda();
  return true;
}

bool Sht45Soft::writeBytes(const uint8_t *data, uint8_t len) const {
  if (!i2cStart()) {
    return false;
  }
  if (!i2cWriteByte((uint8_t)(address_ << 1))) {
    i2cStop();
    return false;
  }
  for (uint8_t i = 0; i < len; i++) {
    delayMicroseconds(100);
    if (!i2cWriteByte(data[i])) {
      i2cStop();
      return false;
    }
  }
  i2cStop();
  return true;
}

bool Sht45Soft::readBytes(uint8_t *buf, uint8_t len) const {
  if (!i2cStart()) {
    return false;
  }
  if (!i2cWriteByte((uint8_t)((address_ << 1) | 1))) {
    i2cStop();
    return false;
  }
  for (uint8_t i = 0; i < len; i++) {
    if (!i2cReadByte(buf[i], i < (len - 1))) {
      i2cStop();
      return false;
    }
  }
  i2cStop();
  return true;
}

uint8_t Sht45Soft::crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0xFF;
  for (uint8_t j = 0; j < len; j++) {
    crc ^= data[j];
    for (uint8_t i = 0; i < 8; i++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

bool Sht45Soft::probe() const {
  busRecover();
  if (!i2cStart()) {
    return false;
  }
  bool ack = i2cWriteByte((uint8_t)(address_ << 1));
  i2cStop();
  delayBetweenFrames();
  return ack;
}

bool Sht45Soft::softReset() const {
  const uint8_t cmd[] = {SHT45_CMD_RESET_0, SHT45_CMD_RESET_1};
  return writeBytes(cmd, 2);
}

bool Sht45Soft::init(MeasureCmd cmd) {
  measureCmd_ = cmd;
  measureDelayMs_ = measureDelayMsFor(cmd);
  busRecover();

  if (!probe()) {
    return false;
  }

  softReset();
  delay(2);
  return true;
}

bool Sht45Soft::measureAndRead(float &tempC, float &humidityPct, int &errorCode) {
  const uint8_t cmd[] = {(uint8_t)measureCmd_};
  if (!writeBytes(cmd, 1)) {
    errorCode = SHT45_ERR_CMD;
    return false;
  }

  delayBetweenFrames();
  delay(measureDelayMs_);

  uint8_t buf[6];
  if (!readBytes(buf, 6)) {
    errorCode = SHT45_ERR_READ;
    return false;
  }

  if (crc8(buf, 2) != buf[2]) {
    errorCode = SHT45_ERR_CRC_TEMP;
    return false;
  }
  if (crc8(buf + 3, 2) != buf[5]) {
    errorCode = SHT45_ERR_CRC_HUM;
    return false;
  }

  uint16_t rawT = ((uint16_t)buf[0] << 8) | buf[1];
  uint16_t rawH = ((uint16_t)buf[3] << 8) | buf[4];

  tempC = -45.0f + rawT * (175.0f / 65535.0f);
  humidityPct = -6.0f + rawH * (125.0f / 65535.0f);

  if (humidityPct > 100.0f) {
    humidityPct = 100.0f;
  }
  if (humidityPct < 0.0f) {
    humidityPct = 0.0f;
  }

  errorCode = SHT45_OK;
  return true;
}

bool Sht45Soft::read(float &tempC, float &humidityPct, int &errorCode) {
  busRecover();
  return measureAndRead(tempC, humidityPct, errorCode);
}

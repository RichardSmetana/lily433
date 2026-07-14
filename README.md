# lily433

[English](#english) · [Deutsch](#deutsch)

---

## English

WT450-TH RF transmitter emulator for **LilyPad Arduino 328 @ 8 MHz** with an SHT45 temperature/humidity sensor and STX882 433 MHz transmitter.

Reads temperature and humidity from the SHT45 via bit-banged I2C, transmits values using the WT450 protocol (rtl_433 #33), and alternates between sensor readings and battery voltage. Between transmission cycles the ATmega328 enters deep sleep to save power.

### Hardware

| Component | Connection |
|-----------|------------|
| LilyPad Arduino 328 | 8 MHz, internal oscillator |
| SHT45 (SHT4x) | SDA → A4, SCL → A5 |
| STX882 / 433 MHz transmitter | DATA → D3 |
| LiPo / battery | LilyPad power supply |

### Arduino IDE

1. Board: **LilyPad Arduino** (or MiniCore: ATmega328P, 8 MHz, BOD 2.7 V)
2. Open the sketch folder `lily433` (`lily433.ino`)
3. Upload

No external libraries required — everything is included in this project.

### Configuration

Adjustable in `lily433.ino`:

| Define | Default | Description |
|--------|---------|-------------|
| `SEND_INTERVAL_MS` | 60000 | Interval between transmissions (ms) |
| `WT450_HOUSE_CODE` | 1 | House code (1–15) |
| `WT450_CHANNEL` | 1 | Channel (1–4) |
| `TX_INVERT` | 0 | Invert transmitter polarity |
| `SERIAL_DEBUG` | 0 | Serial debug output (1 = on) |

### Project structure

```
lily433.ino      Main sketch
Sht45Soft.*      Software I2C driver for SHT45
Wt450Send.*      WT450 OOK transmitter (433 MHz)
LowPowerSleep.*  Watchdog deep sleep
Dbg.h            Serial debug macros
```

### Compatibility

Receivers: WT450-compatible weather stations, [rtl_433](https://github.com/merbanan/rtl_433) (protocol #33), Oregon-NR receivers with WT450 support.

### License

GPL-3.0 — see [LICENSE](LICENSE).

---

## Deutsch

WT450-TH-Funksender-Emulator für **LilyPad Arduino 328 @ 8 MHz** mit SHT45-Temperatur-/Feuchtesensor und STX882 433-MHz-Sender.

Liest Temperatur und Luftfeuchtigkeit per Software-I2C vom SHT45, sendet die Werte im WT450-Protokoll (rtl_433 #33) und wechselt zwischen Messwerten und Batteriespannung. Zwischen den Sendezyklen wird der ATmega328 in den Tiefschlaf versetzt, um Strom zu sparen.

### Hardware

| Komponente | Anschluss |
|------------|-----------|
| LilyPad Arduino 328 | 8 MHz, interner Quarz |
| SHT45 (SHT4x) | SDA → A4, SCL → A5 |
| STX882 / 433-MHz-Sender | DATA → D3 |
| LiPo / Batterie | LilyPad Versorgung |

### Arduino IDE

1. Board: **LilyPad Arduino** (oder MiniCore: ATmega328P, 8 MHz, BOD 2.7 V)
2. Sketch-Ordner `lily433` öffnen (`lily433.ino`)
3. Hochladen

Keine externen Bibliotheken nötig — alles ist im Projekt enthalten.

### Konfiguration

In `lily433.ino` anpassbar:

| Define | Standard | Bedeutung |
|--------|----------|-----------|
| `SEND_INTERVAL_MS` | 60000 | Pause zwischen Sendungen (ms) |
| `WT450_HOUSE_CODE` | 1 | Hauscode (1–15) |
| `WT450_CHANNEL` | 1 | Kanal (1–4) |
| `TX_INVERT` | 0 | Sender-Polarität invertieren |
| `SERIAL_DEBUG` | 0 | Serielle Debug-Ausgabe (1 = an) |

### Projektstruktur

```
lily433.ino      Hauptsketch
Sht45Soft.*      Software-I2C-Treiber für SHT45
Wt450Send.*      WT450-OOK-Sender (433 MHz)
LowPowerSleep.*  Watchdog-Tiefschlaf
Dbg.h            Serielle Debug-Makros
```

### Kompatibilität

Empfänger: WT450-kompatible Stationen, [rtl_433](https://github.com/merbanan/rtl_433) (Protokoll #33), Oregon-NR-Empfänger mit WT450-Unterstützung.

### Lizenz

GPL-3.0 — siehe [LICENSE](LICENSE).

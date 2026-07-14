# lily433

[English](#english) · [Deutsch](#deutsch)

---

## English

WT450-TH RF transmitter emulator for **LilyPad Arduino 328 @ 8 MHz** (or ATmega328P via MiniCore) with an SHT45 temperature/humidity sensor and STX882 433 MHz ASK transmitter.

The sketch reads temperature and humidity from the SHT45 over bit-banged I2C, encodes the values in the WT450 protocol ([rtl_433](https://github.com/merbanan/rtl_433) #33), and transmits them via OOK. Every fourth cycle sends battery voltage instead. Between transmissions the MCU enters watchdog deep sleep with peripherals powered down.

No external Arduino libraries are required.

### How it works

1. **Startup** — probe SHT45 on I2C addresses `0x44` / `0x45`, try high/medium/low precision measure commands, pick the first working combination.
2. **Measure** — read temperature and humidity (up to 3 retries on error or invalid values).
3. **Transmit** — send 3 WT450 OOK bursts (~19 ms gap) on channel `WT450_CHANNEL` with temp/humidity.
4. **Battery cycle** — every 4th transmission uses channel 4 and encodes battery voltage (read via internal 1.1 V bandgap reference).
5. **Sleep** — deep sleep for `SEND_INTERVAL_MS` (default 60 s); TX and I2C pins are released before sleep, sensor and transmitter re-initialised on wake.

If the sensor fails to initialise, the sketch retries every 5 s without transmitting.

### Hardware

| Component | Connection | Notes |
|-----------|------------|-------|
| LilyPad Arduino 328 / ATmega328P | 8 MHz, internal oscillator | MiniCore recommended |
| SHT45 (SHT4x family) | SDA → A4, SCL → A5 | Software I2C — do not use `Wire` |
| STX882 / 433 MHz ASK TX | DATA → D3 | Set `TX_INVERT` if polarity is wrong |
| LiPo / battery | LilyPad power | Voltage reported on channel 4 |

### Arduino IDE

1. Board: **LilyPad Arduino** or **MiniCore → ATmega328P**
2. Clock: **8 MHz internal**
3. Open sketch folder `lily433` (`lily433.ino`) and upload

**Recommended MiniCore fuse settings for lowest sleep current:**

| Fuse option | Setting |
|-------------|---------|
| BOD | **Disabled** |
| Clock | 8 MHz internal |

With BOD enabled (e.g. 2.7 V) the brown-out detector draws additional current during sleep. Disabling BOD in the fuses reduces sleep consumption further. Ensure stable battery supply when BOD is off.

### Power saving

The firmware is tuned for battery operation:

| Measure | Effect |
|---------|--------|
| `SERIAL_DEBUG 0` (default) | No `Serial.begin()` — UART stays off, less flash/RAM and no USART idle current |
| Deep sleep (`LowPowerSleep`) | `SLEEP_MODE_PWR_DOWN` via watchdog; ADC, timers, SPI, TWI and USART powered down |
| `sleep_bod_disable()` | BOD switched off for the duration of each sleep period |
| BOD fuse **disabled** | Lowest sleep current; BOD hardware stays off permanently |
| `prepareForSleep()` | TX pin LOW, I2C pins as high-Z inputs |

For minimum power: flash with **BOD disabled** and keep **`SERIAL_DEBUG` at 0**. Use `SERIAL_DEBUG 1` only while debugging over USB/FTDI (57600 baud).

### Configuration

Adjustable in `lily433.ino`:

| Define | Default | Description |
|--------|---------|-------------|
| `SEND_INTERVAL_MS` | 60000 | Pause between transmissions (ms) |
| `WT450_HOUSE_CODE` | 1 | House code (1–15) |
| `WT450_CHANNEL` | 1 | Channel for temp/humidity (1–4) |
| `TX_INVERT` | 0 | Invert ASK transmitter polarity |
| `SERIAL_DEBUG` | 0 | Serial debug output (`1` = on, 57600 baud) |
| `SHT45_I2C_ADDRESS` | 0x44 | Preferred SHT45 address |
| `SENSOR_READ_RETRIES` | 3 | Retries per measurement |
| `SENSOR_RETRY_MS` | 500 | Delay between read retries (ms) |

### Project structure

| File | Purpose |
|------|---------|
| `lily433.ino` | Main sketch — sensor read, battery check, TX scheduling, sleep |
| `Sht45Soft.*` | Bit-banged I2C driver for SHT45/SHT4x (CRC, bus recovery, address scan) |
| `Wt450Send.*` | WT450 OOK encoder/sender — 36-bit DMC frames, 3 bursts per packet |
| `LowPowerSleep.*` | Watchdog-based deep sleep with peripheral shutdown |
| `Dbg.h` | `DBG_*` macros — compile-time no-ops when `SERIAL_DEBUG` is 0 |

### Protocol & compatibility

- **Protocol:** WT450 / WT450H — OOK pulse DMC, rtl_433 decoder #33
- **Receivers:** WT450-compatible weather stations, [rtl_433](https://github.com/merbanan/rtl_433), Oregon-NR with WT450 support
- **Payload:** temperature (−50 … +205 °C encoded), humidity 0–99 %, battery-OK flag, rolling sequence 0–2

### License

GPL-3.0 — see [LICENSE](LICENSE).

---

## Deutsch

WT450-TH-Funksender-Emulator für **LilyPad Arduino 328 @ 8 MHz** (oder ATmega328P mit MiniCore) mit SHT45-Temperatur-/Feuchtesensor und STX882 433-MHz-ASK-Sender.

Der Sketch liest Temperatur und Luftfeuchtigkeit per Software-I2C vom SHT45, kodiert die Werte im WT450-Protokoll ([rtl_433](https://github.com/merbanan/rtl_433) #33) und sendet sie per OOK. Jeder vierte Zyklus überträgt stattdessen die Batteriespannung. Zwischen den Sendungen geht der MCU in Watchdog-Tiefschlaf mit abgeschalteten Peripherie-Modulen.

Keine externen Arduino-Bibliotheken nötig.

### Ablauf

1. **Start** — SHT45 an I2C-Adressen `0x44` / `0x45` suchen, Messbefehle high/medium/low testen, erste funktionierende Kombination verwenden.
2. **Messen** — Temperatur und Luftfeuchtigkeit lesen (bis zu 3 Wiederholungen bei Fehler oder ungültigen Werten).
3. **Senden** — 3 WT450-OOK-Bursts (~19 ms Abstand) auf Kanal `WT450_CHANNEL` mit Temp./Feuchte.
4. **Batterie-Zyklus** — jede 4. Übertragung nutzt Kanal 4 und kodiert die Batteriespannung (Messung über interne 1,1-V-Bandgap-Referenz).
5. **Schlaf** — Tiefschlaf für `SEND_INTERVAL_MS` (Standard 60 s); TX- und I2C-Pins werden vor dem Schlaf freigegeben, Sensor und Sender danach neu initialisiert.

Bei fehlgeschlagener Sensor-Initialisierung wird alle 5 s erneut versucht, ohne zu senden.

### Hardware

| Komponente | Anschluss | Hinweis |
|------------|-----------|---------|
| LilyPad Arduino 328 / ATmega328P | 8 MHz, interner Quarz | MiniCore empfohlen |
| SHT45 (SHT4x-Familie) | SDA → A4, SCL → A5 | Software-I2C — `Wire` nicht verwenden |
| STX882 / 433-MHz-ASK-Sender | DATA → D3 | `TX_INVERT` bei falscher Polarität setzen |
| LiPo / Batterie | LilyPad-Versorgung | Spannung wird auf Kanal 4 gemeldet |

### Arduino IDE

1. Board: **LilyPad Arduino** oder **MiniCore → ATmega328P**
2. Takt: **8 MHz intern**
3. Sketch-Ordner `lily433` (`lily433.ino`) öffnen und hochladen

**Empfohlene MiniCore-Fuse-Einstellungen für minimalen Schlafstrom:**

| Fuse-Option | Einstellung |
|-------------|-------------|
| BOD | **Deaktiviert** |
| Takt | 8 MHz intern |

Mit aktivem BOD (z. B. 2,7 V) verbraucht der Brown-Out-Detektor im Schlaf zusätzlichen Strom. BOD in den Fuses deaktivieren senkt den Schlafstrom weiter. Bei deaktiviertem BOD stabile Batterieversorgung sicherstellen.

### Stromsparen

Die Firmware ist auf Batteriebetrieb ausgelegt:

| Maßnahme | Wirkung |
|----------|---------|
| `SERIAL_DEBUG 0` (Standard) | Kein `Serial.begin()` — UART bleibt aus, weniger Flash/RAM und kein USART-Leerlaufstrom |
| Tiefschlaf (`LowPowerSleep`) | `SLEEP_MODE_PWR_DOWN` per Watchdog; ADC, Timer, SPI, TWI und USART abgeschaltet |
| `sleep_bod_disable()` | BOD während jedes Schlafzeitraums ausgeschaltet |
| BOD-Fuse **deaktiviert** | Geringster Schlafstrom; BOD-Hardware dauerhaft aus |
| `prepareForSleep()` | TX-Pin LOW, I2C-Pins als High-Z-Eingänge |

Für minimalen Verbrauch: mit **deaktiviertem BOD** flashen und **`SERIAL_DEBUG` auf 0** lassen. `SERIAL_DEBUG 1` nur zum Debuggen über USB/FTDI verwenden (57600 Baud).

### Konfiguration

In `lily433.ino` anpassbar:

| Define | Standard | Bedeutung |
|--------|----------|-----------|
| `SEND_INTERVAL_MS` | 60000 | Pause zwischen Sendungen (ms) |
| `WT450_HOUSE_CODE` | 1 | Hauscode (1–15) |
| `WT450_CHANNEL` | 1 | Kanal für Temp./Feuchte (1–4) |
| `TX_INVERT` | 0 | ASK-Sender-Polarität invertieren |
| `SERIAL_DEBUG` | 0 | Serielle Debug-Ausgabe (`1` = an, 57600 Baud) |
| `SHT45_I2C_ADDRESS` | 0x44 | Bevorzugte SHT45-Adresse |
| `SENSOR_READ_RETRIES` | 3 | Wiederholungen pro Messung |
| `SENSOR_RETRY_MS` | 500 | Pause zwischen Leseversuchen (ms) |

### Projektstruktur

| Datei | Aufgabe |
|-------|---------|
| `lily433.ino` | Hauptsketch — Sensorlesen, Batteriespannung, Sendeplanung, Schlaf |
| `Sht45Soft.*` | Software-I2C-Treiber für SHT45/SHT4x (CRC, Bus-Recovery, Adress-Scan) |
| `Wt450Send.*` | WT450-OOK-Encoder/Sender — 36-Bit-DMC-Frames, 3 Bursts pro Paket |
| `LowPowerSleep.*` | Watchdog-Tiefschlaf mit Peripherie-Abschaltung |
| `Dbg.h` | `DBG_*`-Makros — Compile-Time-No-Ops bei `SERIAL_DEBUG` 0 |

### Protokoll & Kompatibilität

- **Protokoll:** WT450 / WT450H — OOK-Pulse DMC, rtl_433-Decoder #33
- **Empfänger:** WT450-kompatible Wetterstationen, [rtl_433](https://github.com/merbanan/rtl_433), Oregon-NR mit WT450-Unterstützung
- **Nutzdaten:** Temperatur (−50 … +205 °C kodiert), Luftfeuchtigkeit 0–99 %, Batterie-OK-Flag, Sequenz 0–2

### Lizenz

GPL-3.0 — siehe [LICENSE](LICENSE).

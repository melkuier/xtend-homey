# Intergas Xtend to Homey (ESP bridge)

Reads an Intergas Xtend heat pump in near real time and exposes the values as JSON (for Homey or anything else).

The Xtend has its own wifi (`10.20.30.1`), separate from your home network, and a single ESP can only be on one of them at a time. One ESP can switch between the two, but then the data is only fresh and reachable part of the time (see `../docs/xtend-research.md`). For continuous, near-real-time data we use two ESPs connected by a wire:

```
Xtend wifi  ->  READER (ESP A)  --wire (UART)-->  GATEWAY (ESP B)  -> home wifi -> Homey
```

- **xtend_reader** (ESP A): joins the Xtend wifi, polls the values and sends them over the wire.
- **xtend_gateway** (ESP B): joins your home wifi and serves JSON on `http://<ip>/`, `/status` and `/health`.

Either one can be an **ESP8266** (e.g. NodeMCU) or an **ESP32** (WROOM-32).

## Tested combinations

| Reader (A) | Gateway (B) | Status |
|---|---|---|
| ESP8266 | ESP32 WROOM-32 | **tested, works** |
| ESP8266 | ESP8266 | untested |
| ESP32 | ESP32 | untested |
| ESP32 | ESP8266 | untested |

The untested combinations are written but have never run on real hardware. Please report problems as issues.

## Wiring

There are **three** connections: data from reader to gateway, a command wire back (only needed for the update feature) and GND. All chips are 3.3 V; no level shifter needed.

| Wire | Reader (A) | Gateway (B) |
|---|---|---|
| Data (reader to gateway) | ESP8266: GPIO2 (D4) or ESP32: GPIO17 (TX2) | ESP32: GPIO16 (RX2) or ESP8266: GPIO13 (D7) |
| Command (gateway to reader) | ESP8266: GPIO14 (D5) or ESP32: GPIO16 (RX2) | ESP32: GPIO17 (TX2) or ESP8266: GPIO15 (D8) |
| Ground | GND | GND |

So with an ESP8266 reader and an ESP32 gateway: D4 to GPIO16, GPIO17 to D5, and GND to GND.

Notes:
- An ESP8266 gateway moves the hardware UART (`Serial.swap()`), so USB serial output is unavailable; use `/health`.
- An ESP32 WROVER uses GPIO16/17 for PSRAM: choose other pins.
- The command wire is optional: everything works without it except updating the reader over wifi.

## Install

1. Arduino IDE with the ESP8266 and/or ESP32 board packages, and the **ArduinoJson 7** library.
2. Open `xtend_reader/xtend_reader.ino`. Copy `secrets.h.example` to `secrets.h` and enter the Xtend wifi details.
3. Open `xtend_gateway/xtend_gateway.ino`. Copy `secrets.h.example` to `secrets.h` and enter your home wifi, static IP and OTA password.
4. Select the right board and flash. An ESP32 sometimes only flashes at a low speed (Upload Speed 115200) with BOOT held down.
5. Check `http://<gateway-ip>/health`: `frames_ok` should increase once the wire works. Then open `http://<gateway-ip>/` for all values.

The Xtend wifi closes itself after 30 minutes of inactivity; the reader polls every 50 seconds, which keeps it open.

## Adding or removing fields

Edit `fields.h` and keep the copies in both sketch folders identical.

## OTA updates

- **Gateway:** directly over wifi, hostname and password from `secrets.h` (Arduino IDE, port `xtend-gateway`).
- **Reader:** it sits on the Xtend wifi. Open `http://<gateway-ip>/update-reader` and press the button. The gateway sends a command over the command wire, the reader switches to your home wifi for ten minutes (set in `secrets.h`) and shows up in the Arduino IDE as port `xtend-reader`. After the update or after ten minutes it reboots and returns to the Xtend. No data arrives in that time. The very first flash of the reader is still by cable.
- Anyone who can reach `/update-reader` can put the reader in update mode; flashing itself needs the OTA password.

## Stale data

If no frame arrives from the reader for more than 5 minutes, the gateway returns `"status": "geen data"` for every field. The Homey app (1.6.6 and later) then blanks the tiles instead of showing old values. `/health` shows `data_stale`, `reader_update_mode` and the version.

## Inspired by

This project contains no code from the projects below, but was inspired by them. The Xtend field codes come from the work of others, and the idea of two ESPs connected by a wire was also used elsewhere:
- [HA_connection_Xtend](https://github.com/DSchoutsen/HA_connection_Xtend) by Dennis Schoutsen (Home Assistant)
- [xtend-bridge](https://github.com/thomasvt1/xtend-bridge) by Thomas van Tilburg
- [XTEND-plugin](https://github.com/WillemD61/XTEND-plugin) by WillemD61 (Domoticz)
- [esphome-xtend-wifi-wifi](https://github.com/sanderlv/esphome-xtend-wifi-wifi) by sanderlv (ESPHome, two ESP32s with UART)

Built with help from Claude (Anthropic).

## License

MIT, see `LICENSE`. Not affiliated with Intergas. Use at your own risk; the Xtend is only read, never controlled.

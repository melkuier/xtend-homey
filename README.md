# Intergas Xtend to Homey

Read an **Intergas Xtend** heat pump in near real time and use it in **Homey**: temperatures, power, COP, defrost sessions, status and more, including the Homey Energy dashboard.

> Unofficial hobby project, not affiliated with Intergas. It only reads the Xtend, it never controls it. Use at your own risk.

## How it works

The Xtend only has its own wifi (`10.20.30.1`), separate from your home network. A single ESP can read it by switching between the two networks, but then the values are only fresh and reachable part of the time. To keep the data continuously available and near real time, this project uses two ESPs connected by a wire:

```
Xtend wifi -> READER (ESP A) --wire--> GATEWAY (ESP B) -> home wifi -> Homey app
```

| Folder | What it is |
|---|---|
| [`esp/`](esp) | Arduino sketches for the reader and the gateway (ESP8266 or ESP32), wiring tables, OTA updates |
| [`homey-app/`](homey-app) | The Homey Pro app that reads the gateway |

Start with the ESPs ([`esp/README.md`](esp/README.md)), then install the Homey app ([`homey-app/README.md`](homey-app/README.md)).

## Research notes

How the Xtend API works, which codes mean what, and which limits we ran into (polling interval, 30-minute wifi timeout, not-connected sensors, OpenTherm) are in [`docs/xtend-research.md`](docs/xtend-research.md).

## Status

- Reader ESP8266 + gateway ESP32 (WROOM-32): **tested, running at home**, including updating the reader over wifi via the gateway.
- Other chip combinations are written but **untested**. Issues and reports are welcome.
- Homey app: runs on a Homey Pro (Homey 12 or newer). Not yet in the Homey App Store.

## Inspired by

This project contains no code from the projects below, but was inspired by them. The Xtend field codes come from the work of others, and the idea of two ESPs connected by a wire was also used elsewhere:
- [HA_connection_Xtend](https://github.com/DSchoutsen/HA_connection_Xtend) by Dennis Schoutsen (Home Assistant)
- [xtend-bridge](https://github.com/thomasvt1/xtend-bridge) by Thomas van Tilburg
- [XTEND-plugin](https://github.com/WillemD61/XTEND-plugin) by WillemD61 (Domoticz)
- [esphome-xtend-wifi-wifi](https://github.com/sanderlv/esphome-xtend-wifi-wifi) by sanderlv (ESPHome, two ESP32s with UART)

Built with help from Claude (Anthropic).

## License

MIT, see [`LICENSE`](LICENSE).

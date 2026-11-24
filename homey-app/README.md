# Intergas Xtend Monitor (Homey app)

Homey Pro app that shows the data of an Intergas Xtend heat pump: temperatures, flow, pressure, power, COP, status flags, defrost, counters and more. It also feeds the **Homey Energy** dashboard (live power in W and a cumulative kWh counter).

The Xtend only has its own wifi, so the app does not talk to it directly. It reads a small JSON server on your home network, provided by the ESP gateway in [`../esp`](../esp). Set that up first.

Unofficial project, not affiliated with Intergas.

## Install

Requires a Homey Pro (Homey 12.0.0 or newer) and the [Homey CLI](https://apps.developer.homey.app/the-basics/getting-started).

```
npm install -g homey
homey login
cd homey-app
homey app install
```

(`homey app run` runs it temporarily with live logs.) Then add the device in Homey: **Add device, Intergas Xtend Monitor, Intergas Xtend**, and enter the IP address of the gateway in the device settings.

## What you get

- Temperatures, system flow and pressure, thermal power, COP, compressor frequency.
- Heat pump and boiler status as readable text (taken from the Xtend web app), plus yes/no tiles for burner, heat demand, compressor, circulation pump, silent mode and defrost.
- Energy: live electrical power and a cumulative electricity counter for Homey Energy.
- Insights for temperatures, power, COP and a defrost graph (1 while defrosting, otherwise 0).
- Sensors that are not connected show no value instead of a fake number.
- Every value can be switched on or off in the device settings.
- When the gateway has had no data from the reader for a while, the tiles are emptied instead of showing old values.

## Changelog

See [CHANGELOG.md](CHANGELOG.md).

## License

MIT, see `LICENSE`.

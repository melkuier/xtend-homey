# Xtend research notes

What we found while reading an Intergas Xtend (firmware V1.30, with an Intergas Xtreme CW6 boiler). Written down so others do not have to repeat it. **Confirmed** means we saw it on our own installation. **Assumption** means it is a reasonable reading that is not verified. Other installations (other boilers, firmware, sensors) may differ.

## The local API

- The Xtend creates its own wifi (`10.20.30.1`). Its web app and this project use `GET http://10.20.30.1/api/stats/values?fields=<code>,<code>,...`.
- A field is a 4-digit hex code. The answer holds raw integers; the real value is raw times a factor (temperatures 0.01, power in kW 0.001, COP 0.1, and so on). The factors are in `esp/*/fields.h`.
- Confirmed: the web app itself asks for up to about 83 fields in one request. We use batches of 24.
- Confirmed: a raw value of `32767` means "no measurement" (a sensor that is not connected). With a factor of 0.01 that shows up as `327.67`. We turn that into `null` ("niet aangesloten").
- Confirmed: asking for everything every 30 seconds made the Xtend stop answering after a few minutes. At 50 seconds per round it has run for days. We do not know the exact limit.
- From the Domoticz plugin documentation, not tested by us: the Xtend wifi switches itself off after 30 minutes without traffic. Regular polling keeps it open. If it does close, the button on the Xtend has to be pressed again (purple flashing LED).
- Firmware changes can rename or drop fields. Other projects mention that a field present in V0.86 was gone later. We only tested V1.30.

## One ESP or two?

One ESP is possible: it reads the Xtend, then switches to your home wifi to hand over the data, and switches back. That is how this project started. The downside is that the values are only reachable and fresh for part of the time, because the ESP is on the Xtend wifi while it reads and every switch takes seconds.

We chose two ESPs for a continuously reachable, near-real-time result: the reader stays on the Xtend wifi all the time and the gateway stays on your network all the time, with a wire between them. Homey can ask the gateway at any moment. The price is a second chip and a wire.

## What the fields mean

### Status flags (confirmed, from the Xtend web app)

`f9f2` (status flags), counting bits from 0:

| Bit | Meaning |
|---|---|
| 7 | Heat demand heat pump |
| 8 | Heat demand boiler |
| 9 | Boiler active ("Flame") |
| 14 | Defrost active |

`77c3`: bit 3 is silent mode. `77d2`: bit 1 is the central heating pump, bit 5 the compressor running, bit 7 the boiler connected via OpenTherm.

### Status texts (taken from the Xtend web app)

`7e51` (device status) is a code that the web app shows as text, for example 102 "Heating", 104 "Defrost", 126 "Stand-by", 127 "Disabled", 204 "Hot water". The full table is in `homey-app/drivers/xtend/device.js` (`DEVICE_STATUS_TEXT`). `6578` (operating mode): 0 cooling, 1 heating, 2 defrost, 3 pump down, 255 undetermined.

### Power and energy

- `50f2` is the electrical power in kW (raw times 0.001). The Homey app turns this into watts. It reads about 300 to 400 W idle and up to about 1.1 kW while the compressor ramps up, in cycles of roughly 15 minutes on our system.
- The cumulative counter for Homey Energy is the sum of the three electricity totals: heating (`63b3`), hot water (`6358`) and cooling (`63a7`).
- `5041` is the current COP (raw times 0.1).

### Boiler and OpenTherm

- The Xtend talks to the gas boiler over OpenTherm. With our boiler the Xtend does not receive a boiler return temperature or setpoint, so those fields are empty. That depends on the boiler.
- `843a` and `8457` look like OpenTherm status bytes. **Assumption:** the standard layout (bit 0 fault, 1 central heating mode, 2 hot water mode, 3 flame) applies. The app shows these bits as extra tiles; compare them with the Xtend page before relying on them.
- Fields that always read 0 on our installation and that the web app does not use were left out: ignition counters, gas use for hot water, the three MSB fields. Room temperature and room setpoint were not available either (no RF room thermostat reporting).

### Others

- `62e7` is the flow temperature of the heat pump. `621d` ("system flow") read the same on our system, so the Homey app dropped it.
- Counters (compressor starts, running hours, defrost cycles) are cumulative; the daily values ("yesterday") reset once per day.

## Other projects

See "Inspired by" in the main README. Their field lists and our own measurements (captured from the web app's requests) agree on most codes; where they did not we followed what our own Xtend returned.

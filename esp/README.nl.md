# Intergas Xtend naar Homey (ESP-bridge)

Leest een Intergas Xtend warmtepomp realtime uit en maakt de waarden beschikbaar voor Homey (of andere systemen) als JSON.

De Xtend heeft een eigen wifi (`10.20.30.1`) en jouw netwerk is een ander netwerk. Een ESP zit maar op één van de twee tegelijk. Eén ESP kan wisselen, maar dan is de data maar een deel van de tijd vers en bereikbaar (zie `../docs/xtend-research.md`). Voor continue, bijna realtime data gebruiken we twee ESP's, verbonden met een draad:

```
Xtend-wifi  ->  LEZER (ESP A)  --draad (UART)-->  GATEWAY (ESP B)  -> thuiswifi -> Homey
```

- **xtend_reader** (ESP A): zit op het wifi van de Xtend, haalt de waarden op en stuurt ze over de draad.
- **xtend_gateway** (ESP B): zit op je thuiswifi en serveert de JSON op `http://<ip>/`, `/status` en `/health`.

Beide kunnen een **ESP8266** (bijv. NodeMCU) of een **ESP32** (WROOM-32) zijn.

## Welke combinaties zijn getest?

| Lezer (A) | Gateway (B) | Status |
|---|---|---|
| ESP8266 | ESP32 WROOM-32 | **getest, werkt** |
| ESP8266 | ESP8266 | niet getest |
| ESP32 | ESP32 | niet getest |
| ESP32 | ESP8266 | niet getest |

De niet-geteste combinaties zijn geschreven maar nog nooit op echte hardware gedraaid. Meld problemen als issue.

## Bedrading

Er zijn **drie** verbindingen: data van lezer naar gateway, een commandodraad terug (alleen nodig voor de updatefunctie) en GND. Alle chips zijn 3,3 V; een levelshifter is niet nodig.

| Draad | Lezer (A) | Gateway (B) |
|---|---|---|
| Data (lezer naar gateway) | ESP8266: GPIO2 (D4) of ESP32: GPIO17 (TX2) | ESP32: GPIO16 (RX2) of ESP8266: GPIO13 (D7) |
| Commando (gateway naar lezer) | ESP8266: GPIO14 (D5) of ESP32: GPIO16 (RX2) | ESP32: GPIO17 (TX2) of ESP8266: GPIO15 (D8) |
| Aarde | GND | GND |

Dus bij een ESP8266-lezer en ESP32-gateway: D4 naar GPIO16, GPIO17 naar D5, en GND naar GND.

Let op:
- Op een ESP8266-gateway wordt de hardware-UART verplaatst (`Serial.swap()`). USB-meldingen werken dan niet; gebruik `/health`.
- Een ESP32 WROVER gebruikt GPIO16/17 voor PSRAM: kies dan andere pinnen.
- De commandodraad is optioneel: zonder werkt alles, behalve het updaten van de lezer via wifi.

## Installeren

1. Arduino IDE met de ESP8266- en/of ESP32-boardpakketten, en de bibliotheek **ArduinoJson 7**.
2. Open `xtend_reader/xtend_reader.ino`. Kopieer `secrets.h.example` naar `secrets.h` en vul het wifi van de Xtend in.
3. Open `xtend_gateway/xtend_gateway.ino`. Kopieer `secrets.h.example` naar `secrets.h` en vul je thuiswifi, vast IP en OTA-wachtwoord in.
4. Kies het juiste board en flash. De ESP32 flasht soms alleen goed op lage snelheid (Upload Speed 115200) en met de BOOT-knop ingedrukt.
5. Test: `http://<ip-van-gateway>/health` laat `frames_ok` oplopen zodra de draad werkt. Daarna `http://<ip-van-gateway>/` voor alle waarden.

Op de Xtend moet de wifi actief blijven (die sluit zelf na 30 minuten). De lezer vraagt elke 50 seconden alle velden op, wat dit open houdt.

## Velden toevoegen of weghalen

Dat gaat in `fields.h`. Houd dat bestand in beide mappen gelijk.

## Homey-app

In de map `../homey-app` van deze repository. Die haalt de JSON van de gateway op en maakt er apparaten, Insights en Energie van.

## Updaten via wifi (OTA)

- **Gateway:** direct via wifi, hostname en wachtwoord uit `secrets.h` (Arduino IDE, Poort `xtend-gateway`).
- **Lezer:** die zit op het wifi van de Xtend. Open `http://<ip-gateway>/update-reader` en druk op de knop. De gateway stuurt dan een commando over de commandodraad, de lezer schakelt tien minuten over naar je thuiswifi (instellen in `secrets.h`) en verschijnt in de Arduino IDE als poort `xtend-reader`. Na de update of na tien minuten herstart hij en gaat terug naar de Xtend. In die tijd komt er geen data binnen. Het eerste flashen van de lezer blijft met de kabel.
- Wie bij `/update-reader` kan, kan de lezer in updatemodus zetten. Flashen kan alleen met het OTA-wachtwoord.

## Oude data

Komt er langer dan 5 minuten geen frame meer van de lezer, dan geeft de gateway `"status": "geen data"` bij alle velden. De Homey-app (vanaf 1.6.6) maakt dan de tegels leeg in plaats van oude waarden te tonen. `/health` toont `data_stale`, `reader_update_mode` en de versie.

## Geïnspireerd door

Dit project bevat geen code uit onderstaande projecten, maar is er wel door geïnspireerd. De veldcodes van de Xtend komen uit het werk van anderen en het idee van twee ESP's met een draad ertussen kwam ook elders voor:
- [HA_connection_Xtend](https://github.com/DSchoutsen/HA_connection_Xtend) van Dennis Schoutsen (Home Assistant)
- [xtend-bridge](https://github.com/thomasvt1/xtend-bridge) van Thomas van Tilburg
- [XTEND-plugin](https://github.com/WillemD61/XTEND-plugin) van WillemD61 (Domoticz)
- [esphome-xtend-wifi-wifi](https://github.com/sanderlv/esphome-xtend-wifi-wifi) van sanderlv (ESPHome, twee ESP32's met UART)

Gebouwd met hulp van Claude (Anthropic).

## Licentie

MIT, zie `LICENSE`. Niet gelieerd aan Intergas. Gebruik op eigen risico; de Xtend wordt alleen gelezen, nooit aangestuurd.

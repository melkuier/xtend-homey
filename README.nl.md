# Intergas Xtend naar Homey

Lees een **Intergas Xtend** warmtepomp bijna realtime uit en gebruik hem in **Homey**: temperaturen, vermogen, COP, ontdooisessies, status en meer, inclusief het Homey Energie-dashboard.

> Onofficieel hobbyproject, niet gelieerd aan Intergas. De Xtend wordt alleen gelezen, nooit aangestuurd. Gebruik op eigen risico.

## Hoe het werkt

De Xtend heeft alleen een eigen wifi (`10.20.30.1`), los van je thuisnetwerk. Eén ESP kan hem uitlezen door tussen de twee netwerken te wisselen, maar dan zijn de waarden maar een deel van de tijd vers en bereikbaar. Om de data continu en bijna realtime beschikbaar te houden gebruikt dit project twee ESP's met een draad ertussen:

```
Xtend-wifi -> LEZER (ESP A) --draad--> GATEWAY (ESP B) -> thuiswifi -> Homey-app
```

| Map | Wat |
|---|---|
| [`esp/`](esp) | Arduino-sketches voor de lezer en de gateway (ESP8266 of ESP32), bedrading, updaten via wifi |
| [`homey-app/`](homey-app) | De Homey Pro-app die de gateway uitleest |

Begin met de ESP's ([`esp/README.nl.md`](esp/README.nl.md)) en installeer daarna de Homey-app ([`homey-app/README.md`](homey-app/README.md), Engels).

## Onderzoek

Hoe de API van de Xtend werkt, welke codes wat betekenen en waar we tegenaan liepen (pollinterval, wifi dat na 30 minuten dichtgaat, niet-aangesloten sensoren, OpenTherm) staat in [`docs/xtend-research.md`](docs/xtend-research.md) (Engels).

## Status

- Lezer ESP8266 + gateway ESP32 (WROOM-32): **getest en draait thuis**, inclusief het updaten van de lezer via wifi met de gateway.
- Andere chipcombinaties zijn geschreven maar **niet getest**. Meldingen zijn welkom.
- Homey-app: draait op een Homey Pro (Homey 12 of nieuwer). Nog niet in de Homey App Store.

## Geïnspireerd door

Zie de lijst in [`README.md`](README.md#inspired-by). Dit project bevat geen code uit die projecten. Gebouwd met hulp van Claude (Anthropic).

## Licentie

MIT, zie [`LICENSE`](LICENSE).

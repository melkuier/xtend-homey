# Changelog

## 1.1.0

- **Niet-aangesloten sensoren:** stuurt de ESP `"status": "niet aangesloten"`
  mee, dan toont Homey voor die tegel geen waarde (in plaats van 32,7 \u00b0C).
- **Energie:** `measure_power` (W, uit het elektrisch vermogen) en `meter_power`
  (kWh, som van de elektriciteitstotalen CV, warmwater en koelen). De teller
  loopt nooit terug.
- Extra velden: aanvoer systeem, gevraagde temperatuur, compressorfrequentie,
  energietotalen. Bestaande apparaten krijgen de nieuwe capabilities vanzelf.

## 1.2.0

- "Brander aan" volgt het vlamsignaal (`b2bc`), net als de Xtend-pagina zelf.
- `Uren onder spanning` (`71a7`) heette eerder ten onrechte "Branduren totaal".
  Echte ketelcijfers staan nu apart: draaiuren CV en warmwater, starts, vlamverlies.
- Thermisch vermogen apart voor warmtepomp en ketel, COP van gisteren,
  compressorstarts, draaiuren en ontdooicycli van de warmtepomp.
- De Energie-teller (`meter_power`) gebruikt het totaal "Used" van de Xtend (`63b3`).

## 1.2.1 - OpenTherm-status van de ketel

De Xtend praat via OpenTherm met de gasketel. Twee ruwe statusbytes zijn zichtbaar:

| Veld in Homey | Code | Betekenis |
|---|---|---|
| Ketel OpenTherm-status (ruwe waarde) | `843a` | Statusbyte die de ketel meldt |
| OpenTherm-status Xtend naar ketel (ruwe waarde) | `8457` | Statusbyte die de Xtend naar de ketel stuurt |
| Vlam aan volgens OpenTherm (bit 3) | uit `843a` | Waarde met bit 8 gezet |
| CV-modus actief volgens OpenTherm (bit 1) | uit `843a` | Waarde met bit 2 gezet |
| Warmwatermodus actief volgens OpenTherm (bit 2) | uit `843a` | Waarde met bit 4 gezet |

De bitindeling volgt de OpenTherm-standaard. Dat de Xtend de byte ongewijzigd
doorgeeft is niet bevestigd: vergelijk de uitgesplitste velden met "Brander aan"
(vlamsignaal `b2bc`) en met de Xtend-pagina op een moment dat de ketel echt brandt.

## 1.4.0

- **Aan/uit per waarde:** in de apparaatinstellingen staat per waarde een vinkje, in groepen.
  Uitgevinkt = de tegel verdwijnt en er wordt niets meer bijgewerkt of in Insights gelogd.
  Opnieuw aanvinken zet de tegel terug. Dit bepaalt alleen wat Homey bijhoudt; de ESP blijft
  alle waarden ophalen.
- Nieuw warmtepomplogo voor de app en het apparaat.
- De OpenTherm-tegels (vlam, CV-modus, warmwatermodus) worden nu ook op bestaande apparaten toegevoegd.

## 1.5.0

- **Statusteksten** voor "Apparaatstatus" (7e51) en "Bedrijfsmodus" (6578), overgenomen uit de web-app van de Xtend.
- **Brander aan, Warmtevraag warmtepomp, Warmtevraag ketel, Ontdooien actief** komen uit de statusvlaggen (f9f2),
  net als op de Xtend-pagina zelf (bit 7, 8, 9 en 14). Nieuw: Stille modus, CV-pomp aan, Compressor draait,
  Ketel verbonden via OpenTherm, Koelen draaiuren en -starts.
- De elektriciteitsteller voor Homey Energy is weer verwarmen + warmwater + koelen.
- Verwijderd: koudwater, warmwater, mengklep, rookgas, ketelretour, gevraagd ketel, kamertemperatuur en kamersetpoint
  (bij jou niet beschikbaar), en de ruwe brander-I/O. Op bestaande apparaten verdwijnen die tegels vanzelf.

## 1.5.1

- Verwijderd: de ontstekingstellers, het gasverbruik warmwater en de drie MSB-velden. Die gaven bij deze installatie
  altijd 0 en de Xtend-pagina gebruikt ze zelf niet. Op bestaande apparaten verdwijnen ze vanzelf.

## 1.6.0

- "Ontdooien actief" (sessies, met begin en einde) en "Ontdooicycli" (teller) worden nu in Insights gelogd.
  Bij een bestaand apparaat verschijnen ze pas na het uit- en weer aanvinken van die twee waarden in de apparaatinstellingen.
- Nieuwe afbeeldingen voor de app en het apparaat, en een donkerdere merkkleur.

## 1.6.1

- De afbeeldingen hebben een nieuwe bestandsnaam gekregen (xtend-app-*.png en xtend-device-*.png), omdat Homey
  de oude afbeeldingen uit het geheugen bleef tonen.

## 1.6.2

- "Ontdooicycli" (teller) staat niet meer in Insights; "Ontdooien actief" (status, met begin en einde) blijft gelogd.

## 1.6.3

- Merkkleur is nu Intergas-rood (#CE230E).

## 1.6.4

- "Ontdooien actief" wordt bij de eerste start eenmalig opnieuw aangemaakt, zodat het Insights-log vanzelf verschijnt (geen uit- en aanvinken nodig).

## 1.6.5

- Apparaatinstelling "Test: ontdooien 2 minuten als actief tonen", om het Insights-log te controleren.

## 1.6.6

- "Aanvoer systeem" is verwijderd (was gelijk aan aanvoer warmtepomp).
- "Actuele COP" wordt nu in Insights gelogd.
- Nieuw: "Ontdooien (grafiek)", een verborgen 0/1-waarde die in Insights een grafiek geeft van de ontdooisessies.
- Het veld "geen data" van de gateway maakt de tegels leeg als de lezer niets meer stuurt.
- De testoptie voor ontdooien is weggehaald.

'use strict';

const Homey = require('homey');

// Waarde die de ESP in "status" meestuurt als een sensor niet is aangesloten.
const MISSING = 'niet aangesloten';
const NO_DATA = 'geen data';  // gateway ontvangt al even niets meer van de lezer

// Betekenis van "Device status" (7e51), overgenomen uit de web-app van de Xtend (firmware V1.30).
const DEVICE_STATUS_TEXT = {
  0: 'OpenTherm', 15: 'Ketel (extern)', 24: 'Vorstbeveiliging', 37: 'CV (RF-thermostaat)', 51: 'Warmwater (intern)',
  85: 'Sensortest', 86: 'Inbedrijfstelling', 87: 'Carterverwarming', 88: 'Inbedrijfstelling (wacht)',
  102: 'Verwarmen (CV)', 103: 'CV wacht', 104: 'Ontdooien', 105: 'Ontdooien (wacht)',
  106: 'Ontdooien (wacht op doorstroming)', 107: 'CV geblokkeerd door SG Ready', 108: 'CV wacht (warmwater actief)',
  109: 'Betonvloer drogen', 117: 'Koelen start', 118: 'Koelen', 119: 'Koelen wacht',
  120: 'Koelen geblokkeerd door SG Ready', 121: 'Koelen wacht op doorstroming', 122: 'Koelen: condensbeveiliging',
  123: 'Koelen: vloerthermostaat uit', 126: 'Stand-by', 127: 'Uitgeschakeld', 153: 'Ketel nadraai',
  170: 'Service', 171: 'Zelftest warmtepomp', 189: 'Koelen nadraai', 204: 'Warmwater', 205: 'Warmwater (HReco)',
  206: 'Warmwater (legionellapreventie)', 207: 'Warmwater wacht', 230: 'CV start', 231: 'CV nadraai',
  240: 'Ketel (intern)', 255: 'Opwarmen',
};

// Betekenis van "Operational mode" (6578).
const WORKING_MODE_TEXT = { 0: 'Koelen', 1: 'Verwarmen', 2: 'Ontdooien', 3: 'Leegpompen', 255: 'Onbepaald' };

// Maps a key from the ESP32 JSON to a Homey capability.
// kind: 'number' | 'string' | 'boolean'
//  - 'number' uses .value (optioneel vermenigvuldigd met .scale, bv. kW -> W)
//  - 'string' uses .status_text if present, otherwise .value
//  - 'boolean' is true when .value is not 0
// Is een sensor "niet aangesloten", dan wordt de capability leeggemaakt (null).
const FIELD_MAP = [
  { key: 'aanvoer_wp', capability: 'measure_temperature.aanvoer_wp', kind: 'number' },
  { key: 'retour_wp', capability: 'measure_temperature.retour_wp', kind: 'number' },
  { key: 'buitentemperatuur', capability: 'measure_temperature.buiten', kind: 'number' },
  { key: 'gevraagde_temperatuur', capability: 'measure_temperature.gevraagd_systeem', kind: 'number' },
  { key: 'temp_aanvoer_ketel', capability: 'measure_temperature.aanvoer_ketel', kind: 'number' },

  { key: 'systeemdebit', capability: 'xtend_debiet', kind: 'number' },
  { key: 'waterdruk', capability: 'xtend_waterdruk', kind: 'number' },
  { key: 'compressorfrequentie', capability: 'xtend_compressorfrequentie', kind: 'number' },
  { key: 'actueel_thermisch_vermogen', capability: 'xtend_thermisch_vermogen', kind: 'number' },
  { key: 'actuele_cop', capability: 'xtend_actuele_cop', kind: 'number' },
  { key: 'thermisch_vermogen_wp', capability: 'xtend_thermisch_vermogen_wp', kind: 'number' },
  { key: 'thermisch_vermogen_ketel', capability: 'xtend_thermisch_vermogen_ketel', kind: 'number' },
  { key: 'cop_verwarmen_gisteren', capability: 'xtend_cop_gisteren', kind: 'number' },

  // Energie: de ESP geeft kW, Homey Energy wil W.
  { key: 'actueel_elektrisch_vermogen', capability: 'measure_power', kind: 'number', scale: 1000 },

  { key: 'modulatie_ketel', capability: 'xtend_modulatie_ketel', kind: 'number' },
  { key: 'ketel_opentherm_status', capability: 'xtend_ot_ketel_status', kind: 'number' },
  { key: 'thermostaat_opentherm_status', capability: 'xtend_ot_thermostaat_status', kind: 'number' },
  { key: 'statusvlaggen', capability: 'xtend_statusvlaggen', kind: 'number' },
  { key: 'bedrijfsmodus', capability: 'xtend_bedrijfsmodus', kind: 'string', texts: WORKING_MODE_TEXT },
  { key: 'brander_status', capability: 'xtend_brander_status', kind: 'number' },
  { key: 'warmtevraag_status', capability: 'xtend_warmtevraag_status', kind: 'string', texts: DEVICE_STATUS_TEXT },
  { key: 'interne_fout', capability: 'xtend_interne_fout', kind: 'boolean' },

  { key: 'storingscode', capability: 'xtend_storingscode', kind: 'string' },
  { key: 'vergrendelcode', capability: 'xtend_vergrendelcode', kind: 'string' },
  { key: 'notificatiecode', capability: 'xtend_notificatiecode', kind: 'string' },
  { key: 'systeemstatus', capability: 'xtend_systeemstatus', kind: 'string' },
  { key: 'wp_modus', capability: 'xtend_wp_modus', kind: 'string' },
  { key: 'status_rf_thermostaat', capability: 'xtend_status_rf_thermostaat', kind: 'number' },
  { key: 'ionisatiestroom', capability: 'xtend_ionisatiestroom', kind: 'number' },

  { key: 'uren_onder_spanning', capability: 'xtend_branduren_totaal', kind: 'number' },

  { key: 'thermische_energie_cv', capability: 'xtend_thermische_energie_cv', kind: 'number' },

  { key: 'ketel_uren_cv', capability: 'xtend_ketel_uren_cv', kind: 'number' },
  { key: 'ketel_uren_warmwater', capability: 'xtend_ketel_uren_warmwater', kind: 'number' },
  { key: 'ketel_starts', capability: 'xtend_ketel_starts', kind: 'number' },
  { key: 'ketel_vlamverlies', capability: 'xtend_ketel_vlamverlies', kind: 'number' },
  { key: 'compressor_starts', capability: 'xtend_wp_compressor_starts', kind: 'number' },
  { key: 'compressor_draaiuren', capability: 'xtend_wp_draaiuren', kind: 'number' },
  { key: 'ontdooicycli', capability: 'xtend_wp_ontdooicycli', kind: 'number' },
  { key: 'koelen_draaiuren', capability: 'xtend_koelen_draaiuren', kind: 'number' },
  { key: 'koelen_starts', capability: 'xtend_koelen_starts', kind: 'number' },

  { key: 'stroom_cv_totaal', capability: 'xtend_stroom_cv_totaal', kind: 'number' },
  { key: 'stroom_warmwater_totaal', capability: 'xtend_stroom_warmwater_totaal', kind: 'number' },
  { key: 'stroom_koelen_totaal', capability: 'xtend_stroom_koelen_totaal', kind: 'number' },
  { key: 'thermisch_warmwater_totaal', capability: 'xtend_thermisch_warmwater_totaal', kind: 'number' },
  { key: 'thermisch_ketel_totaal', capability: 'xtend_thermisch_ketel_totaal', kind: 'number' },
  { key: 'thermisch_koelen_totaal', capability: 'xtend_thermisch_koelen_totaal', kind: 'number' },

  { key: 'stroom_koelen_gisteren', capability: 'xtend_stroom_koelen_gisteren', kind: 'number' },
  { key: 'stroom_warmwater_gisteren', capability: 'xtend_stroom_warmwater_gisteren', kind: 'number' },
  { key: 'stroom_cv_gisteren', capability: 'xtend_stroom_cv_gisteren', kind: 'number' },
  { key: 'thermisch_ketel_gisteren', capability: 'xtend_thermisch_ketel_gisteren', kind: 'number' },
  { key: 'thermisch_koelen_gisteren', capability: 'xtend_thermisch_koelen_gisteren', kind: 'number' },
  { key: 'thermisch_warmwater_gisteren', capability: 'xtend_thermisch_warmwater_gisteren', kind: 'number' },
  { key: 'thermisch_cv_gisteren', capability: 'xtend_thermisch_cv_gisteren', kind: 'number' },
];

// Totaal elektriciteitsverbruik van de warmtepomp = verwarmen + warmwater + koelen.
// Dat wordt meter_power voor Homey Energy.
const ELECTRIC_TOTAL_KEYS = ['stroom_cv_totaal', 'stroom_warmwater_totaal', 'stroom_koelen_totaal'];

// Capabilities uit een eerdere versie die niet meer worden gebruikt.
const REMOVED_CAPABILITIES = [
  'measure_temperature.aanvoer_systeem',
  'measure_temperature.gevraagd_aanvoer_wp',
  'measure_temperature.koudwater',
  'measure_temperature.warmwater',
  'measure_temperature.mengklep',
  'measure_temperature.rookgas',
  'measure_temperature.retour_ketel',
  'measure_temperature.gevraagd_ketel',
  'measure_temperature.kamer',
  'measure_temperature.kamer_setpoint',
  'xtend_brander_io',
  'xtend_gasverbruik_warmwater',
  'xtend_mislukte_ontstekingen',
  'xtend_succesvolle_ontstekingen',
  'xtend_ontstekingen_warmwater',
  'xtend_ignition_success_dhw_msb',
  'xtend_ignition_successful_msb',
  'xtend_poweron_hours',
];

// Extra capabilities die op al bestaande apparaten worden bijgezet (en hun titels).
// Afgeleide capabilities (niet direct een veld uit de ESP-JSON).
// Bits uit de statusvlaggen van de Xtend, met dezelfde betekenis als in de web-app van de Xtend:
//  f9f2: bit 7 warmtevraag warmtepomp, bit 8 warmtevraag ketel, bit 9 ketel actief ("Flame"), bit 14 ontdooien
//  77c3: bit 3 stille modus          77d2: bit 1 CV-pomp, bit 5 compressor draait, bit 7 ketel via OpenTherm verbonden
const BIT_FIELDS = [
  { source: 'statusvlaggen', bit: 7, capability: 'xtend_warmtevraag_aan' },
  { source: 'statusvlaggen', bit: 8, capability: 'xtend_ketel_warmtevraag' },
  { source: 'statusvlaggen', bit: 9, capability: 'xtend_brander_aan' },
  { source: 'statusvlaggen', bit: 14, capability: 'xtend_ontdooien' },
  { source: 'vlaggen_modus', bit: 3, capability: 'xtend_stille_modus' },
  { source: 'systeem_io', bit: 1, capability: 'xtend_cv_pomp_aan' },
  { source: 'systeem_io', bit: 5, capability: 'xtend_compressor_aan' },
  { source: 'systeem_io', bit: 7, capability: 'xtend_ot_ketel_verbonden' },
];

const DERIVED_CAPABILITIES = [
  'xtend_storing_actief', 'meter_power', 'xtend_ontdooien_grafiek',
  'xtend_ot_vlam', 'xtend_ot_ch_actief', 'xtend_ot_warmwater_actief',
  ...BIT_FIELDS.map((b) => b.capability),
];

// Naam van de aan/uit-instelling per capability (punt wordt dubbele underscore).
const settingId = (cap) => `show_${cap.replace(/\./g, '__')}`;
const TITLES = {
  'measure_temperature.gevraagd_systeem': { en: 'Requested temperature', nl: 'Gevraagde temperatuur' },
  'measure_temperature.kamer': { en: 'Room temperature', nl: 'Kamertemperatuur' },
  'measure_temperature.kamer_setpoint': { en: 'Room setpoint', nl: 'Kamer setpoint' },
  measure_power: { en: 'Heat pump electrical power', nl: 'Elektrisch vermogen warmtepomp' },
  meter_power: { en: 'Heat pump electricity total', nl: 'Elektriciteit warmtepomp totaal' },
};

const FETCH_TIMEOUT_MS = 10000;

class XtendDevice extends Homey.Device {

  async onInit() {
    this.log('Xtend device initialized');
    this._syncing = false;
    await this._removeObsolete();
    await this._syncCapabilities();
    await this._refreshInsightsOnce();
    await this._applyTitles();
    await this._applyPollInterval();
    // Kick off an immediate poll on startup / app restart.
    this._poll();
  }

  async onSettings({ oldSettings, newSettings, changedKeys }) {
    if (changedKeys.includes('poll_interval')) {
      await this._applyPollInterval(newSettings.poll_interval);
    }
    if (changedKeys.some((k) => k.startsWith('show_'))) {
      await this._syncCapabilities(newSettings);
      await this._applyTitles();
    }
    if (changedKeys.includes('ip_address')) {
      // Try immediately with the new address.
      this.homey.setTimeout(() => this._poll(), 500);
    }
  }

  async onDeleted() {
    if (this._pollTimer) {
      this.homey.clearInterval(this._pollTimer);
    }
  }

  // Eenmalig: haal "Ontdooien actief" opnieuw door Homey, zodat het Insights-log (status met begin en einde)
  // ook op bestaande apparaten wordt aangemaakt.
  async _refreshInsightsOnce() {
    const cap = 'xtend_ontdooien';
    const flag = 'insights_ontdooien_v1';
    try {
      if (this.getStoreValue(flag)) return;
      if (this.hasCapability(cap)) {
        await this.removeCapability(cap);
        await this.addCapability(cap);
        this.log('Ontdooien actief opnieuw toegevoegd voor Insights');
      }
      await this.setStoreValue(flag, true);
    } catch (err) {
      this.error('Insights verversen mislukt:', err.message);
    }
  }

  async _removeObsolete() {
    for (const cap of REMOVED_CAPABILITIES) {
      if (this.hasCapability(cap)) {
        await this.removeCapability(cap).catch((err) => this.error(`Kon ${cap} niet verwijderen:`, err.message));
      }
    }
  }

  // Zet capabilities aan of uit volgens de vinkjes in de apparaatinstellingen.
  // Uitgevinkt = tegel verdwijnt en er wordt niets meer bijgewerkt of in Insights gelogd.
  // Ook capabilities die in een nieuwere versie van de app zijn bijgekomen worden zo toegevoegd.
  async _syncCapabilities(settings) {
    const current = settings || this.getSettings();
    const all = [...new Set(FIELD_MAP.map((f) => f.capability).concat(DERIVED_CAPABILITIES))];
    for (const cap of all) {
      const wanted = current[settingId(cap)] !== false;
      const has = this.hasCapability(cap);
      try {
        if (wanted && !has) {
          await this.addCapability(cap);
          this.log(`Capability toegevoegd: ${cap}`);
        } else if (!wanted && has) {
          await this.removeCapability(cap);
          this.log(`Capability verwijderd: ${cap}`);
        }
      } catch (err) {
        this.error(`Kon capability ${cap} niet bijwerken:`, err.message);
      }
    }
  }

  async _applyTitles() {
    for (const [cap, title] of Object.entries(TITLES)) {
      if (this.hasCapability(cap)) {
        await this.setCapabilityOptions(cap, { title }).catch((err) => {
          this.error(`Kon titel van ${cap} niet zetten:`, err.message);
        });
      }
    }
  }

  async _applyPollInterval(seconds) {
    const interval = seconds || this.getSetting('poll_interval') || 60;
    if (this._pollTimer) {
      this.homey.clearInterval(this._pollTimer);
    }
    this._pollTimer = this.homey.setInterval(() => this._poll(), interval * 1000);
  }

  async _poll() {
    if (this._syncing) return; // avoid overlapping requests
    this._syncing = true;

    const ip = this.getSetting('ip_address');
    if (!ip) {
      this._syncing = false;
      await this.setUnavailable(
        this.homey.__({ en: 'No IP address configured yet', nl: 'Nog geen IP-adres ingesteld' })
      ).catch(() => null);
      return;
    }

    const url = `http://${ip}/`;

    try {
      const controller = new AbortController();
      const timeout = this.homey.setTimeout(() => controller.abort(), FETCH_TIMEOUT_MS);

      const response = await fetch(url, { signal: controller.signal });
      this.homey.clearTimeout(timeout);

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }

      const data = await response.json();
      await this._applyData(data);
      await this.setAvailable().catch(() => null);
    } catch (err) {
      this.error('Poll failed:', err.message);
      await this.setUnavailable(
        this.homey.__({ en: 'Cannot reach the ESP32 bridge', nl: 'Kan de ESP32-bridge niet bereiken' })
      ).catch(() => null);
    } finally {
      this._syncing = false;
    }
  }

  _set(capability, value) {
    if (!this.hasCapability(capability)) return;
    this.setCapabilityValue(capability, value).catch((err) => {
      this.error(`Could not set ${capability}:`, err.message);
    });
  }

  async _applyData(data) {
    const isMissing = (entry) => entry && (entry.status === MISSING || entry.status === NO_DATA);

    for (const field of FIELD_MAP) {
      const entry = data[field.key];
      if (entry === undefined || entry === null) continue;

      // Sensor niet aangesloten: geen nepwaarde tonen, maar de tegel leegmaken.
      if (isMissing(entry)) {
        this._set(field.capability, null);
        continue;
      }

      let value;
      if (field.kind === 'boolean') {
        value = Number(entry.value) !== 0;
      } else if (field.kind === 'string') {
        if (entry.status_text !== undefined) {
          value = String(entry.status_text);
        } else if (field.texts) {
          const code = Math.round(Number(entry.value));
          value = field.texts[code] !== undefined ? field.texts[code] : String(entry.value);
        } else {
          value = String(entry.value);
        }
      } else {
        value = Number(entry.value);
        if (Number.isNaN(value)) continue;
        if (field.scale) value = Math.round(value * field.scale * 10) / 10;
      }

      this._set(field.capability, value);
    }

    // Derived: is there an active fault?
    if (data.storingscode && !isMissing(data.storingscode)) {
      this._set('xtend_storing_actief', Number(data.storingscode.value) !== 0);
    }

    // Derived: bits uit de statusvlaggen van de Xtend.
    for (const b of BIT_FIELDS) {
      const src = data[b.source];
      if (!src) continue;
      if (isMissing(src)) {
        this._set(b.capability, null);
        if (b.capability === 'xtend_ontdooien') this._set('xtend_ontdooien_grafiek', null);
        continue;
      }
      const raw = Math.round(Number(src.value));
      if (Number.isNaN(raw)) continue;
      const on = ((raw >> b.bit) & 1) === 1;
      this._set(b.capability, on);
      if (b.capability === 'xtend_ontdooien') this._set('xtend_ontdooien_grafiek', on ? 1 : 0);
    }

    // Derived: OpenTherm-statusbyte van de ketel, standaardlayout:
    // bit 0 = storing, bit 1 = CH-modus actief, bit 2 = warmwatermodus actief, bit 3 = vlam aan.
    // Let op: dat de Xtend de byte ongewijzigd doorgeeft is een aanname; vergelijk met de ruwe waarde.
    const ot = data.ketel_opentherm_status;
    if (ot && isMissing(ot)) {
      this._set('xtend_ot_vlam', null);
      this._set('xtend_ot_ch_actief', null);
      this._set('xtend_ot_warmwater_actief', null);
    } else if (ot && !Number.isNaN(Number(ot.value))) {
      const st = Math.round(Number(ot.value));
      this._set('xtend_ot_vlam', (st & 8) !== 0);
      this._set('xtend_ot_ch_actief', (st & 2) !== 0);
      this._set('xtend_ot_warmwater_actief', (st & 4) !== 0);
    }

    // Derived: totale elektriciteit van de warmtepomp voor Homey Energy (kWh, oplopend).
    let total = 0;
    let seen = 0;
    for (const key of ELECTRIC_TOTAL_KEYS) {
      const entry = data[key];
      if (entry && !isMissing(entry) && !Number.isNaN(Number(entry.value))) {
        total += Number(entry.value);
        seen += 1;
      }
    }
    if (seen > 0 && this.hasCapability('meter_power')) {
      const current = this.getCapabilityValue('meter_power');
      // Een teller mag niet teruglopen (bv. nullen na een herstart van de ESP32).
      if (current === null || current === undefined || total >= current) {
        this._set('meter_power', total);
      }
    }
  }

}

module.exports = XtendDevice;

#pragma once
// Gedeelde veldtabel voor BEIDE ESP's (identiek bestand in beide sketchmappen).
// ESP A haalt precies deze velden op (in groepen van BATCH_SIZE) en rekent om met factor.
// ESP B gebruikt key + name + unit om dezelfde JSON te serveren als voorheen.
// Een veld toevoegen of weghalen kan dus op één plek: hier.
#include <Arduino.h>
#include <string.h>

struct FieldDef {
  const char* key;
  const char* name;
  const char* unit;
  float factor;
  float lastValue;
  bool missing;  // true = sensor niet aangesloten (ruwe waarde 32767)
};

static FieldDef FIELDS[] = {
  {"62e7", "aanvoer_wp", "C", 0.01f, 0.0f, false},
  {"6280", "retour_wp", "C", 0.01f, 0.0f, false},
  {"62d1", "buitentemperatuur", "C", 0.01f, 0.0f, false},
  {"629c", "systeemdebit", "l/min", 0.01f, 0.0f, false},
  {"7ed3", "waterdruk", "bar", 0.01f, 0.0f, false},
  {"777d", "wp_modus", "", 1.0f, 0.0f, false},
  {"77dd", "systeemstatus", "", 1.0f, 0.0f, false},
  {"4133", "storingscode", "", 1.0f, 0.0f, false},
  {"71a7", "uren_onder_spanning", "h", 1.0f, 0.0f, false},
  {"63f0", "thermische_energie_cv", "kWh", 1.0f, 0.0f, false},
  {"7e7a", "brander_status", "", 1.0f, 0.0f, false},
  {"7e82", "ionisatiestroom", "uA", 0.01f, 0.0f, false},
  {"7e51", "warmtevraag_status", "", 1.0f, 0.0f, false},
  {"7e67", "interne_fout", "", 1.0f, 0.0f, false},
  {"7e2c", "vergrendelcode", "", 1.0f, 0.0f, false},
  {"7e14", "status_rf_thermostaat", "", 1.0f, 0.0f, false},
  {"7ee6", "temp_aanvoer_ketel", "C", 0.01f, 0.0f, false},
  {"5077", "actueel_thermisch_vermogen", "kW", 0.001f, 0.0f, false},
  {"503f", "stroom_koelen_gisteren", "kWh", 0.1f, 0.0f, false},
  {"50ff", "stroom_warmwater_gisteren", "kWh", 0.1f, 0.0f, false},
  {"5099", "stroom_cv_gisteren", "kWh", 0.1f, 0.0f, false},
  {"506a", "thermisch_ketel_gisteren", "kWh", 0.1f, 0.0f, false},
  {"5008", "thermisch_koelen_gisteren", "kWh", 0.1f, 0.0f, false},
  {"5098", "thermisch_warmwater_gisteren", "kWh", 0.1f, 0.0f, false},
  {"50ae", "thermisch_cv_gisteren", "kWh", 0.1f, 0.0f, false},
  {"50f2", "actueel_elektrisch_vermogen", "kW", 0.001f, 0.0f, false},
  {"5041", "actuele_cop", "", 0.1f, 0.0f, false},
  {"65a7", "compressorfrequentie", "Hz", 0.01f, 0.0f, false},
  {"621d", "aanvoer_systeem", "C", 0.01f, 0.0f, false},
  {"62ed", "gevraagde_temperatuur", "C", 0.01f, 0.0f, false},
  {"63b3", "stroom_cv_totaal", "kWh", 1.0f, 0.0f, false},
  {"6358", "stroom_warmwater_totaal", "kWh", 1.0f, 0.0f, false},
  {"63a7", "stroom_koelen_totaal", "kWh", 1.0f, 0.0f, false},
  {"6339", "thermisch_warmwater_totaal", "kWh", 1.0f, 0.0f, false},
  {"63df", "thermisch_ketel_totaal", "kWh", 1.0f, 0.0f, false},
  {"63e4", "thermisch_koelen_totaal", "kWh", 1.0f, 0.0f, false},
  {"7940", "notificatiecode", "", 1.0f, 0.0f, false},
  {"f9f2", "statusvlaggen", "", 1.0f, 0.0f, false},
  {"84d1", "modulatie_ketel", "%", 0.01f, 0.0f, false},
  {"6578", "bedrijfsmodus", "", 1.0f, 0.0f, false},
  {"503e", "thermisch_vermogen_wp", "kW", 0.001f, 0.0f, false},
  {"5088", "thermisch_vermogen_ketel", "kW", 0.001f, 0.0f, false},
  {"50f7", "cop_verwarmen_gisteren", "", 0.1f, 0.0f, false},
  {"8ef9", "ketel_uren_cv", "h", 1.0f, 0.0f, false},
  {"8e37", "ketel_uren_warmwater", "h", 1.0f, 0.0f, false},
  {"8e00", "ketel_starts", "", 1.0f, 0.0f, false},
  {"8e18", "ketel_vlamverlies", "", 1.0f, 0.0f, false},
  {"6a8e", "compressor_starts", "", 1.0f, 0.0f, false},
  {"6ac5", "compressor_draaiuren", "h", 1.0f, 0.0f, false},
  {"6a53", "ontdooicycli", "", 1.0f, 0.0f, false},
  {"843a", "ketel_opentherm_status", "", 1.0f, 0.0f, false},
  {"8457", "thermostaat_opentherm_status", "", 1.0f, 0.0f, false},
  {"77d2", "systeem_io", "", 1.0f, 0.0f, false},
  {"77c3", "vlaggen_modus", "", 1.0f, 0.0f, false},
  {"6a78", "koelen_draaiuren", "h", 1.0f, 0.0f, false},
  {"6a28", "koelen_starts", "", 1.0f, 0.0f, false}
};
static const size_t FIELD_COUNT = sizeof(FIELDS) / sizeof(FIELDS[0]);

static FieldDef* findField(const char* key) {
  for (size_t i = 0; i < FIELD_COUNT; i++) {
    if (strcmp(FIELDS[i].key, key) == 0) return &FIELDS[i];
  }
  return nullptr;
}

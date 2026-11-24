// Intergas Xtend -> Homey: LEZER (ESP A).
// Draait op een ESP8266 (getest) of ESP32 (WROOM-32).
//
// Blijft permanent verbonden met het wifi van de Xtend, haalt de waarden op en stuurt per batch
// één regel over een draad (UART) naar de gateway (ESP B), die op je thuisnetwerk zit.
//
// Zendpen (TX) naar de gateway:
//   ESP8266:  GPIO2  (D4 op NodeMCU)   via Serial1
//   ESP32:    GPIO17 (TX2)             via Serial1
// Ontvangstpen (RX) voor het commando "update" van de gateway (tweede draad):
//   ESP8266:  GPIO14 (D5)              via SoftwareSerial
//   ESP32:    GPIO16 (RX2)             via Serial1
// Verbind ook GND met GND. Zie README voor de bedrading per combinatie.
//
// Frameformaat (één regel):  $<json>*<xor-checksum in hex>\n
// Voorbeeld:                 ${"62e7":45.12,"6280":38.4}*5A

#define SOFTWARE_VERSION "2.1.1"

// 1 = luister op de commandodraad (updaten van de lezer via de gateway), 0 = uit.
// Zet op 0 als de lezer vreemd doet; dan werkt hij als versie 2.0 en negeert hij de draad.
#define ENABLE_UPDATE_COMMAND 1

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <SoftwareSerial.h>
  #include <ArduinoOTA.h>
  #define LINK_TX_PIN 2    // D4
  #define CMD_RX_PIN 14    // D5
  SoftwareSerial cmdSerial(CMD_RX_PIN, -1);
#elif defined(ESP32)
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <ArduinoOTA.h>
  #define LINK_TX_PIN 17   // TX2
  #define CMD_RX_PIN 16    // RX2
  #define cmdSerial Serial1  // zelfde UART als de zendlijn: TX = GPIO17, RX = GPIO16
#else
  #error "Alleen ESP8266 en ESP32 worden ondersteund"
#endif
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include "fields.h"
#include "secrets.h"   // kopieer secrets.h.example naar secrets.h

// ---- XTEND NETWERK ----
const char* XTEND_SSID_C     = XTEND_SSID;
const char* XTEND_PASSWORD_C = XTEND_PASSWORD;

// ---- UPDATEMODUS (via commando van de gateway) ----
const unsigned long UPDATE_WINDOW_MS = 600000UL;  // 10 minuten wachten op een update, dan herstart

// ---- LINK / POLLING ----
const unsigned long LINK_BAUD        = 19200;    // moet gelijk zijn aan ESP B
const unsigned long POLL_INTERVAL_MS = 50000UL;  // hele cyclus; verhoog als de Xtend moeite heeft

// De velden komen uit fields.h en worden in groepen opgevraagd. De Xtend-pagina zelf
// vraagt er tot 83 tegelijk, dus 24 per verzoek is ruim binnen wat hij aankan.
const size_t BATCH_SIZE = 24;
const size_t TOTAL_BATCHES = (FIELD_COUNT + BATCH_SIZE - 1) / BATCH_SIZE;

String batchUrl(size_t batch) {
  String url = "http://10.20.30.1/api/stats/values?fields=";
  size_t start = batch * BATCH_SIZE;
  for (size_t i = start; i < FIELD_COUNT && i < start + BATCH_SIZE; i++) {
    if (i > start) url += ',';
    url += FIELDS[i].key;
  }
  return url;
}

const unsigned long WATCHDOG_MS = 600000UL;  // 10 min geen enkele geslaagde batch: ESP herstarten

unsigned long lastPoll = 0;
unsigned long lastSuccessMs = 0;   // laatste moment dat minstens één batch lukte
unsigned long wifiDownSince = 0;   // sinds wanneer de Xtend-wifi weg is (0 = verbonden)
uint8_t failedRounds = 0;          // aantal rondes achter elkaar zonder één geslaagde batch

void connectXtend() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(XTEND_SSID_C, XTEND_PASSWORD_C);

  Serial.print("[Xtend] Verbinden");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000UL) {
    delay(250);
    Serial.print("~");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n[Xtend] Verbonden, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[Xtend] Nog niet verbonden, auto-reconnect blijft proberen.");
  }
}

void sendFrame(JsonDocument& doc) {
  String json;
  serializeJson(doc, json);
  uint8_t cs = 0;
  for (size_t i = 0; i < json.length(); i++) cs ^= (uint8_t)json[i];
  char tail[8];
  snprintf(tail, sizeof(tail), "*%02X\n", cs);
  Serial1.print('$');
  Serial1.print(json);
  Serial1.print(tail);
}

float convertValue(FieldDef* fd, JsonVariant v, bool& missing) {
  missing = false;
  if (v.isNull()) return 0.0f;
  if (v.is<const char*>()) return atof(v.as<const char*>());
  float val = v.as<long>() * fd->factor;
  if ((strcmp(fd->unit, "C") == 0 || strcmp(fd->unit, "%") == 0) && fabsf(val - 327.67f) < 0.01f) {
    missing = true;  // sensor niet aangesloten
    return 32.70f;   // vervangwaarde zoals in de oorspronkelijke code
  }
  return roundf(val * 100.0f) / 100.0f;
}

bool fetchBatch(size_t i) {
  WiFiClient client;
  HTTPClient http;
  http.setTimeout(5000);
  http.useHTTP10(true);

  String url = batchUrl(i);
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.print("  [Xtend] HTTP foutcode: ");
    Serial.println(code);
    http.end();
    return false;
  }
  String payload = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, payload)) return false;
  JsonObject stats = doc["stats"].as<JsonObject>();
  if (stats.isNull()) return false;

  JsonDocument out;
  for (auto kv : stats) {
    FieldDef* fd = findField(kv.key().c_str());
    if (!fd) continue;
    fd->lastValue = convertValue(fd, kv.value(), fd->missing);
    if (fd->missing) {
      out[fd->key] = nullptr;  // null in het frame = niet aangesloten
      Serial.printf("  -> %s: NIET AANGESLOTEN\n", fd->name);
    } else {
      out[fd->key] = fd->lastValue;
      Serial.printf("  -> %s: %.2f%s\n", fd->name, fd->lastValue, fd->unit);
    }
  }
  sendFrame(out);
  return true;
}

void pollAll() {
  Serial.println("\n--- START XTEND DATA OPHALEN ---");
  size_t ok = 0;
  for (size_t i = 0; i < TOTAL_BATCHES; i++) {
    if (fetchBatch(i)) ok++;
    delay(400);
    if (commandReceived()) updateMode();
  }
  Serial.printf("[Xtend] Ronde klaar: %u/%u batches gelukt, vrij geheugen %u bytes, wifi %d dBm\n",
                (unsigned)ok, (unsigned)TOTAL_BATCHES, (unsigned)ESP.getFreeHeap(), (int)WiFi.RSSI());
  if (ok > 0) { lastSuccessMs = millis(); failedRounds = 0; }
  else        { failedRounds++; }
}

void reconnectXtend() {
  Serial.println("[Xtend] Wifi opnieuw opzetten...");
  WiFi.disconnect();
  delay(300);
  WiFi.mode(WIFI_STA);
  WiFi.begin(XTEND_SSID_C, XTEND_PASSWORD_C);
}

void restartIfStuck() {
  if (millis() - lastSuccessMs > WATCHDOG_MS) {
    Serial.println("[Xtend] Al 10 minuten geen data ontvangen, ESP herstart.");
    delay(200);
    ESP.restart();
  }
}


// ---- Commando van de gateway: "$UPDATE*<checksum>" ----
String cmdLine;

bool checkCommand(const String& l) {
  int star = l.lastIndexOf('*');
  if (l.length() < 4 || l[0] != '$' || star < 2) return false;
  String body = l.substring(1, star);
  uint8_t cs = 0;
  for (size_t i = 0; i < body.length(); i++) cs ^= (uint8_t)body[i];
  uint8_t rx = (uint8_t)strtoul(l.substring(star + 1).c_str(), nullptr, 16);
  return cs == rx && body == "UPDATE";
}

bool commandReceived() {
#if !ENABLE_UPDATE_COMMAND
  return false;
#endif
  while (cmdSerial.available()) {
    char c = cmdSerial.read();
    if (c == '\n') {
      cmdLine.trim();
      bool ok = cmdLine.length() && checkCommand(cmdLine);
      cmdLine = "";
      if (ok) return true;
    } else if (c != '\r') {
      if (cmdLine.length() < 64) cmdLine += c;
      else cmdLine = "";
    }
  }
  return false;
}

void sendOtaFrame() {
  JsonDocument doc;
  doc["ota"] = 1;
  sendFrame(doc);
}

// Verlaat het Xtend-wifi, gaat naar je thuiswifi en wacht op een update via Arduino IDE (netwerkpoort).
// Na UPDATE_WINDOW_MS of na de update herstart de ESP en gaat hij terug naar de Xtend.
void updateMode() {
  Serial.println("[Update] Commando ontvangen: overschakelen naar thuiswifi");
  WiFi.disconnect();
  delay(300);
  WiFi.mode(WIFI_STA);
  WiFi.begin(HOME_SSID, HOME_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 30000UL) {
    delay(250);
    sendOtaFrame();  // laat de gateway weten dat de lezer in updatemodus staat
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Update] Thuiswifi niet gevonden, herstart.");
    delay(200);
    ESP.restart();
  }
  Serial.print("[Update] Verbonden, IP: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname(OTA_HOSTNAME_READER);
  ArduinoOTA.setPassword(OTA_PASSWORD_READER);
  ArduinoOTA.onEnd([]() { Serial.println("\n[Update] Klaar, herstart"); });
  ArduinoOTA.begin();

  unsigned long lastAck = 0;
  start = millis();
  while (millis() - start < UPDATE_WINDOW_MS) {
    ArduinoOTA.handle();
    if (millis() - lastAck > 10000UL) { sendOtaFrame(); lastAck = millis(); }
    delay(5);
  }
  Serial.println("[Update] Tijd om, herstart naar Xtend-wifi.");
  delay(200);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);   // USB-debug
#if defined(ESP32)
  Serial1.begin(LINK_BAUD, SERIAL_8N1, CMD_RX_PIN, LINK_TX_PIN);  // TX data naar gateway, RX commando
#else
  Serial1.begin(LINK_BAUD);  // ESP8266: Serial1 heeft alleen TX, op GPIO2 (D4)
#if ENABLE_UPDATE_COMMAND
  cmdSerial.begin(LINK_BAUD);  // commando's van de gateway op GPIO14 (D5)
  pinMode(CMD_RX_PIN, INPUT_PULLUP);  // geen zwevende pen als de draad ontbreekt (voorkomt ruis)
#endif
#endif
  delay(500);
  Serial.println();
  Serial.printf("[Lezer] Versie %s, update-commando %s\n", SOFTWARE_VERSION, ENABLE_UPDATE_COMMAND ? "aan" : "uit");
  connectXtend();
  lastPoll = millis() - POLL_INTERVAL_MS;  // meteen eerste poll
  lastSuccessMs = millis();
}

void loop() {
  if (commandReceived()) updateMode();
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiDownSince == 0) wifiDownSince = millis();
    if (millis() - wifiDownSince > 60000UL) {  // 1 minuut zonder wifi: opnieuw verbinden
      reconnectXtend();
      wifiDownSince = millis();
    }
    restartIfStuck();
    for (int k = 0; k < 10; k++) { if (commandReceived()) updateMode(); delay(100); }
    return;
  }
  wifiDownSince = 0;

  if (millis() - lastPoll >= POLL_INTERVAL_MS) {
    lastPoll = millis();
    pollAll();
    if (failedRounds >= 3) {  // drie rondes zonder resultaat: wifi opnieuw opzetten
      reconnectXtend();
      failedRounds = 0;
    }
  }
  restartIfStuck();
  delay(10);
}

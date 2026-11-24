// Intergas Xtend -> Homey: GATEWAY (ESP B).
// Draait op een ESP32 (WROOM-32, getest) of ESP8266 (NodeMCU).
//
// Blijft permanent op je thuiswifi, ontvangt de frames van de lezer (ESP A) over een draad
// en serveert dezelfde JSON als de Xtend-ESP, zodat de Homey-app ermee kan praten.
//
// Ontvangstpen (RX) van de lezer:
//   ESP32:    GPIO16 (RX2)                 via Serial2; USB (Serial) blijft vrij voor debug
//   ESP8266:  GPIO13 (D7)                  via Serial na Serial.swap(); USB-debug werkt dan niet,
//                                          meldingen komen op Serial1 (GPIO2). Gebruik /health.
// Zendpen (TX) voor het commando "update lezer" naar de lezer (tweede draad):
//   ESP32:    GPIO17 (TX2)
//   ESP8266:  GPIO15 (D8)
// Verbind ook GND met GND. Zie README voor de bedrading per combinatie.
// (WROVER-modules gebruiken GPIO16/17 voor PSRAM: kies dan andere pinnen.)

#define SOFTWARE_VERSION "2.1.0"

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  typedef ESP8266WebServer WebServerType;
  #define LinkSerial Serial       // na swap() op GPIO13/15
  #define DBG Serial1             // alleen TX (GPIO2)
  #define LINK_RX_PIN 13          // D7 (informatief)
  #define LINK_TX_PIN 15          // D8 (informatief)
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  typedef WebServer WebServerType;
  #define LinkSerial Serial2
  #define DBG Serial
  #define LINK_RX_PIN 16          // RX2
  #define LINK_TX_PIN 17          // TX2
#else
  #error "Alleen ESP8266 en ESP32 worden ondersteund"
#endif
#include <ArduinoOTA.h>   // flashen via wifi (na één keer flashen met de kabel)
#include <ArduinoJson.h>
#include "fields.h"
#include "secrets.h"      // kopieer secrets.h.example naar secrets.h

const char* HOME_SSID_C     = HOME_SSID;
const char* HOME_PASSWORD_C = HOME_PASSWORD;

const IPAddress ip(STATIC_IP);
const IPAddress gateway(STATIC_GATEWAY);
const IPAddress subnet(STATIC_SUBNET);
const IPAddress dns(STATIC_DNS);

// ---- LINK ----
const unsigned long LINK_BAUD = 19200;  // moet gelijk zijn aan ESP A

// Na zoveel seconden zonder frame van de lezer meldt de gateway "geen data" in plaats van oude waarden.
// De lezer vraagt elke ~50 s; 300 s laat ruimte voor een paar mislukte rondes.
const unsigned long STALE_AFTER_S = 300;

// Updatemodus van de lezer: commando herhalen tot de lezer antwoordt (ota-frame) of tot de tijd om is.
unsigned long updateRequestUntil = 0;   // 0 = niet actief
unsigned long lastCmdMs = 0;
unsigned long readerOtaMs = 0;          // laatste "ota"-frame van de lezer

WebServerType server(80);

String rxLine;
unsigned long lastFrameMs = 0;
uint32_t framesOk = 0, framesBad = 0;

bool dataIsStale() {
  return lastFrameMs == 0 || (millis() - lastFrameMs) / 1000 > STALE_AFTER_S;
}

// Zelfde JSON als de oorspronkelijke handleRoot; bij oude data komt er "status": "geen data" bij.
void handleRoot() {
  JsonDocument doc;
  bool stale = dataIsStale();
  for (size_t i = 0; i < FIELD_COUNT; i++) {
    JsonObject nested = doc[FIELDS[i].name].to<JsonObject>();
    nested["value"] = FIELDS[i].lastValue;
    nested["unit"] = String(FIELDS[i].unit);
    if (stale) nested["status"] = "geen data";
    else if (FIELDS[i].missing) nested["status"] = "niet aangesloten";
  }
  String response;
  serializeJsonPretty(doc, response);
  server.send(200, "application/json", response);
}

// Extra endpoint (verandert de JSON van "/" niet): zie of ESP A nog data stuurt
void handleHealth() {
  JsonDocument doc;
  doc["version"] = SOFTWARE_VERSION;
  doc["uptime_s"] = millis() / 1000;
  doc["frames_ok"] = framesOk;
  doc["frames_bad"] = framesBad;
  doc["data_stale"] = dataIsStale();
  doc["reader_update_mode"] = readerOtaMs && (millis() - readerOtaMs) < 60000UL;
  doc["last_frame_age_s"] = lastFrameMs ? (long)((millis() - lastFrameMs) / 1000) : -1;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleFrame(const String& l) {
  int star = l.lastIndexOf('*');
  if (l.length() < 5 || l[0] != '$' || star < 2) { framesBad++; return; }

  String json = l.substring(1, star);
  uint8_t cs = 0;
  for (size_t i = 0; i < json.length(); i++) cs ^= (uint8_t)json[i];
  uint8_t rx = (uint8_t)strtoul(l.substring(star + 1).c_str(), nullptr, 16);
  if (cs != rx) { framesBad++; return; }

  JsonDocument doc;
  if (deserializeJson(doc, json)) { framesBad++; return; }

  if (doc["ota"].is<int>()) {   // lezer meldt dat hij in updatemodus staat
    readerOtaMs = millis();
    updateRequestUntil = 0;
    framesOk++;
    return;
  }

  for (JsonPair kv : doc.as<JsonObject>()) {
    FieldDef* fd = findField(kv.key().c_str());
    if (!fd) continue;
    if (kv.value().isNull()) {
      fd->missing = true;
      fd->lastValue = 32.70f;  // zelfde vervangwaarde als voorheen
    } else {
      fd->missing = false;
      fd->lastValue = kv.value().as<float>();
    }
  }
  framesOk++;
  lastFrameMs = millis();
}

void readLink() {
  while (LinkSerial.available()) {
    char c = LinkSerial.read();
    if (c == '\n') {
      rxLine.trim();
      if (rxLine.length()) handleFrame(rxLine);
      rxLine = "";
    } else if (c != '\r') {
      if (rxLine.length() < 1024) rxLine += c;
      else rxLine = "";  // te lang = rommel, begin opnieuw
    }
  }
}

void sendUpdateCommand() {
  const char* body = "UPDATE";
  uint8_t cs = 0;
  for (const char* c = body; *c; c++) cs ^= (uint8_t)*c;
  char buf[24];
  snprintf(buf, sizeof(buf), "$%s*%02X\n", body, cs);
  LinkSerial.print(buf);
}

void handleUpdateReader() {
  if (server.method() == HTTP_POST) {
    updateRequestUntil = millis() + 60000UL;
    lastCmdMs = 0;  // meteen sturen
    server.send(200, "text/html",
      "<html><body style='font-family:sans-serif'><h3>Commando verstuurd</h3>"
      "<p>De lezer schakelt over naar je thuiswifi en wacht 10 minuten op een update "
      "(Arduino IDE, Poort: <b>xtend-reader</b>). Daarna herstart hij en gaat terug naar de Xtend.</p>"
      "<p>Tijdens die tijd komt er geen nieuwe data binnen.</p>"
      "<p><a href='/health'>/health</a></p></body></html>");
  } else {
    server.send(200, "text/html",
      "<html><body style='font-family:sans-serif'><h3>Lezer updaten</h3>"
      "<p>Druk op de knop en de lezer (ESP A) gaat 10 minuten naar je thuiswifi voor een update via wifi. "
      "Er komt dan geen data binnen.</p>"
      "<form method='POST' action='/update-reader'><button type='submit' style='font-size:1.2em;padding:.5em 1em'>"
      "Lezer in updatemodus zetten</button></form></body></html>");
  }
}

void serviceUpdateRequest() {
  if (!updateRequestUntil) return;
  if ((long)(millis() - updateRequestUntil) > 0) { updateRequestUntil = 0; return; }
  if (lastCmdMs == 0 || millis() - lastCmdMs > 5000UL) {
    sendUpdateCommand();
    lastCmdMs = millis();
  }
}

void setup() {
  DBG.begin(115200);
#if defined(ESP32)
  LinkSerial.begin(LINK_BAUD, SERIAL_8N1, LINK_RX_PIN, LINK_TX_PIN);
#else
  LinkSerial.setRxBufferSize(1024);  // moet voor begin()
  LinkSerial.begin(LINK_BAUD);
  LinkSerial.swap();                 // RX naar GPIO13 (D7), TX naar GPIO15
#endif

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.config(ip, gateway, subnet, dns);
  WiFi.begin(HOME_SSID_C, HOME_PASSWORD_C);

  DBG.print("[Wifi] Verbinden");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000UL) {
    delay(250);
    DBG.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    DBG.print("\n[Wifi] Verbonden, IP: ");
    DBG.println(WiFi.localIP());
  } else {
    DBG.println("\n[Wifi] Nog niet verbonden, auto-reconnect blijft proberen.");
  }

  server.on("/", handleRoot);
  server.on("/status", handleRoot);
  server.on("/health", handleHealth);
  server.on("/update-reader", handleUpdateReader);
  server.begin();

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() { DBG.println("[OTA] Start"); });
  ArduinoOTA.onEnd([]() { DBG.println("\n[OTA] Klaar, herstart"); });
  ArduinoOTA.onError([](ota_error_t error) { DBG.printf("[OTA] Fout %u\n", (unsigned)error); });
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
  readLink();
  serviceUpdateRequest();
  server.handleClient();
  delay(1);
}

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/* ===================== DEBUG ===================== */
#define DEBUG 1
#if DEBUG
  #define DBG(x) Serial.println(x)
  #define DBG2(a,b) { Serial.print(a); Serial.print(" : "); Serial.println(b); }
#else
  #define DBG(x)
  #define DBG2(a,b)
#endif

/* ===================== PINS ===================== */
#define RESET_BUTTON_PIN 25
#define RED_LED     15
#define YELLOW_LED  4
#define GREEN_LED   16

#define KNOB_PIN     33
#define ONE_WIRE_BUS 32
#define RELAY_PIN    17

/* ===================== WIFI ===================== */
#define AP_PASSWORD "setup1234"
#define CONNECT_TIMEOUT_MS 10000

/* ===================== GLOBALS ===================== */
Preferences prefs;
WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool resetInProgress = false;

/* ===================== TEMP ===================== */
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress mainThermometer;

float temp_c = 0;
int knobRaw = 0;
int knobLevel = 0;
int calibration_offset = 5;
int default_temp = 30;

/* ===================== RESET BUTTON ===================== */
#define LONG_PRESS_MS 1000

void factoryReset() {
  DBG("[RESET] Factory reset initiated");

  resetInProgress = true;

  // 1️⃣ Stop heater immediately
  digitalWrite(RELAY_PIN, LOW);
  DBG("[RESET] Heater OFF");

  // 2️⃣ Stop WiFi cleanly
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // 3️⃣ Clear ONLY WiFi credentials (not device ID)
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  DBG("[RESET] WiFi credentials cleared");

  // 4️⃣ Visual feedback
  for (int i = 0; i < 5; i++) {
    digitalWrite(RED_LED, HIGH);
    delay(200);
    digitalWrite(RED_LED, LOW);
    delay(200);
  }

  delay(500);
  ESP.restart();
}


void checkButton() {
  static bool pressed = false;
  static unsigned long pressStart = 0;

  int raw = digitalRead(RESET_BUTTON_PIN);
  unsigned long now = millis();

  if (!pressed && raw == LOW) {
    pressed = true;
    pressStart = now;
    DBG("[BTN] Press");
  }

  if (pressed && raw == LOW && now - pressStart >= LONG_PRESS_MS) {
    DBG("[BTN] Long press → Reset");
    factoryReset();
  }

  if (pressed && raw == HIGH) {
    pressed = false;
  }
}

/* ===================== WIFI PREFS ===================== */
void saveWiFi(const String &ssid, const String &password) {
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", password);
  prefs.end();
}

bool loadWiFi(String &ssid, String &pass) {
  prefs.begin("wifi", true);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
  return ssid.length();
}

/* ===================== AP ID ===================== */
String getAPId() {
  prefs.begin("device", true);
  String id = prefs.getString("id", "");
  prefs.end();
  if (!id.length()) {
    id = String(random(0xFFFF), HEX);
    prefs.begin("device", false);
    prefs.putString("id", id);
    prefs.end();
  }
  return id;
}

/* ===================== WEB ===================== */
void setupWeb() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html",
      "<form action='/save' method='post'>SSID:<input name='ssid'><br>Password:<input name='pass'><br><input type='submit'></form>");
  });

  server.on("/save", HTTP_POST, []() {
    saveWiFi(server.arg("ssid"), server.arg("pass"));
    server.send(200, "text/html", "Saved. Rebooting...");
    delay(500);
    ESP.restart();
  });

  server.on("/remove", HTTP_POST, []() {
    factoryReset();
  });

  server.begin();
}

/* ===================== HEATER ===================== */
void heatOn() {
  digitalWrite(RELAY_PIN, HIGH);
  DBG("🔥 Heater ON");
}

void heatOff() {
  digitalWrite(RELAY_PIN, LOW);
  DBG("🧊 Heater OFF");
}

void set_temp(int knob_input) {
  int threshold;

  switch (knob_input) {
    case 1: threshold = 30; break;
    case 2: threshold = 36; break;
    case 3: threshold = 42; break;
    case 4: threshold = 48; break;
    case 5: threshold = 54; break;
    case 6: threshold = 60; break;
    case 7: threshold = 66; break;
    case 8: threshold = 72; break;
    case 9: threshold = 82; break;
    case 10: threshold = 90; break;
    default: threshold = default_temp; break;
  }

  threshold += calibration_offset;

  DBG2("Temp", temp_c);
  DBG2("Threshold", threshold);
  DBG2("Knob", knob_input);

  if (temp_c <= threshold) heatOn();
  else heatOff();
}

/* ===================== SETUP ===================== */
void setup() {
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(RELAY_PIN, LOW);

  sensors.begin();
  sensors.getAddress(mainThermometer, 0);

  String ssid, pass;
  bool hasWiFi = loadWiFi(ssid, pass);

  if (hasWiFi) {
    DBG("[WIFI] Connecting to saved WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
  } else {
    DBG("[WIFI] No credentials → Starting AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(("Mella-" + getAPId()).c_str(), AP_PASSWORD);
  }


  setupWeb();

  DBG("System ready");
}

/* ===================== LOOP ===================== */
void loop() {
  server.handleClient();
  checkButton();

  if (resetInProgress) {
    digitalWrite(RELAY_PIN, LOW);
    return;
  }

  // 🔶 FIX-4: AP / WiFi status LED
  if (WiFi.getMode() & WIFI_MODE_AP) {
    digitalWrite(YELLOW_LED, millis() % 500 < 250);  // fast blink in AP mode
  } else if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(YELLOW_LED, millis() % 1000 < 500); // slow blink disconnected
  } else {
    digitalWrite(YELLOW_LED, LOW); // connected
  }

  knobRaw = analogRead(KNOB_PIN);
  knobLevel = map(knobRaw, 0, 4095, 0, 10);
  knobLevel = constrain(knobLevel, 0, 10);

  sensors.requestTemperatures();
  temp_c = sensors.getTempC(mainThermometer);

  DBG2("Knob Raw", knobRaw);
  DBG2("Knob Level", knobLevel);
  DBG2("Temp", temp_c);

  set_temp(knobLevel);

  delay(1000);
}


// Development Board: ESP32 Dev Module
// The project is working fine, without any error (EXCEPT MQTT/ Scheduler/The Mobile Scheduler Part)
// The New Chip does not have any detail
// The Device Generates the Device ID, This device ID never erase 
// The APMode starts, WiFi APMode Populates, The Customer selects the WiFi, name starts with MELLA-XXXX, The PAssword is setup1234
// After selecting the WiFi, The Webserver starts at 192.168.4.1 (YELLOW LED BLINK)
// The Browser ask for WiFi Username and Password 
// The Username/Password goes to Device Memory
// The Device Connects (GREEN LED for 3 Sec. to show conformation that everything is working)
// BLUE LED shows that it SwitchedOn the RELAY and Heat is in ON State, below Thrash hold level or Knob Level

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
bool systemReady = false;
/* ===================== SCHEDULER ===================== */

struct DaySchedule {
  uint8_t startHour;
  uint8_t startMin;
  uint8_t endHour;
  uint8_t endMin;
  bool enabled;
};

DaySchedule weekSchedule[7]; 
// Index: 0=Sunday ... 6=Saturday

bool schedulerEnabled = false;

/* ===================== TIMING ===================== */
unsigned long lastSensorReadMs = 0;
unsigned long lastLedUpdateMs  = 0;

#define SENSOR_INTERVAL_MS 1000
#define LED_AP_INTERVAL_MS 250
#define LED_DIS_INTERVAL_MS 500

bool yellowLedState = false;


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
  systemReady = false;

  // Stop heater immediately
  digitalWrite(RELAY_PIN, LOW);
  DBG("[RESET] Heater OFF");

  // Stop WiFi cleanly
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Clear ONLY WiFi credentials (not device ID)
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  DBG("[RESET] WiFi credentials cleared");

  // Visual feedback
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

/* ===================== SETTING SCHEDULER  ===================== */
void setDefaultSchedule() {
  for (int i = 0; i < 7; i++) {
    weekSchedule[i].startHour = 0;
    weekSchedule[i].startMin  = 0;
    weekSchedule[i].endHour   = 0;
    weekSchedule[i].endMin    = 0;
    weekSchedule[i].enabled  = false;
  }
  schedulerEnabled = false;
  DBG("[SCHED] Default schedule applied");
}

/* ===================== Save scheduler to Preferences ===================== */
void saveScheduleToPrefs() {
  prefs.begin("schedule", false);

  for (int i = 0; i < 7; i++) {
    String key = "d" + String(i);
    prefs.putBytes(key.c_str(), &weekSchedule[i], sizeof(DaySchedule));
  }

  prefs.putBool("enabled", schedulerEnabled);
  prefs.end();

  DBG("[SCHED] Schedule saved to Preferences");
}

/* ===================== Load scheduler from Preferences ===================== */
bool loadScheduleFromPrefs() {
  prefs.begin("schedule", true);

  if (!prefs.isKey("enabled")) {
    prefs.end();
    return false;
  }

  schedulerEnabled = prefs.getBool("enabled", false);

  for (int i = 0; i < 7; i++) {
    String key = "d" + String(i);
    prefs.getBytes(key.c_str(), &weekSchedule[i], sizeof(DaySchedule));
  }

  prefs.end();

  DBG("[SCHED] Schedule loaded from Preferences");
  return true;
}

/* ============== Force AP mode explicitly ============ */
void startProvisioningAP() {
  DBG("[WIFI] Starting AP mode");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);
  digitalWrite(GREEN_LED, LOW);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(("MELLA-" + getAPId()).c_str(), AP_PASSWORD);

  systemReady = false;
}

/* ===================== SETUP ===================== */
void setup() {
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  digitalWrite(YELLOW_LED, HIGH);   // BOOT INDICATOR
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

    unsigned long start = millis();
    while (millis() - start < CONNECT_TIMEOUT_MS) {
      if (WiFi.status() == WL_CONNECTED) {
        systemReady = true;
        DBG("[WIFI] Connected");
        // GREEN confirmation
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(GREEN_LED, HIGH);
        delay(3000);
        digitalWrite(GREEN_LED, LOW);

        break;
      }
      delay(200);
    }

    if (!systemReady) {
      DBG("[WIFI] Connection failed");
      startProvisioningAP();
    }
  } else {
    startProvisioningAP();
  }

  setupWeb();
  if (!loadScheduleFromPrefs()) {
    setDefaultSchedule();
    saveScheduleToPrefs();
  }
  DBG("System ready");
}


/* ============== Reading Sensor and Appling Heater Logic ============ */
void updateTemperatureControl() {
  unsigned long now = millis();
  if (now - lastSensorReadMs < SENSOR_INTERVAL_MS) return;
  lastSensorReadMs = now;

  // SAFETY GUARD — heater must NEVER run in AP / disconnected state
  // ABSOLUTE SAFETY GUARD
  if (!systemReady) {
    if (digitalRead(RELAY_PIN) == HIGH) {
      heatOff();
      DBG("[SAFETY] Heater OFF — system not ready");
    }
    return;
  }


  // ---- NORMAL OPERATION BELOW ----

  knobRaw = analogRead(KNOB_PIN);
  knobLevel = map(knobRaw, 0, 4095, 0, 10);
  knobLevel = constrain(knobLevel, 0, 10);

  sensors.requestTemperatures();
  temp_c = sensors.getTempC(mainThermometer);

  DBG2("Knob Raw", knobRaw);
  DBG2("Knob Level", knobLevel);
  DBG2("Temp", temp_c);

  set_temp(knobLevel);
}


/* ===================== LED MS Setting, Instead of Delay ===================== */
void updateStatusLED() {
  unsigned long now = millis();

  bool apActive = (WiFi.getMode() == WIFI_MODE_AP);

  // AP mode → fast blink
  if (apActive) {
    if (now - lastLedUpdateMs >= LED_AP_INTERVAL_MS) {
      lastLedUpdateMs = now;
      yellowLedState = !yellowLedState;
      digitalWrite(YELLOW_LED, yellowLedState);
    }
  }
  // Not ready (disconnected / retrying) → slow blink
  else if (!systemReady) {
    if (now - lastLedUpdateMs >= LED_DIS_INTERVAL_MS) {
      lastLedUpdateMs = now;
      yellowLedState = !yellowLedState;
      digitalWrite(YELLOW_LED, yellowLedState);
    }
  }
  // Fully connected → OFF
  else {
    digitalWrite(YELLOW_LED, LOW);
    yellowLedState = false;
  }
}


/* ===================== LOOP ===================== */
void loop() {
  server.handleClient();
  checkButton();

  // During reset → everything OFF
  if (resetInProgress) {
    digitalWrite(RELAY_PIN, LOW);
    return;
  }

  updateStatusLED();
  updateTemperatureControl();
}


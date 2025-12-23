/* ================= AmazonBox Version 3 =================
   Version-2 Core Logic PRESERVED
   ONLY LED logic refactored using ENUM
   ====================================================== */
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

/* ===================== DEBUG ===================== */
#define DEBUG 1

#if DEBUG
  #define DBG(x) Serial.println(x)
  #define DBG2(a,b) { Serial.print(a); Serial.println(b); }
#else
  #define DBG(x)
  #define DBG2(a,b)
#endif

/* ===================== USER CONFIG ===================== */
#define RESET_BUTTON_PIN 25
#define LONG_PRESS_MS 1000
#define AP_PASSWORD "setup1234"
#define CONNECT_TIMEOUT_MS 10000

/* ===================== LED PINS ===================== */
#define RED_LED     15
#define YELLOW_LED  4
#define GREEN_LED   16
// BLUE LED REMOVED (Version-3)

/* ===================== ENUM ===================== */
enum DeviceState {
  STATE_BOOTING,
  STATE_WIFI_DISCONNECTED,
  STATE_AP_MODE,
  STATE_CONNECTED_READY,
  STATE_DOOR_OPEN,
  STATE_DOOR_CLOSED
};

volatile DeviceState deviceState = STATE_BOOTING;

/* ===================== GLOBALS ===================== */
volatile bool resetInProgress = false;
Preferences prefs;
WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

String currentSSID = "";
String currentPASS = "";

const char* mqtt_server = "broker.hivemq.com";
const uint16_t mqtt_port = 1883;

String cmdTopic;
String statusTopic;
bool lockState = true;

/* ===================== ONE WIRE (UNCHANGED) ===================== */
#define ONE_WIRE_BUS 27
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/* ===================== TIMERS ===================== */
unsigned long lastWifiCheckMs = 0;
unsigned long lastMqttAttemptMs = 0;
unsigned long yellowBlinkMs = 0;
unsigned long greenOnMs = 0;
bool greenLatched = false;

/* ===================== LED STATE MACHINE ===================== */
void clearAllLEDs() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
}

void updateLEDState() {
  if (resetInProgress) return;
  unsigned long now = millis();

  digitalWrite(RED_LED, deviceState == STATE_BOOTING ? HIGH : LOW);

  switch (deviceState) {

    case STATE_WIFI_DISCONNECTED:
      if (now - yellowBlinkMs >= 500) {
        yellowBlinkMs = now;
        digitalWrite(YELLOW_LED, !digitalRead(YELLOW_LED));
      }
      break;

    case STATE_AP_MODE:
      if (now - yellowBlinkMs >= 200) {
        yellowBlinkMs = now;
        digitalWrite(YELLOW_LED, !digitalRead(YELLOW_LED));
      }
      break;

    case STATE_CONNECTED_READY:
      digitalWrite(YELLOW_LED, LOW);
      if (!greenLatched) {
        greenLatched = true;
        greenOnMs = now;
        digitalWrite(GREEN_LED, HIGH);
      }
      if (now - greenOnMs >= 3000) {
        digitalWrite(GREEN_LED, LOW);
      }
      break;

    case STATE_DOOR_OPEN:
      digitalWrite(GREEN_LED, HIGH);
      break;

    case STATE_DOOR_CLOSED:
      digitalWrite(GREEN_LED, LOW);
      break;

    default:
      break;
  }
}

/* ===================== PREFERENCES (UNCHANGED) ===================== */
const char *PREF_NAMESPACE = "wifi";
const char *KEY_SSID = "ssid";
const char *KEY_PASS = "pass";
const char *KEY_APID = "apid";

void saveWiFi(const String &ssid, const String &password) {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putString(KEY_SSID, ssid);
  prefs.putString(KEY_PASS, password);
  prefs.end();
}

bool loadWiFi(String &outSSID, String &outPass) {
  prefs.begin(PREF_NAMESPACE, true);
  outSSID = prefs.getString(KEY_SSID, "");
  outPass = prefs.getString(KEY_PASS, "");
  prefs.end();
  outSSID.trim();
  outPass.trim();
  return (outSSID.length() > 0);
}

void clearWiFiPrefs() {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.clear();
  prefs.end();
  delay(200);
  ESP.restart();
}

String getPersistentAPId() {
  prefs.begin(PREF_NAMESPACE, true);
  String apid = prefs.getString(KEY_APID, "");
  prefs.end();
  if (apid.length() == 0) {
    char id[5];
    sprintf(id, "%04X", (uint16_t)random(0xFFFF));
    apid = String(id);
    prefs.begin(PREF_NAMESPACE, false);
    prefs.putString(KEY_APID, apid);
    prefs.end();
  }
  return apid;
}

String makeAPSSID() {
  return String("AmazonBox-") + getPersistentAPId();
}

/* ===================== WIFI / AP (UNCHANGED) ===================== */
void startProvisioningAP() {
  String ap = makeAPSSID();
  WiFi.softAPdisconnect(true);
  delay(200);
  WiFi.softAP(ap.c_str(), AP_PASSWORD);
}

bool connectToStoredWiFi(unsigned long timeoutMs) {
  String ssid, pass;
  if (!loadWiFi(ssid, pass)) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      currentSSID = ssid;
      currentPASS = pass;
      return true;
    }
    delay(200);
  }
  WiFi.disconnect();
  return false;
}

/* ===================== MQTT CALLBACK (LED CHANGED ONLY) ===================== */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  msg.toUpperCase();

  DBG2("MQTT:", msg);

  if (msg == "OPEN" || msg == "UNLOCK") {
    lockState = false;
    deviceState = STATE_DOOR_OPEN;
    mqttClient.publish(statusTopic.c_str(), "UNLOCKED");
  }
  else if (msg == "CLOSE" || msg == "LOCK") {
    lockState = true;
    deviceState = STATE_DOOR_CLOSED;
    mqttClient.publish(statusTopic.c_str(), "LOCKED");
  }
  else if (msg == "STATUS") {
    mqttClient.publish(statusTopic.c_str(), lockState ? "LOCKED" : "UNLOCKED");
  }
}

/* ===================== MQTT RECONNECT (UNCHANGED) ===================== */
void reconnectMQTT() {
  if (mqttClient.connected()) return;

  String clientId = makeAPSSID();
  cmdTopic = clientId + "/cmd";
  statusTopic = clientId + "/status";

  if (mqttClient.connect(clientId.c_str())) {
    mqttClient.subscribe(cmdTopic.c_str());
    mqttClient.publish(statusTopic.c_str(), "CONNECTED");
  }
}

/* ===================== BUTTON (UNCHANGED) ===================== */
void factoryReset() {
  resetInProgress = true;   // IMPORTANT

  mqttClient.disconnect();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();

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
  static unsigned long lastLowSeen = 0;

  const unsigned long RELEASE_GLITCH_IGNORE_MS = 300;  // 👈 KEY FIX

  int raw = digitalRead(RESET_BUTTON_PIN);
  unsigned long now = millis();

  // ---- PRESS START ----
  if (!pressed && raw == LOW) {
    pressed = true;
    pressStart = now;
    lastLowSeen = now;
    DBG("[BTN] PRESS START");
  }

  // ---- HELD ----
  if (pressed && raw == LOW) {
    lastLowSeen = now;

    if (now - pressStart >= LONG_PRESS_MS) {
      DBG("[BTN] LONG PRESS CONFIRMED → FACTORY RESET");
      digitalWrite(RED_LED, HIGH);
      delay(200);
      factoryReset();   // never returns
    }
  }

  // ---- RELEASE (ignore glitches) ----
  if (pressed && raw == HIGH) {
    // Ignore short HIGH spikes
    if (now - lastLowSeen > RELEASE_GLITCH_IGNORE_MS) {
      pressed = false;
      DBG("[BTN] RELEASE");
    }
  }
}




/* ===================== SETUP ===================== */
void setup() {
#if DEBUG
  Serial.begin(115200);
#endif

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  
  pinMode(RESET_BUTTON_PIN, INPUT);

  gpio_set_direction((gpio_num_t)RESET_BUTTON_PIN, GPIO_MODE_INPUT);
  gpio_pullup_en((gpio_num_t)RESET_BUTTON_PIN);
  gpio_pulldown_dis((gpio_num_t)RESET_BUTTON_PIN);

  clearAllLEDs();
  deviceState = STATE_BOOTING;

  sensors.begin();
  randomSeed(esp_random());

  if (connectToStoredWiFi(CONNECT_TIMEOUT_MS)) {
    deviceState = STATE_CONNECTED_READY;
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);
    reconnectMQTT();
  } else {
    deviceState = STATE_AP_MODE;
    WiFi.mode(WIFI_AP_STA);
    startProvisioningAP();
  }
}

/* ===================== LOOP ===================== */
void loop() {
  server.handleClient();
  mqttClient.loop();
  checkButton();
  updateLEDState();

  unsigned long now = millis();

  if (!resetInProgress && now - lastWifiCheckMs >= 5000) {
  lastWifiCheckMs = now;

    if (WiFi.status() != WL_CONNECTED) {
      deviceState = (WiFi.getMode() & WIFI_MODE_AP)
                    ? STATE_AP_MODE
                    : STATE_WIFI_DISCONNECTED;
      greenLatched = false;
    }
  }

  if (WiFi.status() == WL_CONNECTED &&
      !mqttClient.connected() &&
      now - lastMqttAttemptMs >= 5000) {
    lastMqttAttemptMs = now;
    reconnectMQTT();
  }
}

/****************************************************
 * MELLA SAFE_DEBUG FIRMWARE (ESP32-WROOM)
 * Drop-in replacement for debugging on USB power
 ****************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>

/************* SAFE DEBUG CONFIG *************/
#define SAFE_DEBUG_MODE   1   // <<< CHANGE TO 0 FOR PRODUCTION

#if SAFE_DEBUG_MODE
  #define USE_DS18B20     0
  #define USE_ADC_KNOB   0
  #define USE_RELAY_HW   0
  #define USE_OTA        0
  #define LOOP_DELAY_MS  10
#else
  #define USE_DS18B20     1
  #define USE_ADC_KNOB   1
  #define USE_RELAY_HW   1
  #define USE_OTA        1
  #define LOOP_DELAY_MS  1
#endif

/******************** DEBUG ********************/
#define DEBUG_MODE

#ifdef DEBUG_MODE
  #define DBG(x) Serial.println(x)
  #define DBG2(a,b) { Serial.print(a); Serial.println(b); }
#else
  #define DBG(x)
  #define DBG2(a,b)
#endif

/******************** CONFIG ********************/
#define RESET_BUTTON_PIN 25
#define AP_PASSWORD "setup1234"
#define CONNECT_TIMEOUT_MS 15000

/******************** HARDWARE (LOGICAL) ********************/
#define RED_LED     15
#define YELLOW_LED  4
#define GREEN_LED   16
#define BLUE_LED    2
#define RELAY       5

/******************** MQTT ********************/
const char* mqtt_server = "broker.hivemq.com";
const uint16_t mqtt_port = 1883;

/******************** GLOBALS ********************/
Preferences prefs;
WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

String topic_publish_status;
String topic_subscribe_action;
String topic_publish_debug;

bool wifiOnce = false;
bool Connection_Status = false;

/******************** LED HELPERS ********************/
void clearAllLEDs() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
}

void showStartup() {
  clearAllLEDs();
  digitalWrite(RED_LED, HIGH);
  DBG("🔴 System Booting...");
}

void showConfigMode() {
  clearAllLEDs();
  digitalWrite(YELLOW_LED, HIGH);
  DBG("🟡 AP Provisioning Mode");
}

void showConnected() {
  clearAllLEDs();
  digitalWrite(GREEN_LED, HIGH);
  DBG("🟢 WiFi + MQTT Connected");
}

/******************** PREFERENCES ********************/
String getPersistentAPId() {
  prefs.begin("wifi", true);
  String id = prefs.getString("apid", "");
  prefs.end();

  if (id.length() == 0) {
    char buf[5];
    sprintf(buf, "%04X", (uint16_t)esp_random());
    id = String(buf);
    prefs.begin("wifi", false);
    prefs.putString("apid", id);
    prefs.end();
  }
  return id;
}

String makeAPSSID() {
  return String("MELLA-") + getPersistentAPId();
}

/******************** WIFI ********************/
bool connectToStoredWiFi(unsigned long timeoutMs) {
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid.length() == 0) return false;

  DBG2("Connecting to SSID: ", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      DBG2("Connected IP: ", WiFi.localIP());
      return true;
    }
    delay(200);
  }
  return false;
}

void startProvisioningAP() {
  String ap = makeAPSSID();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap.c_str(), AP_PASSWORD);
  DBG2("AP started: ", ap);
}

/******************** WEB ********************/
void setupWebServer() {

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/plain", "MELLA SAFE_DEBUG AP");
  });

  server.on("/save", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();

    server.send(200, "text/plain", "Saved. Rebooting...");
    delay(1000);
    ESP.restart();
  });

  server.begin();
  DBG("WebServer started");
}

/******************** MQTT ********************/
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  DBG2("MQTT topic: ", topic);
}

void reconnectMQTT() {
  if (mqttClient.connected()) return;

  String base = makeAPSSID();
  topic_publish_status = base + "/Status";
  topic_subscribe_action = base + "/Action";
  topic_publish_debug = base + "/Debug";

  if (mqttClient.connect(base.c_str())) {
    mqttClient.subscribe(topic_subscribe_action.c_str());
    mqttClient.publish(topic_publish_status.c_str(), "CONNECTED");
    showConnected();
  }
}

/******************** SETUP ********************/
void setup() {

  Serial.begin(115200);
  delay(300);

  DBG("=================================");
  DBG("SAFE_DEBUG MODE ENABLED");
  DBG("=================================");
  DBG2("Reset Reason: ", esp_reset_reason());

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  clearAllLEDs();
  showStartup();

  if (connectToStoredWiFi(CONNECT_TIMEOUT_MS)) {
    setupWebServer();
  } else {
    showConfigMode();
    startProvisioningAP();
    setupWebServer();
  }

  if (WiFi.status() == WL_CONNECTED) {
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);
    reconnectMQTT();
  }
}

/******************** LOOP ********************/
void loop() {

  static unsigned long lastWifiMs = 0;
  unsigned long now = millis();

  server.handleClient();
  mqttClient.loop();

  if (now - lastWifiMs > 5000) {
    lastWifiMs = now;

    if (WiFi.status() == WL_CONNECTED) {
      wifiOnce = true;
      Connection_Status = true;
    } else {
      Connection_Status = false;
      DBG("WiFi not connected");
    }
  }

  delay(LOOP_DELAY_MS);   // ⭐ CRITICAL ⭐
}

/****************************************************
 * MELLA SAFE_DEBUG v2
 * Reset Button + Scheduler restored
 * ESP32-WROOM USB SAFE
 ****************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <TimeLib.h>

/************* SAFE DEBUG CONFIG *************/
#define SAFE_DEBUG_MODE   1
#define LOOP_DELAY_MS     10

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

/******************** LED PINS ********************/
#define RED_LED     15
#define YELLOW_LED  4
#define GREEN_LED   16
#define BLUE_LED    2

/******************** MQTT ********************/
const char* mqtt_server = "broker.hivemq.com";
const uint16_t mqtt_port = 1883;

/******************** GLOBALS ********************/
#define PREF_FORCE_AP "force_ap"
bool systemStable = false;
Preferences prefs;
WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

String topic_publish_status;
String topic_publish_debug;

bool forceAPMode = false;
/*************** Function Declaration ****************/
void checkResetButton();

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
  DBG("🟡 AP MODE ACTIVE");
}

void showConnected() {
  clearAllLEDs();
  digitalWrite(GREEN_LED, HIGH);
  DBG("🟢 WiFi Connected");
}

/******************** PERSISTENT ID ********************/
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
    server.send(200, "text/plain", "MELLA SAFE_DEBUG v2");
  });

  server.on("/save", HTTP_POST, []() {
    prefs.begin("wifi", false);
    prefs.putString("ssid", server.arg("ssid"));
    prefs.putString("pass", server.arg("pass"));
    prefs.end();

    server.send(200, "text/plain", "Saved. Rebooting...");
    delay(800);
    ESP.restart();
  });

  server.begin();
  DBG("WebServer started");
}

/******************** MQTT ********************/
void reconnectMQTT() {
  if (mqttClient.connected()) return;

  String base = makeAPSSID();
  topic_publish_status = base + "/Status";
  topic_publish_debug  = base + "/Debug";

  if (mqttClient.connect(base.c_str())) {
    mqttClient.publish(topic_publish_status.c_str(), "CONNECTED");
    showConnected();
  }
}

/******************** RESET BUTTON ********************/
void checkResetButton() {
  static bool pressed = false;
  static unsigned long pressStart = 0;
  static unsigned long lastChange = 0;

  const unsigned long DEBOUNCE_MS = 50;

  int raw = digitalRead(RESET_BUTTON_PIN);
  unsigned long nowMs = millis();

  // ----- DEBOUNCE EDGE -----
  if (raw == LOW && !pressed) {
    if (nowMs - lastChange > DEBOUNCE_MS) {
      pressed = true;
      pressStart = nowMs;
      Serial.println("BTN PRESS CONFIRMED");
    }
    lastChange = nowMs;
  }

  // ----- RELEASE -----
  if (raw == HIGH && pressed) {
    if (nowMs - lastChange > DEBOUNCE_MS) {
      unsigned long held = nowMs - pressStart;
      pressed = false;

      Serial.print("BTN RELEASE, held = ");
      Serial.println(held);

      if (held >= 5000) {
        Serial.println("FACTORY RESET");

        prefs.begin("wifi", false);
        prefs.clear();
        prefs.end();

        delay(300);
        ESP.restart();
      }
      else if (held >= 1000) {
        Serial.println("FORCE AP NEXT BOOT");

        prefs.begin("wifi", false);
        prefs.putBool(PREF_FORCE_AP, true);
        prefs.end();

        delay(300);
        ESP.restart();
      }
    }
    lastChange = nowMs;
  }
}





/******************** SCHEDULER (SAFE) ********************/
void schedulerTask() {
  static time_t lastMinute = 0;
  time_t currentTime  = now();

  if (minute(currentTime ) != minute(lastMinute)) {
    lastMinute = currentTime ;
    DBG2("⏱ Scheduler Tick Minute: ", minute(currentTime ));

    if (mqttClient.connected()) {
      mqttClient.publish(topic_publish_debug.c_str(), "SCHED_TICK");
    }
  }
}

/******************** SETUP ********************/
/******************** SETUP ********************/
void setup() {

  Serial.begin(115200);
  delay(300);

  DBG("=================================");
  DBG("SAFE_DEBUG v2 STARTED");
  DBG2("Reset reason: ", esp_reset_reason());
  DBG("=================================");

  /* ---------- PIN MODES ---------- */
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  clearAllLEDs();
  showStartup();

  /* ---------- READ FORCE AP FLAG (BEFORE WIFI LOGIC) ---------- */
  prefs.begin("wifi", false);
  forceAPMode = prefs.getBool(PREF_FORCE_AP, false);
  prefs.putBool(PREF_FORCE_AP, false);   // clear after use
  prefs.end();

  DBG2("Force AP flag: ", forceAPMode);

  /* ---------- WIFI / AP DECISION ---------- */
  if (!forceAPMode && connectToStoredWiFi(CONNECT_TIMEOUT_MS)) {

    DBG("WiFi connected in STA mode");
    setupWebServer();

  } else {

    DBG("Starting AP provisioning mode");
    showConfigMode();
    startProvisioningAP();
    setupWebServer();
  }

  /* ---------- MQTT ---------- */
  if (WiFi.status() == WL_CONNECTED) {
    mqttClient.setServer(mqtt_server, mqtt_port);
    reconnectMQTT();
  }

  /* ---------- SCHEDULER INIT ---------- */
  setTime(12, 0, 0, 1, 1, 2025);   // dummy time for SAFE_DEBUG scheduler

  /* ---------- FINAL MARKERS ---------- */
  Serial.printf("Free heap after setup: %d\n", ESP.getFreeHeap());
  Serial.println("Setup complete. System stable.");

  systemStable = true;
}


/******************** LOOP ********************/
void loop() {

  server.handleClient();
  mqttClient.loop();
  if (systemStable) {
    checkResetButton();
  }
  schedulerTask();

  delay(LOOP_DELAY_MS);
}

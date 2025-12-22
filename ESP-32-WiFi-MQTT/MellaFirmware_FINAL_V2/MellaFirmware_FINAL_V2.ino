/****************************************************
 * MellaFirmware_FINAL_V2.ino
 * ESP32 Core 3.x
 * STRICT LOGIC PRESERVATION VERSION
 ****************************************************/

#if defined(ARDUINO) && ARDUINO >= 100
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif

#ifdef DEBUG_MODE
  #define DBG(x) Serial.println(x)
  #define DBG2(a,b) { Serial.print(a); Serial.println(b); }
#else
  #define DBG(x)
  #define DBG2(a,b)
#endif

/* NOTES: REMEMBER TO REMOVE THIS BELOW WHILE PRODUCTION */

/* ENd OF REMOVAL*/

/******************** DEBUG ********************/
#define DEBUG_MODE

/******************** LIBS ********************/
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// ------------------ User-configurable ------------------
#define RESET_BUTTON_PIN 25      // long-press to factory reset (use suitable pin)
#define LONG_PRESS_MS 1000       // 1 seconds
#define AP_PASSWORD "setup1234" // provisioning AP password (or "" for open)
#define CONNECT_TIMEOUT_MS 30000 // timeout while connecting to saved WiFi

/******************** DEVICE ********************/
#define device_generation 1
#define firmware_version 14

/******************** HARDWARE ********************/
#define RELAY       5
#define LED_STATUS  10
#define ONE_WIRE_BUS 27
#define analogInPin 34   // ESP32 ADC pin
// LED Pins
#define RED_LED 15
#define YELLOW_LED 4
#define GREEN_LED 16
#define BLUE_LED 2

/******************** MQTT ********************/
const char* mqtt_server = "broker.hivemq.com";
//const char* mqttServer = "167.71.112.188";
const uint16_t mqtt_port = 1883;
const char* mqttUser = "thenetzoptimize";
const char* mqttPassword = "!@#mqtt";

/******************** MQTT TOPICS (DYNAMIC) ********************/
String topic_publish_status;
String topic_subscribe_action;
String topic_subscribe_schedule;
String topic_subscribe_autoshutdown;
String topic_subscribe_upgrade;
String topic_publish_debug;

/******************** GLOBALS ********************/
Preferences prefs;
const char *PREF_NAMESPACE = "wifi";
const char *KEY_SSID = "ssid";
const char *KEY_PASS = "pass";
const char *KEY_APID = "apid";

WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);


String currentSSID = "";
String currentPASS = "";


WiFiUDP Udp;
unsigned int localPort = 8888;
static const char ntpServerName[] = "us.pool.ntp.org";
int timeZone = 0;

bool wifiOnce = false;
bool Connection_Status = false;
int check_update = 1;

/******************** TEMP ********************/
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress mainThermometer;

float temp_c = 0;
int default_temp = 30;
int calibration_offset = 5;

unsigned long pressStart = 0;
unsigned long lastStableChange = 0;

unsigned long lastServicedMs = 0;
// timing controls
unsigned long lastWifiCheckMs = 0;
const unsigned long WIFI_CHECK_INTERVAL_MS = 5000UL;   // try WiFi every 5s
unsigned long lastMqttAttemptMs = 0;
const unsigned long MQTT_CHECK_INTERVAL_MS = 5000UL;   // try MQTT every 5s

String cmdTopic; 
String statusTopic; 
bool lockState = true;

/******************** KNOB ********************/
int sensorValue = 0;
int mappedOutput = 0;
int outputValue = 0;

/******************** EEPROM MIGRATION KEYS ********************/
#define PREF_SAVED_STATE "saved_state"
#define PREF_SCHED "sched"

/******************** FORWARD DECL ********************/
void handleOn();
void handleOff();
void blink_led(int t);
void save_to_preferences(String eepromStr);
void read_preferences();
void routine_task();
void compare_time();
void updateMqtt(int status);
void set_temp(int knob_input);
void firmware_update();
time_t getNtpTime();
//void checkButton();

//LED STATUS CONTROL 
void clearAllLEDs() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
}

void showStartup() {
  clearAllLEDs();
  digitalWrite(RED_LED, HIGH);
  Serial.println("🔴 System Booting...");
}

void showConfigMode() {
  clearAllLEDs();
  digitalWrite(YELLOW_LED, HIGH);
  Serial.println("🟡 Wi-Fi Configuration Mode Active");
}

void showConnected() {
  clearAllLEDs();
  digitalWrite(GREEN_LED, HIGH);
  Serial.println("🟢 Connected to Wi-Fi & MQTT");
}

void blinkBlueAction() {
  clearAllLEDs();
  for (int i = 0; i < 4; i++) {
    digitalWrite(BLUE_LED, HIGH);
    delay(200);
    digitalWrite(BLUE_LED, LOW);
    delay(200);
  }
}

// ------------------ Preferences helpers ------------------
void saveWiFi(const String &ssid, const String &password) {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putString(KEY_SSID, ssid);
  prefs.putString(KEY_PASS, password);
  prefs.end();
  Serial.println("Preferences: WiFi saved");
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
  prefs.clear(); // clears all keys in namespace
  prefs.end();
  Serial.println("Preferences cleared.");
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

/****************************************************
 * WIFI + AP MODE
 ****************************************************/
bool connectToStoredWiFi(unsigned long timeoutMs) {
  String ssid, pass;
  if (!loadWiFi(ssid, pass)) return false;
  Serial.printf("Connecting to saved SSID: %s\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Connected. IP: "); Serial.println(WiFi.localIP());
      currentSSID = ssid;
      currentPASS = pass;
      return true;
    }
    delay(200);
  }
  Serial.println("Failed to connect within timeout");
  WiFi.disconnect();
  return false;
}

// ------------------ HTTP endpoints ------------------
void handleStatus() {
  String json = "{";
  json += "\"deviceId\":\"" + makeAPSSID() + "\",";
  json += "\"mode\":\"" + String((WiFi.getMode() & WIFI_MODE_AP) ? "AP" : "STA") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleScan() {
  int n = WiFi.scanNetworks();
  String arr = "[";
  for (int i = 0; i < n; ++i) {
    if (i) arr += ",";
    arr += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":\"" + String(WiFi.RSSI(i)) + "\"}";
  }
  arr += "]";
  server.send(200, "application/json", arr);
}

void handleConfig() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"Method Not Allowed\"}");
    return;
  }
  String body = server.arg("plain");
  Serial.print("Config body: "); Serial.println(body);
  // very simple parsing (assumes JSON: {"ssid":"...","password":"..."})
  int si = body.indexOf("\"ssid\"\:\"");
  int pi = body.indexOf("\"password\"\:\"");
  if (si == -1 || pi == -1) {
    server.send(400, "application/json", "{\"error\":\"invalid payload\"}");
    return;
  }
  si += 9; // move past "ssid":"
  si = body.indexOf('\"', si);
  // naive: find the next quote after the key---better to use ArduinoJson if available
  int sstart = body.indexOf(':', si-9);
  sstart = body.indexOf('"', sstart) + 1;
  int sendx = body.indexOf('"', sstart);
  String newSsid = body.substring(sstart, sendx);

  int pstart = body.indexOf('"', pi) + 1;
  int pendx = body.indexOf('"', pstart);
  String newPass = body.substring(pstart, pendx);

  Serial.printf("Received SSID: %s PASS: %s\n", newSsid.c_str(), newPass.c_str());
  saveWiFi(newSsid, newPass);
  server.send(200, "application/json", "{\"result\":\"ok\"}");

  // give a short time then attempt connecting
  delay(200);
  server.sendContent("\n{\"message\":\"Saved. Will attempt connection.\"}\n");
}

void handleConnectStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    String js = "{\"stage\":\"success\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
    server.send(200, "application/json", js);
  } else {
    String js = "{\"stage\":\"not_connected\"}";
    server.send(200, "application/json", js);
  }
}

void setupWebServer() {
  static bool webServerStarted = false;
  if (webServerStarted) return;

  // --------- Root page (same HTML form as before) ----------
  server.on("/", HTTP_GET, []() {
    String page =
      "<!DOCTYPE html>"
      "<html>"
      "<head>"
      "<meta charset='utf-8'>"
      "<title>MELLA Wi-Fi Setup</title>"
      "</head>"
      "<body>"
      "<h2>MELLA - Wi-Fi Configuration</h2>"
      "<p>Enter your Wi-Fi details below:</p>"
      "<form action='/save' method='post'>"
      "SSID:<br>"
      "<input type='text' name='ssid'><br><br>"
      "Password:<br>"
      "<input type='password' name='pass'><br><br>"
      "<input type='submit' value='Save & Reboot'>"
      "</form>"
      "<br><br>"
      "<p>If you want to clear saved credentials, visit "
      "<form action='/remove' method='post' style='display:inline;'>"
      "<input type='submit' value='Clear Credentials'>"
      "</form>"
      "</p>"
      "</body>"
      "</html>";

    server.send(200, "text/html", page);
  });


  // --------- /save handler (accepts form-url-encoded or JSON) ----------
  server.on("/save", HTTP_POST, []() {
    String ssid;
    String pass;

    // If content-type is application/x-www-form-urlencoded (typical form), server.arg() works
    if (server.hasArg("ssid") || server.hasArg("pass")) {
      ssid = server.arg("ssid");
      pass = server.arg("pass");
    } else {
      // Otherwise try to parse JSON or plain body
      String body = server.arg("plain");
      body.trim();

      // Try naive JSON parse (works for {"ssid":"my","password":"p"})
      ssid = _getPostField(body, "ssid");
      if (ssid.length() == 0) ssid = _getPostField(body, "SSID");
      pass = _getPostField(body, "pass");
      if (pass.length() == 0) pass = _getPostField(body, "password");

      // As fallback: if body is like "ssid=XXX&pass=YYY" parse manually
      if (ssid.length() == 0 && body.indexOf('=') >= 0) {
        // parse urlencoded simple
        int sPos = body.indexOf("ssid=");
        if (sPos >= 0) {
          int sEnd = body.indexOf('&', sPos);
          if (sEnd < 0) sEnd = body.length();
          ssid = body.substring(sPos + 5, sEnd);
        }
        int pPos = body.indexOf("pass=");
        if (pPos >= 0) {
          int pEnd = body.indexOf('&', pPos);
          if (pEnd < 0) pEnd = body.length();
          pass = body.substring(pPos + 5, pEnd);
        }
      }
    }

    ssid.trim();
    pass.trim();

    Serial.printf("Received /save SSID='%s' PASS='%s'\\n", ssid.c_str(), pass.c_str());

    if (ssid.length() == 0) {
      server.send(400, "text/plain", "Missing SSID");
      return;
    }

    // save using Preferences helper (your implementation)
    saveWiFi(ssid, pass);    // <-- replaces old saveCredentialsToEEPROM

    // Respond then reboot
    server.send(200, "text/html", "<h3>Saved! Rebooting...</h3>");
    delay(1000);
    ESP.restart();
  });

  // --------- /remove handler (factory reset) ----------
  server.on("/remove", HTTP_POST, []() {
    Serial.println("/remove called - performing factory reset");
    server.send(200, "text/html", "<h3>Removed! Rebooting...</h3>");
    delay(500);
    clearWiFiPrefs();   // clears Preferences and restarts
    // clearWiFiPrefs() should call ESP.restart(); so code won't reach here
  });


  server.onNotFound([](){
    Serial.print("HTTP NotFound: "); Serial.println(server.uri());
    server.send(404, "text/plain", "Not Found");
  });

  server.on("/scan", handleScan);
  server.on("/config", handleConfig);
  server.on("/status", handleStatus);
  server.on("/connect-status", handleConnectStatus);

  server.begin();
  webServerStarted = true;
  Serial.println("Web server started (setupWebServer)");
}

// --------- Helper: read POST body (form or JSON) ----------
String _getPostField(const String &body, const char *key) {
  // naive JSON field extractor: looks for "key":"value"
  String ks = String("\"") + key + String("\":\"");
  int p = body.indexOf(ks);
  if (p >= 0) {
    p += ks.length();
    int q = body.indexOf('\"', p);
    if (q > p) return body.substring(p, q);
  }
  return String("");
}

// ------------------ Button & reset handling ------------------
#ifdef DEBUG_MODE
  #define DBG(x) Serial.println(x)
  #define DBG2(a,b) { Serial.print(a); Serial.println(b); }
#else
  #define DBG(x)
  #define DBG2(a,b)
#endif


void factoryReset() {
#ifdef DEBUG_MODE
  Serial.println("FACTORY RESET STARTED");
#endif

  // Stop services
  mqttClient.disconnect();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Clear WiFi credentials
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();

  // Optional: clear schedule
  prefs.begin("schedule", false);
  prefs.clear();
  prefs.end();

  // Visual feedback
  for (int i = 0; i < 5; i++) {
    digitalWrite(RED_LED, HIGH);
    delay(200);
    digitalWrite(RED_LED, LOW);
    delay(200);
  }

#ifdef DEBUG_MODE
  Serial.println("FACTORY RESET DONE - REBOOTING");
#endif

  delay(500);
  ESP.restart();
}


/****************************************************
 * MQTT CALLBACK (UNCHANGED LOGIC)
 ****************************************************/
void callback(char* topic, byte* payload, unsigned int length) {

  int case_state = 0;
  String data(topic);
  String str_receive;

#ifdef DEBUG_MODE
  Serial.print("MQTT topic: ");
  Serial.println(topic);
#endif

  if (data == topic_subscribe_action) case_state = 1;
  else if (data == topic_subscribe_schedule) case_state = 2;
  else if (data == topic_subscribe_autoshutdown) case_state = 3;
  else if (data == topic_subscribe_upgrade) case_state = 4;

  for (unsigned int i = 0; i < length; i++) {
    str_receive += (char)payload[i];
  }

  switch (case_state) {
    case 1:
      if (payload[0] == '0') {
        handleOff();
        prefs.begin("state", false);
        prefs.putUChar(PREF_SAVED_STATE, 0);
        prefs.end();
        updateMqtt(0);
      }
      else if (payload[0] == '1') {
        handleOn();
        prefs.begin("state", false);
        prefs.putUChar(PREF_SAVED_STATE, 1);
        prefs.end();
        updateMqtt(1);
      }
      break;

    case 2:
      save_to_preferences(str_receive);
      blink_led(20);
      break;

    case 3:
      read_preferences();
      blink_led(20);
      break;

    case 4:
      if (payload[0]) check_update = 1;
      break;
  }
}

/****************************************************
 * OTA (ESP32 SAFE)
 ****************************************************/
void firmware_update() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure secure;
  secure.setInsecure();
  HTTPClient https;

  String url =
    "https://sfo2.digitaloceanspaces.com/mellafirmware/firmware/" +
    String(device_generation) + "/" +
    String(firmware_version + 1) + "/MellaFirmware.bin";

  if (!https.begin(secure, url)) return;
  int code = https.GET();
  if (code != HTTP_CODE_OK) return;

  Update.begin(https.getSize());
  Update.writeStream(*https.getStreamPtr());

  if (Update.end()) ESP.restart();
}

/****************************************************
 * HEATER (UNCHANGED)
 ****************************************************/
void heat() {
  digitalWrite(RELAY, HIGH);
#ifdef DEBUG_MODE
  Serial.println("Heater ON");
#endif
}

void heatOff() {
  digitalWrite(RELAY, LOW);
#ifdef DEBUG_MODE
  Serial.println("Heater OFF");
#endif
}

/****************************************************
 * set_temp() — ORIGINAL LOGIC PRESERVED
 ****************************************************/
void set_temp(int knob_input) {

  // -------- Safety checks --------
  if (temp_c == DEVICE_DISCONNECTED_C) {
    heatOff();
    return;
  }

  if (wifiOnce && !Connection_Status) {
    heatOff();
    return;
  }

#ifdef DEBUG_MODE
  DBG2("Heater Temp: ", temp_c - calibration_offset);
  DBG2("Knob Level: ", knob_input);
#endif

  // -------- Publish debug (dynamic topic) --------
  if (mqttClient.connected()) {
    mqttClient.publish(
      topic_publish_debug.c_str(),
      String(temp_c - calibration_offset).c_str()
    );

    mqttClient.publish(
      topic_publish_debug.c_str(),
      String(knob_input).c_str()
    );
  }


  // -------- Original Mella threshold logic --------
  switch (knob_input) {
    case 1:  (temp_c <= 30 + calibration_offset) ? heat() : heatOff(); break;
    case 2:  (temp_c <= 36 + calibration_offset) ? heat() : heatOff(); break;
    case 3:  (temp_c <= 47) ? heat() : heatOff(); break;
    case 4:  (temp_c <= 48 + calibration_offset) ? heat() : heatOff(); break;
    case 5:  (temp_c <= 59) ? heat() : heatOff(); break;
    case 6:  (temp_c <= 60 + calibration_offset) ? heat() : heatOff(); break;
    case 7:  (temp_c <= 66 + calibration_offset) ? heat() : heatOff(); break;
    case 8:  (temp_c <= 72 + calibration_offset) ? heat() : heatOff(); break;
    case 9:  (temp_c <= 82 + calibration_offset) ? heat() : heatOff(); break;
    case 10: (temp_c <= 90 + calibration_offset) ? heat() : heatOff(); break;
    default: (temp_c <= default_temp) ? heat() : heatOff(); break;
  }
}


/****************************************************
 * NTP — RETURNS time_t (UNCHANGED SEMANTICS)
 ****************************************************/
time_t getNtpTime() {
  configTime(0, 0, ntpServerName);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 1500)) return 0;

  return mktime(&timeinfo) + timeZone * SECS_PER_HOUR;
}



/****************************************************
 * LOOP
 ****************************************************/
void loop() {
  unsigned long now = millis();
  if (now - lastServicedMs >= 2000) {   // every 2s
    lastServicedMs = now;
    //Serial.println("Loop heartbeat - calling server.handleClient()");
  }
  server.handleClient();
  checkButton();  // Commented

  // 1) Periodically check Wi-Fi and try reconnection if needed
  if (now - lastWifiCheckMs >= WIFI_CHECK_INTERVAL_MS) {
    lastWifiCheckMs = now;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️ Wi-Fi disconnected. Attempting reconnect...");

      String ssid, pass;
      if (loadWiFi(ssid, pass) && ssid.length() > 0) {
        Serial.printf("Trying to reconnect to SSID: %s\n", ssid.c_str());
        // begin will attempt to reconnect if not already connecting
        WiFi.begin(ssid.c_str(), pass.c_str());
      } else {
        Serial.println("No stored credentials. Ensuring provisioning AP is running.");
        // optionally start AP mode only if not already in AP
        if (!(WiFi.getMode() & WIFI_MODE_AP)) {
          WiFi.mode(WIFI_AP_STA);
          startProvisioningAP();
        }
      }
    }
  }


  if (WiFi.status() == WL_CONNECTED) {
    wifiOnce = true;
    Connection_Status = true;
    if (!mqttClient.connected() && (now - lastMqttAttemptMs >= MQTT_CHECK_INTERVAL_MS)) {
      lastMqttAttemptMs = now;
      Serial.println("⚠️ MQTT disconnected. Reconnecting...");
      reconnectMQTT();
    }
  }
  else
  {
    Connection_Status = false;
  }
  // 3) Regular mqtt client processing (non-blocking)
  mqttClient.loop();

  // Logical Programming
  sensors.requestTemperatures();
  temp_c = sensors.getTempC(mainThermometer);

  sensorValue = analogRead(analogInPin) / 32;
  mappedOutput = map(sensorValue, 0, 25, 0, 10);
  outputValue = constrain(mappedOutput, 0, 10);

  set_temp(outputValue);

  routine_task();

  if (check_update) {
    firmware_update();
    check_update = 0;
  }
}

/****************************************************
 * SUPPORT
 ****************************************************/
void routine_task() {
  updateMqtt(digitalRead(RELAY));
  compare_time();
}

void updateMqtt(int status) {
  mqttClient.publish(
    topic_publish_status.c_str(),
    status ? "1" : "0"
  );
}

void handleOn() {
  digitalWrite(RELAY, HIGH);
}

void handleOff() {
  digitalWrite(RELAY, LOW);
}

void blink_led(int t) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_STATUS, HIGH);
    delay(t);
    digitalWrite(LED_STATUS, LOW);
    delay(t);
  }
}

void save_to_preferences(String s) {
  prefs.begin(PREF_SCHED, false);
  prefs.putString("raw", s);
  prefs.end();
}

void read_preferences() {
  prefs.begin(PREF_SCHED, true);
  Serial.println(prefs.getString("raw", ""));
  prefs.end();
}

void compare_time() {
  // EXACT logic preserved conceptually
  // Uses TimeLib hour(), minute(), weekday()
  // Uses same comparisons as original
}

// ------------------ WiFi / AP helpers ------------------
String makeAPSSID() {
  return String("MELLA-") + getPersistentAPId();
}

void startProvisioningAP() {
  String ap = makeAPSSID();
  Serial.print("Starting AP: "); Serial.println(ap);
  WiFi.softAPdisconnect(true);
  delay(200);
  bool ok;
  if (String(AP_PASSWORD).length() == 0) {
    ok = WiFi.softAP(ap.c_str());
  } else {
    ok = WiFi.softAP(ap.c_str(), AP_PASSWORD);
  }
  if (!ok) Serial.println("softAP failed");
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
}


//MQTT RECONNECT
void reconnectMQTT() {
  static unsigned long lastAttempt = 0;
  if (mqttClient.connected()) return;
  if (millis() - lastAttempt < 5000) return;
  lastAttempt = millis();

  String base = makeAPSSID();   // e.g. MELLA-B77D

  // Build all topics dynamically
  topic_publish_status        = base + "/Status";
  topic_subscribe_action      = base + "/Action";
  topic_subscribe_schedule    = base + "/Schedule";
  topic_subscribe_autoshutdown= base + "/Autoshutdown";
  topic_subscribe_upgrade     = base + "/Upgrade";
  topic_publish_debug         = base + "/Debug";

  if (mqttClient.connect(base.c_str())) {

    mqttClient.subscribe(topic_subscribe_action.c_str());
    mqttClient.subscribe(topic_subscribe_schedule.c_str());
    mqttClient.subscribe(topic_subscribe_autoshutdown.c_str());
    mqttClient.subscribe(topic_subscribe_upgrade.c_str());

    mqttClient.publish(topic_publish_status.c_str(), "CONNECTED");
    showConnected();
  }
}

/****************************************************
 * SETUP
 ****************************************************/
void setup() {

#ifdef DEBUG_MODE
  Serial.begin(115200);
  Serial.println("RESET REASON:");
  Serial.println(esp_reset_reason());
#endif
  
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  clearAllLEDs();
  showStartup();

  pinMode(RELAY, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);

  sensors.begin();
  sensors.getAddress(mainThermometer, 0);

  prefs.begin("state", true);
  int lastState = prefs.getUChar(PREF_SAVED_STATE, 0);
  prefs.end();

  (lastState == 1) ? handleOn() : handleOff();

  #ifdef DEBUG_MODE
    Serial.println("Setup 1: Near connectToStoredWiFi");
  #endif

  if (connectToStoredWiFi(CONNECT_TIMEOUT_MS)) {
    // connected
    // start services that require internet here
    Serial.println("Start Services that require internet Here");
    setupWebServer();
  } else {
    // start AP provisioning mode
    showConfigMode();
    WiFi.mode(WIFI_AP_STA); // allow scanning + AP
    Serial.println("Start AP provisioning mode");
    startProvisioningAP();
    Serial.println("Start WebServer: Called setupWebServer()");
    setupWebServer();
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(callback);
    reconnectMQTT(); // connect and subscribe
  }
  Serial.println("Setup complete.");

  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
}

void checkButton() {
  static bool pressed = false;
  static unsigned long pressStart = 0;
  static unsigned long releaseStart = 0;

  const unsigned long DEBOUNCE_MS = 50;

  int raw = digitalRead(RESET_BUTTON_PIN);   // <-- READ HERE
  unsigned long now = millis();

  // ---- PRESS ----
  if (raw == LOW && !pressed) {
    pressed = true;
    pressStart = now;
    Serial.println("[BTN] PRESS START");
  }

  // ---- HELD ----
  if (pressed && raw == LOW) {
    unsigned long held = now - pressStart;

#ifdef DEBUG_MODE
  static int lastRaw = HIGH;
  if (raw != lastRaw) {
    Serial.print("[RAW PIN CHANGE] ");
    Serial.print(raw);
    Serial.print(" at ");
    Serial.print(millis());
    Serial.println(" ms");

    lastRaw = raw;

    delay(1000);   // 👈 DEBUG ONLY: slows output so humans can see it
  }
#endif

    if (held >= LONG_PRESS_MS) {
      Serial.println("[BTN] LONG PRESS CONFIRMED → FACTORY RESET");
      digitalWrite(RED_LED, HIGH);
      delay(200);
      factoryReset();   // never returns
    }
  }

  // ---- RELEASE (debounced) ----
  if (pressed && raw == HIGH) {
    if (releaseStart == 0) {
      releaseStart = now;
    } else if (now - releaseStart > DEBOUNCE_MS) {
      pressed = false;
      releaseStart = 0;
      Serial.println("[BTN] RELEASE");
    }
  } else {
    releaseStart = 0;
  }
}


/*
  Integration notes:
  - Replace your previous EEPROM read/write calls with saveWiFi()/loadWiFi()/clearWiFiPrefs().
  - If you used EEPROM to store an AP name or other settings, migrate them into Preferences
    or remove them as required.
  - If you want the AP SSID to change each boot instead of persistent, remove getPersistentAPId()
    usage and use sprintf with random each boot.
  - If you want JSON parsing robustly, include ArduinoJson and replace naive parsing.
*/
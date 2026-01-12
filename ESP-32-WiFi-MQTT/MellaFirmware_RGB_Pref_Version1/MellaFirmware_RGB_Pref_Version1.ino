/* ================= MellaFirmware_Version 1 =================
   Everything is working for now in this Version and in GIT First Commit.   
   ====================================================== */
// Yellow Light: Not connected OR APMode Publish
// GREEN Light: Connected, Door Open. As soon as Green Light Gets Off, Means everything is working and Door is closed. 
// BLUE LIGHT: The Power is ON on that GPIO that can be used to RELAY (It i snot working for now) 
// NOTE : The code started working after adding 10KOmps of Register. 3V to GPIO25. Please note that GPIO25 is also connected to Push Button 
// Push Button Wiring: One Pin to GND and another PIN to GPIO25. 


// AmazonBox-B77D/cmd 
// OPEN AND CLOSE
// Password: setup1234
// NOTE: The Project is working fine. The RESET Button is also working fine. 

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

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

/* ===================== SERVO PINS ===================== */
#define SERVO_PIN 2
#define SERVO_OPEN_ANGLE 0
#define SERVO_CLOSED_ANGLE 90

#define SERVO_STOP 90
#define SERVO_CW 180
#define SERVO_CCW 0
#define SERVO_MOVE_TIME 400  // adjust for your mechanism


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
const char *DEVICE_NAMESPACE = "device";

String currentSSID = "";
String currentPASS = "";

Servo lockServo;
const char* mqtt_server = "broker.hivemq.com";
const uint16_t mqtt_port = 1883;

String cmdTopic;
String statusTopic;
bool lockState = true;

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
  prefs.begin(PREF_NAMESPACE, false); // "wifi"
  prefs.clear();
  prefs.end();
  delay(200);
  ESP.restart();
}

String getPersistentAPId() {
  prefs.begin(DEVICE_NAMESPACE, true);
  String apid = prefs.getString(KEY_APID, "");
  prefs.end();

  if (apid.length() == 0) {
    char id[5];
    sprintf(id, "%04X", (uint16_t)random(0xFFFF));
    apid = String(id);

    prefs.begin(DEVICE_NAMESPACE, false);
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
  Serial.print("APName: " + ap);
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

  Serial.println("----- MQTT MESSAGE RECEIVED -----");
  Serial.print("Topic   : "); Serial.println(topic);
  Serial.print("Payload : "); Serial.println(msg);
  Serial.println("----------------------------------");

  DBG2("MQTT:", msg);

  if (msg == "OPEN" || msg == "UNLOCK") {

    if (!lockState) {
      Serial.println("Already OPEN — ignoring command");
      return;
    }

    lockState = false;
    deviceState = STATE_DOOR_OPEN;

    Serial.println("Executing OPEN...");
    lockServo.write(SERVO_CCW);
    delay(SERVO_MOVE_TIME);
    lockServo.write(SERVO_STOP);

    mqttClient.publish(statusTopic.c_str(), "UNLOCKED");
  }
  else if (msg == "CLOSE" || msg == "LOCK") {

    if (lockState) {
      Serial.println("Already CLOSED — ignoring command");
      return;
    }

    lockState = true;
    deviceState = STATE_DOOR_CLOSED;

    Serial.println("Executing CLOSE...");
    lockServo.write(SERVO_CW);
    delay(SERVO_MOVE_TIME);
    lockServo.write(SERVO_STOP);

    mqttClient.publish(statusTopic.c_str(), "LOCKED");
  }

  else if (msg == "STATUS") {
    mqttClient.publish(statusTopic.c_str(), lockState ? "LOCKED" : "UNLOCKED");
    Serial.print("Published: ");
    Serial.println(lockState ? "LOCKED" : "UNLOCKED");
  }
}

/* ===================== MQTT RECONNECT (UNCHANGED) ===================== */
void reconnectMQTT() {
  if (mqttClient.connected()) return;

  String clientId = makeAPSSID();
  cmdTopic = clientId + "/cmd";
  statusTopic = clientId + "/status";

  Serial.println("========== MQTT ==========");
  Serial.print("Client ID   : "); Serial.println(clientId);
  Serial.print("Subscribing : "); Serial.println(cmdTopic);
  Serial.print("Publishing  : "); Serial.println(statusTopic);

  if (mqttClient.connect(clientId.c_str())) {
    mqttClient.subscribe(cmdTopic.c_str());
    mqttClient.publish(statusTopic.c_str(), "CONNECTED");
    Serial.println("MQTT connected & subscribed");
  } else {
    Serial.print("MQTT connect failed, rc=");
    Serial.println(mqttClient.state());
  }

  Serial.println("==========================");
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
  lockServo.detach();
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

  lockServo.attach(SERVO_PIN);
  lockServo.write(SERVO_CLOSED_ANGLE); // Start locked

  clearAllLEDs();
  deviceState = STATE_BOOTING;

  randomSeed(esp_random());

  if (connectToStoredWiFi(CONNECT_TIMEOUT_MS)) {
    deviceState = STATE_CONNECTED_READY;
    setupWebServer(); 
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);
    reconnectMQTT();
  } else {
    deviceState = STATE_AP_MODE;
    WiFi.mode(WIFI_AP_STA);
    startProvisioningAP();
    setupWebServer();
  }
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
      "<title>AmazonBox Wi-Fi Setup</title>"
      "</head>"
      "<body>"
      "<h2>AmazonBox - Wi-Fi Configuration</h2>"
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

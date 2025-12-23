/*
  AmazonBox - Preferences-only storage template for ESP32
  -------------------------------------------------------
  The difference between Version 2 and Version 3: 
  In this version we are minimising the LED and trying to achieve the same using Two LED i.e Yellow, Green.
  Yellow: If Blink @delay 500, means the product is not connected to the Internet, If it populated the APMode then the Delay will be 200.
  Green: Means connected to the Internet and ready for the Use. This will be same like it is working presently, BUT the Green Light will be Off after getting connected with the Internet.
  It will take Delay of 3 Sec, and will go Off. This means that everything is working fine and now the GREEN is off. If there will be any issues, we already discussed that Yellow will Blink as we discussed. 
  As soon as the Reset Button clicked, it will start Yellow Blink @500, and as soon as it reached to the APMode advertising, it will start @200.  

  The Green_LED will have another set of feature. Instead of BLUE to show if the Door gets message from MQTT to Open or Close, It will Blink @Delay of 1 Sec.
  This means as soon as the Firmware gets any message for Open or Close, the Green LED will go HIGH and as soon as it gets Close, it will be LOW.
  This will let the User know that the Door is Opened Or Closed. 
  Later we will add one Distance Sensor and see if the Door is Opened or Closed. 

  Notes:
   - Preferences namespace: "wifi"
   - Replace or merge your application-specific logic (LEDs, RGB,
     MQTT, Firebase etc.) into the appropriate places.
*/
// Code is running properly without any issue.
// AmazonBox-B77D/cmd 
// OPEN AND CLOSE
// PAssword: setup1234
// NOTE: The Project is working fine. The RESET Button is also working fine. 

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

// ------------------ User-configurable ------------------
#define RESET_BUTTON_PIN 25      // long-press to factory reset (use suitable pin)
#define LONG_PRESS_MS 1000       // 1 seconds
#define AP_PASSWORD "setup1234" // provisioning AP password (or "" for open)
#define CONNECT_TIMEOUT_MS 10000 // timeout while connecting to saved WiFi

// EEPROM sizes (not used) kept for reference: SSID 64, PASS 128

// ------------------ Globals ------------------
Preferences prefs;
const char *PREF_NAMESPACE = "wifi";
const char *KEY_SSID = "ssid";
const char *KEY_PASS = "pass";
const char *KEY_APID = "apid";

WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);
void checkButton();

String currentSSID = "";
String currentPASS = "";

const char* mqtt_server = "broker.hivemq.com";
const uint16_t mqtt_port = 1883;

// LED Pins
#define RED_LED 15
#define YELLOW_LED 4
#define GREEN_LED 16
#define BLUE_LED 2
#define DEBUG_MODE;

#ifdef DEBUG_MODE
  #define DBG(x) Serial.println(x)
  #define DBG2(a,b) { Serial.print(a); Serial.println(b); }
#else
  #define DBG(x)
  #define DBG2(a,b)
#endif


// For OneWire/DS18B20 example (if your project uses it)
#define ONE_WIRE_BUS 27
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

unsigned long pressStart = 0;
unsigned long lastStableChange = 0;

String cmdTopic; 
String statusTopic; 
bool lockState = true;
unsigned long lastServicedMs = 0;
// timing controls
unsigned long lastWifiCheckMs = 0;
const unsigned long WIFI_CHECK_INTERVAL_MS = 5000UL;   // try WiFi every 5s
unsigned long lastMqttAttemptMs = 0;
const unsigned long MQTT_CHECK_INTERVAL_MS = 5000UL;   // try MQTT every 5s


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

// ------------------ WiFi / AP helpers ------------------
String makeAPSSID() {
  return String("AmazonBox-") + getPersistentAPId();
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


//MQTT CALLBACK
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("📩 Message received from broker!");
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
    Serial.print((char)payload[i]);
  }
  Serial.println();
  msg.trim();
  msg.toUpperCase();

  if (msg == "OPEN" || msg == "UNLOCK") {
    blinkBlueAction();
    lockState = false;
    mqttClient.publish(statusTopic.c_str(), "UNLOCKED");
    Serial.println("✅ Box Unlocked");
  } else if (msg == "CLOSE" || msg == "LOCK") {
    blinkBlueAction();
    lockState = true;
    mqttClient.publish(statusTopic.c_str(), "LOCKED");
    Serial.println("🔒 Box Locked");
  } else if (msg == "STATUS") {
    mqttClient.publish(statusTopic.c_str(), lockState ? "LOCKED" : "UNLOCKED");
  } else {
    mqttClient.publish(statusTopic.c_str(), "ERROR: UNKNOWN_CMD");
    Serial.println("⚠️ Unknown command received");
  }
  showConnected();
}

//MQTT RECONNECT
void reconnectMQTT() {
  static unsigned long lastAttempt = 0;
  if (mqttClient.connected()) return;

  if (millis() - lastAttempt < 5000) return;
  lastAttempt = millis();

  String clientId = makeAPSSID();
  cmdTopic = clientId + "/cmd";
  statusTopic = clientId + "/status";

  if (mqttClient.connect(clientId.c_str())) {
    mqttClient.subscribe(cmdTopic.c_str());
    mqttClient.publish(statusTopic.c_str(), "CONNECTED");
    showConnected();
  }
}


// ------------------ setup & loop ------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("In Setup");
  
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  clearAllLEDs();

  showStartup();
  
  // optional: init DS18B20 in your project
  sensors.begin();

  randomSeed(esp_random());

  // Try connect to stored WiFi
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
    mqttClient.setCallback(mqttCallback);
    reconnectMQTT(); // connect and subscribe
  }
  Serial.println("Setup complete.");
}

void loop() {
  // service web server & button frequently
  
  unsigned long now = millis();
  if (now - lastServicedMs >= 2000) {   // every 2s
    lastServicedMs = now;
    //Serial.println("Loop heartbeat - calling server.handleClient()");
  }
  server.handleClient();
  checkButton();


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
    if (!mqttClient.connected() && (now - lastMqttAttemptMs >= MQTT_CHECK_INTERVAL_MS)) {
      lastMqttAttemptMs = now;
      Serial.println("⚠️ MQTT disconnected. Reconnecting...");
      reconnectMQTT();
    }
  } else {
    // wifi not connected — skip MQTT reconnect attempts
    // optionally call mqttClient.disconnect() to cleanup
  }

  // 3) Regular mqtt client processing (non-blocking)
  mqttClient.loop();

  // add any other periodic tasks here (sensors, LEDs, etc.)
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
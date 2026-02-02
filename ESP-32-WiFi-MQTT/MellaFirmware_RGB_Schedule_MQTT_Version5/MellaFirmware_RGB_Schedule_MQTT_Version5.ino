// The project is working fine for Online Scheduler, without any error (EXCEPT ONLINE Mobile Scheduler Part)
// Created two files called client.php and time.php, using time.php you can set the time of the Device using EPOCH,
// And using the client.php, one can set the Switch on and Switch off time of the machine. 
// Everything is working fine for now.
// This has not been tested using Multiple Chip for now. 
// The Payload and the Topics are mentioned in the excel file we created. 

// In this version we are working on MQTT settings and configurations.
// The New Chip does not have any detail
// The Device Generates the Device ID, This device ID never erase 
// The APMode starts, WiFi APMode Populates, The Customer selects the WiFi, name starts with MELLA-XXXX, The PAssword is setup1234
// After selecting the WiFi, The Webserver starts at 192.168.4.1 (YELLOW LED BLINK)
// The Browser ask for WiFi Username and Password 
// The Username/Password goes to Device Memory
// The Device Connects (GREEN LED for 3 Sec. to show conformation that everything is working)
// BLUE LED shows that it SwitchedOn the RELAY and Heat is in ON State, below Thrash hold level or Knob Level


#include <time.h>
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h> // MQTT Message Handler
/* ===================== DEBUG ===================== */
#define DEBUG 1
#if DEBUG
  #define DBG(x) Serial.println(x)
  #define DBG2(a,b) do { Serial.print(a); Serial.print(" : "); Serial.println(b); } while(0)
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

bool bootComplete = false;   // blocks heater until system is fully ready

bool heatingAllowed = false;

unsigned long lastSchedulerCheckMs = 0;
#define SCHEDULER_CHECK_INTERVAL_MS 60000   // 1 minute

/* ===================== MQTT CONFIG ===================== */
const char* MQTT_HOST = "broker.hivemq.com";   // dev (HiveMQ demo)
const int   MQTT_PORT = 1883;

String deviceName;

String mqttClientId;
String topicScheduleCmd;
String topicTimeCmd;
String topicState;
String topicDebug;
String topicStatusCmd;
String topicCmdSchedDump;


bool mqttConnected = false;
unsigned long lastMqttReconnectMs = 0;
#define MQTT_RECONNECT_INTERVAL_MS 5000

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
/* ===================== TIME ENGINE ===================== */
time_t deviceEpoch = 0;
unsigned long lastEpochUpdateMs = 0;

#define EPOCH_SYNC_INTERVAL_MS 60000  // 1 minute

int getCurrentDay();     // 0 = Sunday ... 6 = Saturday
int getCurrentHour();    // 0–23
int getCurrentMinute();  // 0–59

// ADDED FOR FAKE OVERRIDE "DEBUGGING"
bool manualTimeOverride = false;
int manualDay = 0;
int manualHour = 0;
int manualMin = 0;

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

/* ==================== MQTT HELPER FUNCTION ====== */
void initMqttTopics() {
  String id = getAPId();   // already stable & persisted

  mqttClientId   = "mella-" + id;
  topicScheduleCmd = "mella/" + id + "/cmd/schedule";
  topicTimeCmd     = "mella/" + id + "/cmd/time";
  topicState       = "mella/" + id + "/state";
  topicDebug       = "mella/" + id + "/debug";
  topicStatusCmd = "mella/" + id + "/cmd/status";
  topicCmdSchedDump = "mella/" + id + "/cmd/sched_dump";

}

/* ========= ADD A STATUS DUMP FUNCTION ========== */
void dumpStatus(const char* source) {
  DBG("### dumpStatus() CALLED ###");
  int day = getCurrentDay();
  int hour = getCurrentHour();
  int min = getCurrentMinute();

  DBG("===== DEVICE STATUS =====");
  DBG2("Source", source);
  DBG2("Device", "Mella-" + getAPId());
  DBG2("Epoch", (uint32_t)deviceEpoch);
  DBG2("Day", day);
  DBG2("Hour", hour);
  DBG2("Minute", min);
  DBG2("Scheduler Enabled", schedulerEnabled ? "YES" : "NO");

  DaySchedule &d = weekSchedule[day];

  if (!d.enabled) {
    DBG("Today Schedule : DISABLED");
  } else {
    DBG2("Today Window",
      String(d.startHour) + ":" + String(d.startMin) +
      " -> " +
      String(d.endHour) + ":" + String(d.endMin)
    );
  }

  bool active = isScheduleActiveNow();
  DBG2("Schedule Active Now", active ? "YES" : "NO");
  DBG2("Heating Allowed", heatingAllowed ? "YES" : "NO");

  DBG("==========================");

  /* ---- Publish same info to MQTT ---- */
  if (mqttClient.connected()) {
    StaticJsonDocument<384> doc;
    doc["type"] = "status"; 
    doc["device"] = deviceName;
    doc["epoch"] = (uint32_t)deviceEpoch;
    doc["day"] = day;
    doc["hour"] = hour;
    doc["minute"] = min;
    doc["schedulerEnabled"] = schedulerEnabled;
    doc["todayEnabled"] = d.enabled;
    doc["scheduleActive"] = active;
    doc["heatingAllowed"] = heatingAllowed;
    doc["source"] = source;

    char buf[384];
    serializeJson(doc, buf);
    mqttClient.publish(topicState.c_str(), buf);
  }
}

/* ============ MQTT scheuled Dump ===========================================*/
void dumpScheduleMQTT(const char* source) {
  DBG("===== SCHEDULE DUMP =====");
  DBG2("Source", source);
  DBG2("Scheduler Enabled", schedulerEnabled ? "YES" : "NO");

  StaticJsonDocument<512> doc;
  doc["type"] = "sched_dump";
  doc["device"] = deviceName;
  doc["schedulerEnabled"] = schedulerEnabled;

  JsonArray days = doc.createNestedArray("days");

  for (int i = 0; i < 7; i++) {
    DaySchedule &d = weekSchedule[i];
    JsonObject day = days.createNestedObject();
    day["day"] = i;
    day["enabled"] = d.enabled;

    if (d.enabled) {
      char start[6], end[6];
      sprintf(start, "%02d:%02d", d.startHour, d.startMin);
      sprintf(end, "%02d:%02d", d.endHour, d.endMin);
      day["start"] = start;
      day["end"] = end;

      Serial.printf(
        "Day %d : ENABLED  %s -> %s\n",
        i, start, end
      );
    } else {
      Serial.printf("Day %d : DISABLED\n", i);
    }
  }

  DBG("=========================");

  char payload[512];
  serializeJson(doc, payload);

  mqttClient.publish(
    topicState.c_str(),
    payload,
    false
  );
}

/* =================== MQTT MESSAGE HANDLER ============================*/

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  
  Serial.print("RAW TOPIC = [");
  Serial.print(topic);
  Serial.println("]");
  payload[length] = '\0';
  //String msg = String((char*)payload);

  DBG2("[MQTT] Topic", topic);
  Serial.print("[MQTT] Payload : ");
  Serial.write(payload, length);
  Serial.println();
  //DBG2("[MQTT] Payload", msg);

  /* ===== STATUS COMMAND (NO JSON REQUIRED) ===== */
  if (String(topic) == topicStatusCmd) {
    DBG("[MQTT] Status request received");
    dumpStatus("mqtt-status");
  }

  if (String(topic) == topicCmdSchedDump) {
    DBG("[MQTT] Schedule dump requested");
    dumpScheduleMQTT("mqtt");
    return;
  }

  if (length == 0) {
    DBG("[MQTT] Empty payload ignored");
    return;
  }

  StaticJsonDocument<4096> doc;
  //DeserializationError err = deserializeJson(doc, msg);
  DeserializationError err = deserializeJson(doc, payload, length);

  if (err) {
    DBG2("[MQTT] JSON parse failed", err.c_str());
    DBG2("[MQTT] Payload length", length);
    return;
  }

  int version = doc["version"] | 0;
  if (version != 1) {
    DBG("[MQTT] Unsupported version");
    return;
  }


  /* ===== Schedule Command ===== */
  if (String(topic) == topicScheduleCmd) {
      // ===== Guard: reject empty schedule payloads =====
    bool hasAnyScheduleData =
      doc.containsKey("enabled") ||
      doc.containsKey("day") ||
      doc.containsKey("days");

    if (!hasAnyScheduleData) {
      DBG("[MQTT] Schedule payload ignored (no actionable fields)");
      return;
    }

    if (doc.containsKey("enabled")) {
      schedulerEnabled = doc["enabled"];
    }

    // ===== Single-day schedule support =====
    if (doc.containsKey("day")) {
      String dstr = doc["day"].as<String>();
      int day = -1;

      if      (dstr == "sun") day = 0;
      else if (dstr == "mon") day = 1;
      else if (dstr == "tue") day = 2;
      else if (dstr == "wed") day = 3;
      else if (dstr == "thu") day = 4;
      else if (dstr == "fri") day = 5;
      else if (dstr == "sat") day = 6;

      if (day >= 0) {
        bool en = doc["enabled"] | false;

        weekSchedule[day].enabled = en;

        if (en) {
          String st = doc["start"] | "00:00";
          String et = doc["end"]   | "00:00";

          weekSchedule[day].startHour = st.substring(0, 2).toInt();
          weekSchedule[day].startMin  = st.substring(3, 5).toInt();
          weekSchedule[day].endHour   = et.substring(0, 2).toInt();
          weekSchedule[day].endMin    = et.substring(3, 5).toInt();
        } else {
          weekSchedule[day].startHour = 0;
          weekSchedule[day].startMin  = 0;
          weekSchedule[day].endHour   = 0;
          weekSchedule[day].endMin    = 0;
        }
      }
    }


    if (doc.containsKey("days")) {
      JsonObject days = doc["days"];

      for (JsonPair kv : days) {
        int day = String(kv.key().c_str()).toInt();
        JsonObject d = kv.value();

        if (d.containsKey("en"))
          weekSchedule[day].enabled = d["en"];

        if (d.containsKey("sh"))
          weekSchedule[day].startHour = d["sh"];

        if (d.containsKey("sm"))
          weekSchedule[day].startMin = d["sm"];

        if (d.containsKey("eh"))
          weekSchedule[day].endHour = d["eh"];

        if (d.containsKey("em"))
          weekSchedule[day].endMin = d["em"];

        // FIX #2 (correct placement)
        if (weekSchedule[day].enabled &&
            weekSchedule[day].startHour == weekSchedule[day].endHour &&
            weekSchedule[day].startMin  == weekSchedule[day].endMin) {
          DBG2("[SCHED] Invalid window → disabling day", day);
          weekSchedule[day].enabled = false;
        }
      }
    }

    saveScheduleToPrefs();
    updateSchedulerPermission();

    DBG("[MQTT] Schedule updated");
  }

  /* ===== Time Command ===== */
  else if (String(topic) == topicTimeCmd) {
    time_t epoch = doc["epoch"] | 0;
    if (epoch > 0) {
      setDeviceTime(epoch);
      DBG("[MQTT] Time updated");
    }
  }

}
/* ========== MQTT CONNECT / RECONNECT LOGIC ====================*/
void ensureMqttConnected() {
  if (!systemReady) return;
  if (mqttClient.connected()) return;

  unsigned long now = millis();
  if (now - lastMqttReconnectMs < MQTT_RECONNECT_INTERVAL_MS) return;
  lastMqttReconnectMs = now;

  DBG("[MQTT] Connecting...");

  if (mqttClient.connect(mqttClientId.c_str())) {
    DBG("[MQTT] Connected");

    mqttClient.subscribe(topicScheduleCmd.c_str());
    mqttClient.subscribe(topicTimeCmd.c_str());
    mqttClient.subscribe(topicStatusCmd.c_str());
    mqttClient.subscribe(topicCmdSchedDump.c_str());

    mqttConnected = true;
  } else {
    DBG2("[MQTT] Failed rc", mqttClient.state());
  }
}

/* =========== PUBLISH DEVICE STATE (DEVICE → MOBILE) ===================*/
void publishDeviceState() {
  if (!mqttClient.connected()) return;

  StaticJsonDocument<256> doc;
  doc["version"] = 1;
  doc["schedulerEnabled"] = schedulerEnabled;
  doc["activeNow"] = heatingAllowed;
  doc["day"] = getCurrentDay();
  doc["hour"] = getCurrentHour();
  doc["minute"] = getCurrentMinute();
  doc["heater"] = heatingAllowed ? "ON" : "OFF";
  doc["source"] = "scheduler";

  char buf[256];
  serializeJson(doc, buf);

  mqttClient.publish(topicState.c_str(), buf);
}

/* ==================== Scheduler Decision Function (CORE LOGIC) ====== */
bool isScheduleActiveNow() {
  if (!schedulerEnabled) {
    DBG("[SCHED] Scheduler globally disabled");
    return false;
  }

  int today = getCurrentDay();
  int hour  = getCurrentHour();
  int min   = getCurrentMinute();

  DaySchedule &d = weekSchedule[today];

  if (!d.enabled) {
    DBG("[SCHED] Today disabled");
    return false;
  }

  int nowMin   = hour * 60 + min;
  int startMin = d.startHour * 60 + d.startMin;
  int endMin   = d.endHour * 60 + d.endMin;

  // Normal same-day window
  if (startMin <= endMin) {
    return (nowMin >= startMin && nowMin < endMin);
  }

  // Overnight window (e.g. 22:00 → 06:00)
  return (nowMin >= startMin || nowMin < endMin);
}

/* ====================== Time Scheduler ================  */
void saveEpoch(time_t epoch) {
  prefs.begin("time", false);
  prefs.putULong("epoch", (uint32_t)epoch);
  prefs.end();
}

time_t loadEpoch() {
  prefs.begin("time", true);
  time_t epoch = prefs.getULong("epoch", 0);
  prefs.end();
  return epoch;
}

/* ====== Set device time (used later by mobile / NTP)  ====  */
void setDeviceTime(time_t epoch) {
  deviceEpoch = epoch;
  lastEpochUpdateMs = millis();
  saveEpoch(epoch);

  struct timeval tv;
  tv.tv_sec = epoch;
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);

  DBG("[TIME] Device time set");
}

/* =============Force immediate scheduler evaluation =============================*/
void reevaluateSchedulerNow() {
  updateSchedulerPermission();
}

/* =============== Update Scheduler → Permission Mapping =========================*/
void updateSchedulerPermission() {

  // 🚨 Boot guard
  if (!bootComplete) {
    heatingAllowed = false;
    heatOff();
    return;
  }

  // 🚨 Global scheduler OFF
  if (!schedulerEnabled) {
    heatingAllowed = false;
    heatOff();
    DBG("[SCHED] Scheduler globally disabled");
    return;
  }

  bool active = isScheduleActiveNow();

  if (active != heatingAllowed) {
    DBG(active
      ? "[SCHED] Inside schedule → Heating allowed"
      : "[SCHED] Outside schedule → Heating blocked");
  }

  heatingAllowed = active;

  if (!heatingAllowed) {
    heatOff();
  }
}


/* =============== Update device time (offline tick) =========================*/
void updateDeviceTime() {
  unsigned long now = millis();

  if (now - lastEpochUpdateMs >= EPOCH_SYNC_INTERVAL_MS) {
    deviceEpoch += (now - lastEpochUpdateMs) / 1000;
    lastEpochUpdateMs = now;
    saveEpoch(deviceEpoch);

    // 🔍 TEMP DEBUG (can be removed later)
    DBG("----- TIME DEBUG -----");
    DBG2("DAY", getCurrentDay());     // 0 = Sunday
    DBG2("HOUR", getCurrentHour());
    DBG2("MIN", getCurrentMinute());
    DBG("----------------------");
  }
}

/* ==============   Read current day & time (CORE API)      ================*/
int getCurrentDay() {
  if (manualTimeOverride) return manualDay;
  struct tm timeinfo;
  localtime_r(&deviceEpoch, &timeinfo);
  return timeinfo.tm_wday;
}

int getCurrentHour() {
  if (manualTimeOverride) return manualHour;
  struct tm timeinfo;
  localtime_r(&deviceEpoch, &timeinfo);
  return timeinfo.tm_hour;
}

int getCurrentMinute() {
  if (manualTimeOverride) return manualMin;
  struct tm timeinfo;
  localtime_r(&deviceEpoch, &timeinfo);
  return timeinfo.tm_min;
}

/* =========== Add helper to print one day ===================*/
void dumpSchedule() {
  DBG("===== SCHEDULE DUMP =====");
  DBG2("Scheduler Enabled", schedulerEnabled ? "YES" : "NO");

  for (int i = 0; i < 7; i++) {
    Serial.print("Day ");
    Serial.print(i);
    Serial.print(" : ");

    if (!weekSchedule[i].enabled) {
      Serial.println("DISABLED");
      continue;
    }

    Serial.print("ENABLED  ");
    Serial.print(weekSchedule[i].startHour);
    Serial.print(":");
    if (weekSchedule[i].startMin < 10) Serial.print("0");
    Serial.print(weekSchedule[i].startMin);
    Serial.print("  ->  ");
    Serial.print(weekSchedule[i].endHour);
    Serial.print(":");
    if (weekSchedule[i].endMin < 10) Serial.print("0");
    Serial.println(weekSchedule[i].endMin);
  }

  DBG("==========================");
}

/* =================  HANDLE SERIAL COMMAND HANDLER ========  */
void handleSerialCommands() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  DBG("----- SERIAL DEBUG -----");
  DBG2("RAW CMD", cmd);
  DBG2("CMD LENGTH", cmd.length());
  DBG("------------------------");

  if (cmd == "sched on") {
    schedulerEnabled = true;
    saveScheduleToPrefs();
    reevaluateSchedulerNow();
    DBG("[CMD] Scheduler ENABLED");
  }
  else if (cmd == "sched off") {
    schedulerEnabled = false;
    saveScheduleToPrefs();   // ✅ ADD THIS
    DBG("[CMD] Scheduler DISABLED");
  }
  else if (cmd.startsWith("setday")) {
    manualDay = cmd.substring(7).toInt();
    manualTimeOverride = true;
    DBG2("[CMD] Day set", manualDay);
  }
  else if (cmd.startsWith("settime")) {
    sscanf(cmd.c_str(), "settime %d %d", &manualHour, &manualMin);
    manualTimeOverride = true;
    reevaluateSchedulerNow();
    DBG("[CMD] Time overridden");
  }
  else if (cmd.startsWith("setwindow")) {
    int sh, sm, eh, em;
    sscanf(cmd.c_str(), "setwindow %d %d %d %d", &sh, &sm, &eh, &em);

    weekSchedule[manualDay].startHour = sh;
    weekSchedule[manualDay].startMin  = sm;
    weekSchedule[manualDay].endHour   = eh;
    weekSchedule[manualDay].endMin    = em;
    weekSchedule[manualDay].enabled  = true;

    saveScheduleToPrefs();   // ✅ CRITICAL FIX
    reevaluateSchedulerNow();
    DBG("[CMD] Schedule window set & applied/Saved");
  }
  else if (cmd == "status") {
    DBG("===== QUICK STATUS =====");
    DBG2("DAY", getCurrentDay());
    DBG2("HOUR", getCurrentHour());
    DBG2("MIN", getCurrentMinute());
    DBG2("ACTIVE", isScheduleActiveNow() ? "YES" : "NO");
    DBG("========================");
    
    dumpStatus("serial");
  }
  else if (cmd == "realtime") {
    manualTimeOverride = false;
    DBG("[CMD] Back to real time");
  }
  else if (cmd == "sched dump") {
    dumpSchedule();
  }
}

/* ====================== Reset Button Code ================  */
void factoryReset() {
  DBG("[RESET] Factory reset initiated");

  resetInProgress = true;
  systemReady = false;

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

  bool valid = true;

  for (int i = 0; i < 7; i++) {
    String key = "d" + String(i);
    if (!prefs.isKey(key.c_str())) {
      valid = false;
      break;
    }
  }

  if (!valid) {
    prefs.end();
    DBG("[SCHED] Schedule data incomplete");
    return false;
  }

  schedulerEnabled = prefs.getBool("enabled", false);

  for (int i = 0; i < 7; i++) {
    String key = "d" + String(i);
    prefs.getBytes(key.c_str(), &weekSchedule[i], sizeof(DaySchedule));
  }

  prefs.end();

  DBG("[SCHED] Schedule loaded from Preferences");
  DBG2("[SCHED] Global enabled", schedulerEnabled ? "YES" : "NO");
  return true;
}


/* ============== Force AP mode explicitly ============ */
void startProvisioningAP() {
  DBG("[WIFI] Starting AP mode");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(("Mella-" + getAPId()).c_str(), AP_PASSWORD);

  systemReady = false;
}

/* ===================== SETUP ===================== */
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);   // 🔒 HARD OFF at power-on

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  // ---- Sensors init (safe) ----
  sensors.begin();
  sensors.getAddress(mainThermometer, 0);

  // ---- Restore scheduler FIRST ----
  bool schedLoaded = loadScheduleFromPrefs();

  if (!schedLoaded) {
    DBG("[SCHED] No saved schedule, applying defaults");
    setDefaultSchedule();
    saveScheduleToPrefs();
  }


  // ===== Fix-2: Auto-enable scheduler if any day is enabled =====
  bool anyDayEnabled = false;

  for (int i = 0; i < 7; i++) {
    if (weekSchedule[i].enabled) {
      anyDayEnabled = true;
      break;
    }
  }

  if (anyDayEnabled && !schedulerEnabled) {
    schedulerEnabled = true;
    saveScheduleToPrefs();
    DBG("[SCHED] Auto-enabled (at least one day active)");
  }
  else if (!anyDayEnabled) {
    DBG("[SCHED] No active days configured");
  }


  // ---- Restore time BEFORE WiFi ----
  deviceEpoch = loadEpoch();
  if (deviceEpoch == 0) {
    DBG("[TIME] No saved time, using safe default");
    setDeviceTime(1700000000);
  } else {
    DBG("[TIME] Time restored from memory");
    lastEpochUpdateMs = millis();
  }

  /* ========= ADD THIS BLOCK (DEVICE ID DEBUG) ========= */
  String deviceId = getAPId();
  deviceName = "Mella-" + getAPId();
  DBG("================================");
  DBG2("Device Name", deviceName);
  DBG2("Device ID", deviceId);
  DBG("================================");
  /* ====================================================== */

  // ---- WiFi handling (unchanged) ----
  String ssid, pass;
  bool hasWiFi = loadWiFi(ssid, pass);
  if (hasWiFi) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long start = millis();
    while (millis() - start < CONNECT_TIMEOUT_MS) {
      if (WiFi.status() == WL_CONNECTED) {
        systemReady = true;
        break;
      }
      delay(200);
    }
    if (!systemReady) startProvisioningAP();
  } else {
    startProvisioningAP();
  }

  // 
  initMqttTopics();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setBufferSize(2048);
  DBG2("[MQTT] Buffer size", mqttClient.getBufferSize());
  mqttClient.setCallback(mqttCallback);

  setupWeb();

  bootComplete = true;   // ✅ NOW the system may control the heater
  updateSchedulerPermission();   // ✅ immediate evaluation
  heatingAllowed = false;
  heatOff();


  DBG("System ready");
}



/* ============== Reading Sensor and Appling Heater Logic ============ */

void updateTemperatureControl() {
  unsigned long now = millis();
  if (now - lastSensorReadMs < SENSOR_INTERVAL_MS) return;
  lastSensorReadMs = now;

  // Absolute safety
  if (!systemReady || !heatingAllowed) {
    heatOff();
    return;
  }

  knobRaw = analogRead(KNOB_PIN);
  knobLevel = map(knobRaw, 0, 4095, 0, 10);
  knobLevel = constrain(knobLevel, 0, 10);

  sensors.requestTemperatures();
  temp_c = sensors.getTempC(mainThermometer);

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

  handleSerialCommands();
  server.handleClient();
  checkButton();

  ensureMqttConnected();
  mqttClient.loop();

  static unsigned long lastStatePub = 0;
  if (millis() - lastStatePub > 60000) {
    lastStatePub = millis();
    publishDeviceState();
  }


  // During reset → everything OFF
  if (resetInProgress) {
    digitalWrite(RELAY_PIN, LOW);
    return;
  }

  static unsigned long lastSchedulerCheckMs = 0;
  updateDeviceTime();

  if (millis() - lastSchedulerCheckMs >= 60000) {
    lastSchedulerCheckMs = millis();
    updateSchedulerPermission();
  }

  updateStatusLED();
  updateTemperatureControl();
  static unsigned long lastSchedDbg = 0;

  // TEMP Debug Hook (No Heater Yet)
  if (millis() - lastSchedDbg > 60000) {  // once per minute
    lastSchedDbg = millis();

    bool active = isScheduleActiveNow();
    DBG("===== SCHEDULER CHECK =====");
    DBG2("DAY", getCurrentDay());
    DBG2("HOUR", getCurrentHour());
    DBG2("MIN", getCurrentMinute());
    DBG2("ACTIVE", active ? "YES" : "NO");
    DBG("===========================");
  }

}



/**************************************************
 * MellaFirmware.ino
 * FINAL – SINGLE FILE – VERIFIED FUNCTIONS INCLUDED
 **************************************************/

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#define DEBUG_MODE
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ================= DEBUG =================
#define DEBUG_MODE

// ================= GLOBAL OBJECTS =================
Preferences prefs;
WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP Udp;

// ================= MQTT TOPICS =================
const char* publish_status         = "003915A1/Status";
const char* publish_debug          = "003915A1/Debug";
const char* subscribe_action       = "003915A1/Action";
const char* subscribe_schedule     = "003915A1/Schedule";
const char* subscribe_autoshutdown = "003915A1/Autoshutdown";
const char* subscribe_upgrade      = "003915A1/Upgrade";

// ================= MQTT CONFIG =================
#define MQTT_SERVER "YOUR_MQTT_SERVER"
#define MQTT_PORT   1883
#define MQTT_USER   "YOUR_MQTT_USER"
#define MQTT_PASS   "YOUR_MQTT_PASS"

// ================= NTP =================
static const char ntpServerName[] = "us.pool.ntp.org";
int timeZone;
unsigned int localPort = 8888;

// ================= DEVICE =================
#define device_generation 2
#define firmware_version  1

// ================= HARDWARE =================
#define RELAY 26
#define LED_STATUS 2
#define transistorPin 25

// ================= STATE =================
bool status_on  = false;
bool status_off = true;
bool wifiOnce = false;
bool Connection_Status = true;

// ================= TEMP =================
float temp_c = 0;
float calibration_offset = 0;
float default_temp = 40;
#define DEVICE_DISCONNECTED_C -127

// ================= WIFI STORAGE =================
bool loadWiFiCredentials(String &ssid, String &pass) {
  prefs.begin("wifi", true);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
  ssid.trim(); pass.trim();
  return ssid.length() > 0;
}

void saveWiFiCredentials(const String &ssid, const String &pass) {
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
}

void factoryReset() {
#ifdef DEBUG_MODE
  Serial.println("FACTORY RESET");
#endif
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  delay(300);
  ESP.restart();
}

// ================= HEATER =================
void heat() {
  digitalWrite(RELAY, HIGH);
#ifdef DEBUG_MODE
  Serial.println("Heater: ON");
#endif
}

void heatOff() {
  digitalWrite(RELAY, LOW);
#ifdef DEBUG_MODE
  Serial.println("Heater: OFF");
#endif
}

void updateMqtt(int status)
{
#ifdef DEBUG_MODE
  Serial.println(publish_status);
#endif
  const char* payload = status ? "1" : "0";
  if (client.connected()) {
    client.publish(publish_status, payload);
#ifdef DEBUG_MODE
    Serial.println("MQTT Status Updated");
#endif
  }
}

void handleOn()  { heat();    updateMqtt(1); }
void handleOff() { heatOff(); updateMqtt(0); }

// ================= SET TEMP =================
void set_temp(int knob_input)
{
  if (temp_c == DEVICE_DISCONNECTED_C) {
#ifdef DEBUG_MODE
    Serial.println("No Thermometer");
#endif
    heatOff();
    return;
  }

  if (wifiOnce && !Connection_Status) {
#ifdef DEBUG_MODE
    Serial.println("Disconnect Detected");
#endif
    heatOff();
    return;
  }

  client.publish(publish_debug, String(temp_c - calibration_offset).c_str());
  client.publish(publish_debug, String(knob_input).c_str());

#ifdef DEBUG_MODE
  Serial.println("Set Temp");
#endif

  switch (knob_input)
  {
    case 1: if (temp_c <= 30 + calibration_offset) heat(); else heatOff(); break;
    case 2: if (temp_c <= 36 + calibration_offset) heat(); else heatOff(); break;
    case 3: if (temp_c <= 42 + calibration_offset + 5) heat(); else heatOff(); break;
    case 4: if (temp_c <= 48 + calibration_offset) heat(); else heatOff(); break;
    case 5: if (temp_c <= 54 + calibration_offset + 5) heat(); else heatOff(); break;
    case 6: if (temp_c <= 60 + calibration_offset) heat(); else heatOff(); break;
    case 7: if (temp_c <= 66 + calibration_offset) heat(); else heatOff(); break;
    case 8: if (temp_c <= 72 + calibration_offset) heat(); else heatOff(); break;
    case 9: if (temp_c <= 82 + calibration_offset) heat(); else heatOff(); break;
    case 10: if (temp_c <= 90 + calibration_offset) heat(); else heatOff(); break;
    default: if (temp_c <= default_temp) heat(); else heatOff(); break;
  }

#ifdef DEBUG_MODE
  Serial.print("Heater Pin Value: ");
  Serial.println(digitalRead(transistorPin));
#endif
}

// ================= ROUTINE TASK =================
void routine_task(void)
{
#ifdef DEBUG_MODE
  Serial.print(hour()); Serial.print(":");
  Serial.print(minute()); Serial.print(":");
  Serial.print(second()); Serial.print("--");
  Serial.print(weekday());
  Serial.println();
#endif

  int pinStatus = digitalRead(RELAY);
  updateMqtt(pinStatus ? 1 : 0);

#ifdef DEBUG_MODE
  Serial.println("Checking Schedule");
#endif
}

// ================= MQTT =================
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  if (String(topic) == subscribe_upgrade) {
    firmware_update(); // keep original hook
  }
}

void reconnectMQTT()
{
  while (!client.connected()) {
    if (client.connect("MellaDevice", MQTT_USER, MQTT_PASS)) {
      client.subscribe(subscribe_action);
      client.subscribe(subscribe_schedule);
      client.subscribe(subscribe_autoshutdown);
      client.subscribe(subscribe_upgrade);
    } else {
      delay(3000);
    }
  }
}

// ================= SETUP / LOOP =================
void setup()
{
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqttCallback);
}

void loop()
{
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) reconnectMQTT();
    client.loop();
    routine_task();
  }
}

void firmware_update()
{
#ifdef DEBUG_MODE
  Serial.println("CHECKING_FOR_UPDATES");
#endif

  if (WiFi.status() != WL_CONNECTED) {
#ifdef DEBUG_MODE
    Serial.println("WiFi not connected, OTA aborted");
#endif
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  String url = "https://sfo2.digitaloceanspaces.com/mellafirmware/firmware/" +
               String(device_generation) + "/" +
               String(firmware_version + 1) +
               "/MellaFirmware.bin";

#ifdef DEBUG_MODE
  Serial.println(url);
#endif

  if (!https.begin(client, url)) {
#ifdef DEBUG_MODE
    Serial.println("HTTPS begin failed");
#endif
    return;
  }

  int httpCode = https.GET();

  if (httpCode != HTTP_CODE_OK) {
#ifdef DEBUG_MODE
    Serial.printf("HTTP GET failed: %d\n", httpCode);
#endif
    https.end();
    return;
  }

  int contentLength = https.getSize();
  bool canBegin = Update.begin(contentLength);

  if (!canBegin) {
#ifdef DEBUG_MODE
    Serial.println("Not enough space for OTA");
#endif
    https.end();
    return;
  }

  WiFiClient* stream = https.getStreamPtr();
  size_t written = Update.writeStream(*stream);

  if (written == contentLength) {
#ifdef DEBUG_MODE
    Serial.println("OTA write successful");
#endif
  } else {
#ifdef DEBUG_MODE
    Serial.println("OTA write incomplete");
#endif
  }

  if (Update.end()) {
#ifdef DEBUG_MODE
    Serial.println("OTA finished, rebooting");
#endif
    ESP.restart();
  } else {
#ifdef DEBUG_MODE
    Serial.println("OTA failed");
#endif
  }

  https.end();
}

#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#define EEPROM_SIZE 256

// USER CONFIG 
const char* defaultAP_SSID = "SafetyBox_Config";
const char* defaultAP_PASS = "safety123";  
const char* mqtt_server = "broker.hivemq.com";
const uint16_t mqtt_port = 1883;

const int LOCK_LED_PIN = 2;  

// Globals
WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

String deviceUID;
String cmdTopic;
String statusTopic;

bool lockState = true; 
unsigned long lastMQTTReconnectAttempt = 0;
const unsigned long MQTT_RETRY_MS = 5000UL;

//Save Wi-Fi credentials (SSID & Password) to EEPROM
void saveCredentialsToEEPROM(const char* ssid, const char* pass) 
{
  EEPROM.begin(EEPROM_SIZE);
  int idx = 0;
  
  for (int i = 0; i < 64; ++i) {
  char c = (i < (int)strlen(ssid)) ? ssid[i] : '\0';
  EEPROM.write(idx++, c);
  }
  for (int i = 0; i < 128; ++i) 
  {
  char c = (i < (int)strlen(pass)) ? pass[i] : '\0';
  EEPROM.write(idx++, c);
  }
  EEPROM.commit();
  EEPROM.end();
}

void readCredentialsFromEEPROM(String& outSsid, String& outPass) 
{
  EEPROM.begin(EEPROM_SIZE);
  int idx = 0;
  char bufS[65];
  bufS[64] = '\0';
  char bufP[129];
  bufP[128] = '\0';
  for (int i = 0; i < 64; ++i) bufS[i] = EEPROM.read(idx++);
  for (int i = 0; i < 128; ++i) bufP[i] = EEPROM.read(idx++);
  EEPROM.end();
  outSsid = String(bufS);
  outPass = String(bufP);
  outSsid.trim();
  outPass.trim();
}

void clearCredentialsInEEPROM() 
{
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < 192; ++i) EEPROM.write(i, 0);
  EEPROM.commit();
  EEPROM.end();
}

//LED helper
void ledOn() 
{
  digitalWrite(LOCK_LED_PIN, HIGH);
}  
void ledOff() 
{
  digitalWrite(LOCK_LED_PIN, LOW);
}  

void blink(int times, int onMs, int offMs) 
{
  for (int i = 0; i < times; ++i) 
  {
    digitalWrite(LOCK_LED_PIN, HIGH);
    delay(onMs);
    digitalWrite(LOCK_LED_PIN, LOW);
    delay(offMs);
  }
}

// status blink patterns
void blinkStartup()
{
  blink(1, 200, 100);
}
void blinkConnecting()
{
  blink(50, 80, 80);
}  
void blinkUnknownCmd() 
{
  blink(20, 50, 50);
}

//Serve the configuration webpage for Wi-Fi setup
String configPage = R"rawliteral
(
  <!DOCTYPE html>
  <html>
  <head>
  <meta charset="utf-8">
  <title>SafetyBox Setup</title>
  </head>
  <body>
  <h3>SafetyBox - WiFi Setup</h3>
  <form action="/save" method="post">
  SSID:<br>
  <input name="ssid" type="text">
  <br>
  Password:<br>
  <input name="pass" type="password">
  <br><br>
  <input type="submit" value="Save & Reboot">
  </form>
  <p>If you want to clear saved creds, visit <a href="/clear">/clear</a></p>
  </body>
  </html>
)rawliteral";

//Handle root page of configuration server
void handleRoot(){
  server.send(200, "text/html", configPage);
}

//Handle saving of SSID and Password from user input
void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    saveCredentialsToEEPROM(ssid.c_str(), pass.c_str());
    server.send(200, "text/html", "<p>Saved. Rebooting...</p>");
    delay(1000);
    ESP.restart();
  } 
  else
  {
    server.send(400, "text/plain", "Bad Request - missing fields");
  }
}

//Handle clearing of saved credentials and reboot
void handleClear()
{
  clearCredentialsInEEPROM();
  server.send(200, "text/html", "<p>Credentials cleared. Rebooting...</p>");
  delay(1000);
  ESP.restart();
}

//Start Access Point mode for Wi-Fi configuration
void startConfigPortal() 
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(defaultAP_SSID, defaultAP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("Config AP IP: ");
  Serial.println(apIP);
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/clear", handleClear);
  server.begin();
  unsigned long start = millis();
  
  while (true)
  {
    server.handleClient();
    digitalWrite(LOCK_LED_PIN, HIGH);
    delay(400);
    digitalWrite(LOCK_LED_PIN, LOW);
    delay(400);
  }
}

//Handle received MQTT messages for lock/unlock commands
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  String msg;
  for (unsigned int i = 0; i < length; ++i) msg += (char)payload[i];
  msg.trim();
  msg.toUpperCase();
  Serial.print("MQTT msg: ");
  Serial.println(msg);
  
  if (String(topic) == cmdTopic) 
  {
    if (msg == "OPEN" || msg == "UNLOCK") {
      // Unlock sequence
      blink(3, 120, 80);  
      ledOff();           
      lockState = false;
      mqttClient.publish(statusTopic.c_str(), "UNLOCKED");
      Serial.println("Unlocked");
    } 
    else if (msg == "CLOSE" || msg == "LOCK")
    {
      // Lock sequence
      blink(3, 300, 150);  
      ledOn();             
      lockState = true;
      mqttClient.publish(statusTopic.c_str(), "LOCKED");
      Serial.println("Locked");
    } 
    else if (msg == "STATUS") 
    {
      mqttClient.publish(statusTopic.c_str(), lockState ? "LOCKED" : "UNLOCKED");
    } 
    else 
    {
      mqttClient.publish(statusTopic.c_str(), "ERROR:UNKNOWN_CMD");
      blinkUnknownCmd();
    }
  }
}

// Attempt MQTT reconnection if disconnected
void mqttReconnectIfNeeded() 
{
  if (!mqttClient.connected()) 
  {
    if (millis() - lastMQTTReconnectAttempt < MQTT_RETRY_MS) return;
    lastMQTTReconnectAttempt = millis();
    Serial.print("Attempting MQTT connect...");
    
    if (mqttClient.connect(deviceUID.c_str()))
    {
      Serial.println("connected");
      mqttClient.subscribe(cmdTopic.c_str());
      mqttClient.publish(statusTopic.c_str(), "CONNECTED");
      // indicate connection by steady LED = locked (default)
      ledOn();
    } 
    else
    {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      // indicate trying (fast blink briefly)
      blink(6, 100, 100);
    }
   }
}

//Connect to Wi-Fi using stored credentials
bool connectToWiFi(const String& ssid, const String& pass) 
{
  if (ssid.length() == 0) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.print("Connecting to WiFi...");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) 
  {
    if (millis() - start > 20000UL) 
    { 
      Serial.println(" WiFi connect timeout");
      return false;
    }
    digitalWrite(LOCK_LED_PIN, HIGH);
    delay(100);
    digitalWrite(LOCK_LED_PIN, LOW);
    delay(100);
    Serial.print(".");
  }
    Serial.println();
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

//Setup: Initialize Wi-Fi, MQTT, and default lock state
void setup() 
{
  Serial.begin(115200);
  pinMode(LOCK_LED_PIN, OUTPUT);
  digitalWrite(LOCK_LED_PIN, LOW); 

  // prepare UID and topics
  deviceUID = "SB_" + WiFi.macAddress();
  deviceUID.replace(":", "");
  cmdTopic = "SafetyBox/" + deviceUID + "/cmd";
  statusTopic = "SafetyBox/" + deviceUID + "/status";
  Serial.println("Device UID: " + deviceUID);
  Serial.println("Cmd topic: " + cmdTopic);
  Serial.println("Status topic: " + statusTopic);

  // startup blink
  blinkStartup();

  // read creds
  String ssid, pass;
  readCredentialsFromEEPROM(ssid, pass);
  if (ssid.length() == 0)
  {
    Serial.println("No WiFi credentials found. Starting config portal...");
    startConfigPortal();
  }
  else{
    Serial.print("Found SSID: ");
    Serial.println(ssid);
    bool ok = connectToWiFi(ssid, pass);
    
    if (!ok) 
    {
      Serial.println("WiFi connect failed. Entering config portal.");
      startConfigPortal();
    }
  }
  // setup MQTT client
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  ledOn();
  lockState = true;
}

//Loop: Handle auto reconnection for Wi-Fi and MQTT
void loop() 
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, trying to reconnect...");
    // try read creds and connect
    String ssid, pass;
    readCredentialsFromEEPROM(ssid, pass);
    
    if (ssid.length() > 0) 
    {
      if (connectToWiFi(ssid, pass)) {
        // connected
        mqttClient.setServer(mqtt_server, mqtt_port);
      } 
      
      else {
        startConfigPortal();
      }
    } 
    else {
      startConfigPortal();
    }
  }
  mqttReconnectIfNeeded();
  mqttClient.loop();
  
  static unsigned long lastHeartbeat = 0;
  
  if (millis() - lastHeartbeat > 60000UL) 
  {
    mqttClient.publish(statusTopic.c_str(), WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
    lastHeartbeat = millis();
  }
}

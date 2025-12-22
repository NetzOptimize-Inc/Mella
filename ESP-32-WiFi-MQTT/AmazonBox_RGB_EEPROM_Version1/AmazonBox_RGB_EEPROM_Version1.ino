// The Project is working
// Including the Reset Button
// As soon as Someone Press Reset Button, it Erase the EEPROM data and after try to connect to WiFi, if not connected, it Advertise the APMode and After connecting to APMode
// and browser connects to 192.168.4.1, After providing the WiFi Credentials, it Connects to the WiFi and executes the program. 
// The Device creates a new Identification every time, Thus we need to change the MQTT Susbscription name everytime to switch on/Off Blue LED. 

#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#define EEPROM_SIZE 256

// USER CONFIG
String defaultAP_SSID;
char id[5];


// const char* defaultAP_SSID = "AmazonBox-" + String(id);
// The Output will be like AmazonBox-1A3F OR AmazonBox-00AF OR AmazonBox-FE23
const char* defaultAP_PASS = "Connect123";
const char* mqtt_server = "broker.hivemq.com";
const uint16_t mqtt_port = 1883;

// LED Pins
#define RED_LED 15
#define YELLOW_LED 4
#define GREEN_LED 16
#define BLUE_LED 2

// RESET BUTTON
#define RESET_BUTTON_PIN 33

WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

String cmdTopic; 
String statusTopic; 
bool lockState = true;

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
  Serial.println(defaultAP_SSID);
  Serial.println("=============");
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

//EEPROM FUNCTIONS
void saveCredentialsToEEPROM(const char* ssid, const char* pass) {
  EEPROM.begin(EEPROM_SIZE);
  int idx = 0;
  for (int i = 0; i < 64; ++i)
    EEPROM.write(idx++, (i < (int)strlen(ssid)) ? ssid[i] : '\0');
  for (int i = 0; i < 128; ++i)
    EEPROM.write(idx++, (i < (int)strlen(pass)) ? pass[i] : '\0');
  EEPROM.commit();
  EEPROM.end();
}

void readCredentialsFromEEPROM(String& outSsid, String& outPass) {
  EEPROM.begin(EEPROM_SIZE);
  int idx = 0;
  char bufS[65]; bufS[64] = '\0';
  char bufP[129]; bufP[128] = '\0';
  for (int i = 0; i < 64; ++i) bufS[i] = EEPROM.read(idx++);
  for (int i = 0; i < 128; ++i) bufP[i] = EEPROM.read(idx++);
  EEPROM.end();
  outSsid = String(bufS);
  outPass = String(bufP);
  outSsid.trim();
  outPass.trim();
  Serial.printf("SSID from EEPROM: %s\n", outSsid.c_str());
  Serial.printf("PASS from EEPROM: %s\n", outPass.c_str());
}

// Erase the EEPRom. 
void factoryReset() {
  EEPROM.begin(512);
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0xFF);  // Default erased value
  }
  EEPROM.commit();
  Serial.println("EEPROM cleared. Restarting...");
  delay(300);
  ESP.restart();
}

//CONFIG PORTAL 
void startConfigPortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(defaultAP_SSID.c_str(), defaultAP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  Serial.println("APMode WiFi Name: " + defaultAP_SSID);
  Serial.print("Config AP IP: ");
  Serial.println(apIP);
  showConfigMode();

 server.on("/", []() {
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
    "<a href='/clear'>/clear</a></p>"
    "</body>"
    "</html>";
    
  server.send(200, "text/html", page);
});



  server.on("/save", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    saveCredentialsToEEPROM(ssid.c_str(), pass.c_str());
    server.send(200, "text/html", "Saved! Rebooting...");
    delay(2000);
    ESP.restart();
  });

  server.on("/remove", HTTP_POST, []() {
    factoryReset();
    server.send(200, "text/html", "Removed! Rebooting...");
    delay(2000);
    ESP.restart();
  });

  server.begin();
  while (true) {
    server.handleClient();
    delay(100);
  }
}

//WIFI CONNECTION
bool connectToWiFi(const String& ssid, const String& pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.print("Connecting to WiFi...");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 20000) {
      Serial.println("❌ Wi-Fi connect timeout");
      return false;
    }
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("✅ WiFi Connected! IP: " + WiFi.localIP().toString());
  return true;
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
  
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection ... :");
    String clientId = "AmazonBox-" + String(id);
    cmdTopic = clientId + "/cmd";
    statusTopic = clientId + "/status";
    Serial.print(cmdTopic);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println(" : Connected ✅");
      mqttClient.subscribe(cmdTopic.c_str());
      mqttClient.publish(statusTopic.c_str(), "CONNECTED");
      showConnected();
      Serial.println("📡 Subscribed To Topic: AmazonBox/cmd");
    } else {
      Serial.print("❌ Failed, rc=");
      Serial.println(mqttClient.state());
      Serial.println("Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

//SETUP

void setup() {  
  sprintf(id, "%04X", random(0xffff));
  defaultAP_SSID = String("AmazonBox-") + id;
  Serial.begin(115200);
  Serial.println(defaultAP_SSID);
  Serial.println("In Setup");
  
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  
  clearAllLEDs();

  showStartup();
  delay(2000);

  String ssid, pass;
  readCredentialsFromEEPROM(ssid, pass);
  if (ssid == "") {
    startConfigPortal();
  } else {
    if (!connectToWiFi(ssid, pass)) startConfigPortal();
  }

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  reconnectMQTT(); // connect and subscribe
}

//LOOP
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ Wi-Fi disconnected. Reconnecting...");
    String ssid, pass;
    readCredentialsFromEEPROM(ssid, pass);
    if (ssid != "") connectToWiFi(ssid, pass);
  }

  if (!mqttClient.connected()) {
    Serial.println("⚠️ MQTT disconnected. Reconnecting...");
    reconnectMQTT();
  }

  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    delay(50);
    
    if (digitalRead(RESET_BUTTON_PIN) == HIGH) {
      Serial.println("⚠️ RESET BUTTON CLICKED...");
      factoryReset();
    }
  }

  mqttClient.loop();
}

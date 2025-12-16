#include <WiFi.h>
#include <PubSubClient.h>

#define LED_PIN 2 

// MQTT Configuration
const char* mqtt_server = "broker.hivemq.com";  
const int mqtt_port = 1883;
const char* topic = "shreya/smartconfig/led";   

WiFiClient espClient;
PubSubClient client(espClient);

// Function to handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (message == "on") {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED turned ON");
  } else if (message == "off") {
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED turned OFF");
  }
}

// Function to connect to MQTT broker
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client_Shreya")) {
      Serial.println("connected");
      client.subscribe(topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 2 seconds...");
      delay(2000);
    }
  }
}

// Function to start SmartConfig
void startSmartConfig() {
  Serial.println("Waiting for SmartConfig...");
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();

  while (!WiFi.smartConfigDone()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nSmartConfig received.");
  Serial.println("Connecting to Wi-Fi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("\n--- ESP32 SmartConfig LED Control ---");
  startSmartConfig();

  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}

#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

#include <TimeLib.h>
#include <AnchorOTA.h>
#include "wifi_conf.h"
#include "wifi_drv.h"
#include <Preferences.h> // Added New
#ifdef DEBUG_MODE
  #define DEBUG_PRINT(x)     Serial.print(x)
  #define DEBUG_PRINTLN(x)   Serial.println(x, y)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

#define UART_SERVICE_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define STRING_BUF_SIZE 100
#define FACTORY_RESET1
#define DEBUG_MODE
#define OTA_PORT 8082

#define SSID_OFFSET        100
#define PASS_OFFSET        200
#define VALID_OFFSET       300

AmebaWebServer provisioningServer;
bool provisioningActive = false;

String provSSID = "";
String provPASS = "";

IPAddress ota_ip;
#define device_generation 2
#define firmware_version 001
#define SAVED_STATUS_ADDRESS 64
WiFiClient wifiClient;
PubSubClient client(wifiClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
#define ONE_WIRE_BUS PA27

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress mainThermometer;

String uniqueID;
String actionTopic, scheduleTopic, autoshutDownTopic, upgradeTopic, statusTopic, debugTopic;

String publish_status = "003915A1/Status";
String subscribe_action = "003915A1/Action";
String subscribe_schedule = "003915A1/Schedule";
String subscribe_autoshutdown = "003915A1/Autoshutdown";
String subscribe_upgrade = "003915A1/Upgrade";
String publish_debug = "003915A1/Debug";

int check_update = 1;
bool MQTT_Status = 0;

const char* mqttServer = "167.71.112.166"; // Working Credentials

const int mqttPort = 1883; // Working Credentials
const char* mqttUser = "themella"; // Working Credentials
const char* mqttPassword = "!@#mqtt"; // Working Credentials

WiFiUDP Udp;
unsigned int localPort = 8888; 
bool shouldHeat = false;
bool heatingState = false;  // Track current heating state
float hysteresis = 2.0;

#define LED2 PA15
#define LED_STATUS PA12
#define transistorPin PA26
#define analogInPin PB3

int sensorValue = 0;
int outputValue = 0;
int mappedOutput = 0;
int default_temp = 30;
float temp_c;
int calibration_offset = 5;
bool Connection_Status = 0;
bool wifiOnce = false;
bool wifiFirstConnect = true;
int timeZone;  
byte packetBuffer[NTP_PACKET_SIZE]; 

unsigned long lastVersionPublishTime = 0;
const unsigned long versionPublishInterval = 600000;

int wifiRetryCount = 0;
const int maxRetries = 3;
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 30000; 
bool bleActive = false;
String receivedSSID = "";
String receivedPASS = "";
bool gotSSID = false;
bool gotPASS = false;
bool shouldConnect = false;

BLEService UartService(UART_SERVICE_UUID);
BLECharacteristic Rx(CHARACTERISTIC_UUID_RX);
BLECharacteristic Tx(CHARACTERISTIC_UUID_TX);
BLEAdvertData advdata;
BLEAdvertData scndata;
bool notify = false;
bool wifiDisconnectNeeded = false;
bool status_on = false;
bool status_off = false;

char SSID[128];
char Password[128];

Preferences prefs;

static const char *PREF_NAMESPACE = "wifi_cfg";
static const char *KEY_SSID       = "ssid";
static const char *KEY_PASS       = "pass";
static const char *KEY_FLAG       = "valid";

static const uint32_t FLASH_VALID_FLAG = 0xA5A5A5A5; // same as your original logic


/********************************************************************
 * SAVE CREDENTIALS (Preferences version, same logic + debug)
 ********************************************************************/
void saveCredentialsToFlash(String ssid, String password) {
  DEBUG_PRINTLN("Saving credentials to flash memory...");

  if (!prefs.begin(PREF_NAMESPACE, false)) {
    DEBUG_PRINTLN("ERROR: Failed to open preferences namespace!");
    return;
  }

  // --- Write SSID ---
  DEBUG_PRINT("Saving SSID: ");
  DEBUG_PRINTLN(ssid);

  if (!prefs.putString(KEY_SSID, ssid)) {
    DEBUG_PRINTLN("ERROR: Failed to write SSID into flash!");
    prefs.end();
    return;
  }

  // --- Write Password ---
  DEBUG_PRINT("Saving Password (length): ");
  DEBUG_PRINTLN(password.length());

  if (!prefs.putString(KEY_PASS, password)) {
    DEBUG_PRINTLN("ERROR: Failed to write Password into flash!");
    prefs.end();
    return;
  }

  // --- Write validity flag ---
  DEBUG_PRINT("Writing validity flag: 0x");
  DEBUG_PRINTLN(String(FLASH_VALID_FLAG, HEX));

  if (!prefs.putUInt(KEY_FLAG, FLASH_VALID_FLAG)) {
    DEBUG_PRINTLN("ERROR: Failed to write validity flag!");
    prefs.end();
    return;
  }

  prefs.end();
  delay(50);

  // ===== VERIFY SAVED DATA (same flow as your original) =====
  prefs.begin(PREF_NAMESPACE, true);
  uint32_t readFlag = prefs.getUInt(KEY_FLAG, 0);
  prefs.end();

  if (readFlag == FLASH_VALID_FLAG) {
    DEBUG_PRINTLN("Credentials saved to flash memory successfully!");
    DEBUG_PRINT("Saved SSID: ");
    DEBUG_PRINTLN(ssid);
  } else {
    DEBUG_PRINTLN("ERROR: Failed to verify saved credentials!");
  }
}



/********************************************************************
 * LOAD CREDENTIALS (Preferences version, same logic + debug)
 ********************************************************************/
bool loadCredentialsFromFlash(String &ssid, String &password) {
  DEBUG_PRINTLN("Attempting to load credentials from flash memory...");

  if (!prefs.begin(PREF_NAMESPACE, true)) {
    DEBUG_PRINTLN("ERROR: Failed to open preferences namespace!");
    return false;
  }

  // --- Check validity flag ---
  uint32_t validFlag = prefs.getUInt(KEY_FLAG, 0);

  Serial.print("Flash validity flag: 0x");
  #ifdef DEBUG_MODE
    Serial.println(validFlag, HEX);
  #else
    Serial.println(validFlag, HEX);
  #endif

  if (validFlag != FLASH_VALID_FLAG) {
    DEBUG_PRINTLN("No valid credentials found in flash memory.");
    prefs.end();
    return false;
  }

  // --- Load SSID ---
  ssid = prefs.getString(KEY_SSID, "");
  DEBUG_PRINT("Loaded SSID: ");
  DEBUG_PRINTLN(ssid);

  // --- Load Password ---
  password = prefs.getString(KEY_PASS, "");
  DEBUG_PRINT("Password length: ");
  DEBUG_PRINTLN(password.length());

  prefs.end();

  // --- Validate contents just like your original logic ---
  if (ssid.length() > 0 && ssid.length() < 128) {
    DEBUG_PRINTLN("Credentials loaded from flash memory successfully!");
    return true;
  }

  DEBUG_PRINTLN("Invalid credentials data in flash memory.");
  return false;
}



/********************************************************************
 * CLEAR CREDENTIALS (Preferences version, same logic + debug)
 ********************************************************************/
void clearCredentialsFromFlash() {
  DEBUG_PRINTLN("Clearing credentials from flash memory...");

  if (!prefs.begin(PREF_NAMESPACE, false)) {
    DEBUG_PRINTLN("ERROR: Failed to open preferences namespace!");
    return;
  }

  // Remove keys (equivalent to writing 0xFF in your original logic)
  prefs.remove(KEY_SSID);
  prefs.remove(KEY_PASS);
  prefs.remove(KEY_FLAG);

  prefs.end();

  DEBUG_PRINTLN("Credentials cleared from flash memory.");
}

/**
 * @brief Enhanced WiFi connection function with retry mechanism
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return true if connected successfully, false otherwise
 */
bool connectToWiFi(String ssid, String password) {
  DEBUG_PRINTLN("Attempting to connect to WiFi...");
  Serial.print("SSID: ");
  DEBUG_PRINTLN(ssid);
  
  // Disconnect if already connected
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
    delay(1000);
  }
  
  // Convert to char arrays for compatibility
  char ssidBuffer[ssid.length() + 1];
  char passBuffer[password.length() + 1];
  ssid.toCharArray(ssidBuffer, sizeof(ssidBuffer));
  password.toCharArray(passBuffer, sizeof(passBuffer));
  
  WiFi.begin(ssidBuffer, passBuffer);
  
  // Wait for connection with timeout
  unsigned long start = millis();
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    Serial.print(".");
    dots++;
    if (dots % 40 == 0) { DEBUG_PRINTLN(); } // New line every 40 dots
    delay(500);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINTLN("\nWiFi Connected successfully!");
    Serial.print("IP Address: ");
    DEBUG_PRINTLN(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    DEBUG_PRINTLN(" dBm");
    wifiRetryCount = 0; // Reset retry count on successful connection
    return true;
  } else {
    DEBUG_PRINTLN("\nFailed to connect to WiFi.");
    Serial.print("WiFi Status: ");
    DEBUG_PRINTLN(WiFi.status());
    return false;
  }
}

/**
 * @brief 
 */
void stopBLE() {
  if (bleActive) {
    DEBUG_PRINTLN("Stopping BLE services...");
    
    // Stop advertising first
    BLE.configAdvert()->stopAdv();
    delay(500);
    
    // End BLE
    BLE.end();
    delay(1000);
    
    bleActive = false;
    DEBUG_PRINTLN("BLE services stopped.");
  }
}

/**
 * @brief Starts BLE advertising for WiFi configuration
 */
void startBLEConfig() {
  if (bleActive) {
    DEBUG_PRINTLN("BLE already active, stopping first...");
    stopBLE();
  }
  
  DEBUG_PRINTLN("Starting BLE Wi-Fi Config...");
  
  // Reset flags
  gotSSID = false;
  gotPASS = false;
  shouldConnect = false;
  
  // Initialize BLE
  BLE.init();
  BLE.configServer(1);
  
  bleActive = true;
  DEBUG_PRINTLN("BLE Wi-Fi Config Service Started and Advertising.");
}

void readCB (BLECharacteristic* chr, uint8_t connID) {
    (void)connID;  // suppress unused parameter warning
    Serial.print("Characteristic ");
    Serial.print(chr->getUUID().str());
    Serial.print(" read by connection ");
    DEBUG_PRINTLN(connID);
}

void setup() {
  Serial.begin(115200);
  Serial.println("DEBUG: setup done");
  delay(2000); // Give serial time to initialize
  DEBUG_PRINTLN("\n"); // Print a newline for cleaner output
  
  pinMode(LED2, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(transistorPin, OUTPUT);
  WiFi.status(); // Initialize WiFi subsystem
  delay(1000);
  
  #ifdef FACTORY_RESET
    DEBUG_PRINTLN("Attempting to factory reset ESP32...");
    clearCredentialsFromFlash(); // Use enhanced clear function
    DEBUG_PRINTLN("Factory reset done: WiFi credentials cleared.");
  #endif

  uint8_t macBuffer[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(macBuffer);
  Serial.print("MAC Address: ");
  for (int i = 0; i < WL_MAC_ADDR_LENGTH; i++) {
    if (macBuffer[i] < 16) {
      Serial.print("0");
    }
    Serial.print(macBuffer[i], HEX);
    if (i < 5) {
      Serial.print(":");
    }
  }
  DEBUG_PRINTLN();
  
  char chipID[13];
  sprintf(chipID, "%02X%02X%02X%02X%02X%02X", macBuffer[0], macBuffer[1], macBuffer[2], macBuffer[3], macBuffer[4], macBuffer[5]);
  uniqueID = String(chipID);

  DEBUG_PRINTLN("*********************************************");
  Serial.print("Device Id: ");
  DEBUG_PRINTLN(uniqueID);
  Serial.print("Device Iteration: ");
  DEBUG_PRINTLN(device_generation);
  Serial.print("Firmware Version: ");
  DEBUG_PRINTLN(firmware_version);
  DEBUG_PRINTLN("*********************************************");

  // Initialize sensors
  sensors.begin();

  String storedSSID = "";
  String storedPASS = "";
  bool hasValidCredentials = false;
  
  if (loadCredentialsFromFlash(storedSSID, storedPASS)) {
    DEBUG_PRINTLN("Found stored credentials, attempting to connect...");
 
    strncpy(SSID, storedSSID.c_str(), sizeof(SSID) - 1);
    SSID[sizeof(SSID) - 1] = '\0';
    strncpy(Password, storedPASS.c_str(), sizeof(Password) - 1);
    Password[sizeof(Password) - 1] = '\0';

    if (connectToWiFi(storedSSID, storedPASS)) {
        DEBUG_PRINTLN("Connected using stored credentials!");
        receivedSSID = storedSSID;
        receivedPASS = storedPASS;
        lastWiFiCheck = millis();
        hasValidCredentials = true;
    } else {
        DEBUG_PRINTLN("Failed to connect with stored credentials.");
        DEBUG_PRINTLN("Clearing old credentials...");
        clearCredentialsFromFlash();
    }
  }
  else
  {
      // ===== NO CREDENTIALS FOUND → ENTER PROVISIONING MODE =====
      DEBUG_PRINTLN("No stored credentials found.");
      DEBUG_PRINTLN("Starting WiFi provisioning mode...");

      startProvisioningMode();  
      // <--- you already have this or similar (AP mode / BLE setup / Web portal)

      hasValidCredentials = false;
  }

  if (!hasValidCredentials) {
    DEBUG_PRINTLN("Setting up BLE for WiFi configuration...");
    ble_setup();
  } else {
    DEBUG_PRINTLN("WiFi connected, skipping BLE setup.");
  }
}




// Optional: Add function to adjust hysteresis dynamically
void setHysteresis(float newHysteresis) {
  if (newHysteresis > 0 && newHysteresis <= 10) {  // Reasonable bounds
    hysteresis = newHysteresis;
    #ifdef DEBUG_MODE
    Serial.print("Hysteresis set to: ±");
    Serial.println(hysteresis/2);
    #endif
  }
}

void loop() {
  // Visual heartbeat (retain original LED behavior)
  digitalWrite(LED2, HIGH);
  digitalWrite(LED_STATUS, HIGH);

  // Handle provisioning web server if active (AP mode provisioning)
  if (provisioningActive) {
    provisioningServer.handleClient();
  }

  // Handle new WiFi credentials received from provisioning (web portal)
  if (shouldConnect && gotSSID && gotPASS) {
    DEBUG_PRINTLN("Attempting to connect with new credentials...");
    shouldConnect = false;

    // small delay to allow any pending web requests to finish
    delay(1000);

    if (connectToWiFi(receivedSSID, receivedPASS)) {
      DEBUG_PRINTLN("WiFi connection successful! Saving credentials...");

      // Save credentials using Preferences-based function
      saveCredentialsToFlash(receivedSSID, receivedPASS);

      // Stop provisioning web server / AP if it was active
      if (provisioningActive) {
        provisioningServer.stop();
        WiFi.softAPdisconnect(true);
        provisioningActive = false;
        DEBUG_PRINTLN("Provisioning stopped.");
      }

      DEBUG_PRINTLN("Configuration complete!");
      wifiFirstConnect = true;
    } else {
      DEBUG_PRINTLN("WiFi connection failed with new credentials.");
    }

    // clear temporary flags
    gotSSID = false;
    gotPASS = false;
  }

  // Handle manual wifi disconnect request
  if (wifiDisconnectNeeded) {
    WiFi.disconnect();
    DEBUG_PRINTLN("Disconnected from WiFi.");
    wifiDisconnectNeeded = false;
  }

  // If connected to WiFi
  if (WiFi.status() == WL_CONNECTED) {
    if (wifiFirstConnect) {
      // Do one-time actions after the first successful connection
      timeClient.update();

      rtw_wifi_setting_t wifi_setting;
      wifi_get_setting(WLAN0_NAME, &wifi_setting);

      wifiFirstConnect = false;
      DEBUG_PRINTLN("Saving SSID");
      DEBUG_PRINTLN(WiFi.SSID());

      // Prefer receivedSSID if it exists (came from provisioning); otherwise use current WiFi info
      if (receivedSSID.length() > 0) {
        saveCredentialsToFlash(receivedSSID, receivedPASS);
      } else {
        // Save the SSID/password obtained from the stack (wifi_setting.password)
        // Convert C char* to String for saveCredentialsToFlash()
        String currentSSID = String(WiFi.SSID());
        String currentPASS = String((const char *)wifi_setting.password);
        saveCredentialsToFlash(currentSSID, currentPASS);
      }
    }

    // MQTT setup / routine
    if (MQTT_Status == 0) {
      setupMQTT();
      MQTT_Status = 1;
    } else {
      routine_task();
    }

    // Maintain MQTT client
    client.loop();
    if (!client.connected()) {
      reconnect();
    }

    publishFirmwareVersion();

  } else {
    // Not connected path
    DEBUG_PRINTLN("Not Connected");
    if (millis() - lastWiFiCheck > wifiCheckInterval) {
      lastWiFiCheck = millis();
      // Try reconnecting only if we have stored SSID/Password and haven't hit retries
      if (strlen(SSID) > 0 && strlen(Password) > 0 && wifiRetryCount < maxRetries) {
        Serial.print("WiFi disconnected. Retry attempt ");
        Serial.print(wifiRetryCount + 1);
        Serial.print(" of ");
        DEBUG_PRINTLN(maxRetries);

        String ssidStr = String(SSID);
        String passStr = String(Password);

        if (connectToWiFi(ssidStr, passStr)) {
          DEBUG_PRINTLN("Reconnected successfully!");
        } else {
          wifiRetryCount++;
          if (wifiRetryCount >= maxRetries) {
            DEBUG_PRINTLN("Max retry attempts reached. Starting provisioning (AP mode)...");
            // Clear old credentials and start provisioning AP
            clearCredentialsFromFlash();
            memset(SSID, 0, sizeof(SSID));
            memset(Password, 0, sizeof(Password));
            receivedSSID = "";
            receivedPASS = "";
            startProvisioningMode();
          }
        }
      }
      // If no credentials at all, start provisioning (AP mode)
      else if (!provisioningActive && strlen(SSID) == 0) {
        DEBUG_PRINTLN("No WiFi connection and no valid credentials. Starting provisioning (AP mode)...");
        startProvisioningMode();
      }
    }
  }

  // Sensor / knob processing (unchanged)
  sensorValue = analogRead(analogInPin) / 32;
  client.publish(publish_debug.c_str(), String("Knob Value" + String(analogRead(analogInPin))).c_str());
  DEBUG_PRINTLN(analogRead(analogInPin));
  mappedOutput = map(sensorValue, 0, 25, 0, 10);
  outputValue = constrain(mappedOutput, 0, 10);

  #ifdef DEBUG_MODE
    Serial.print("Pot value: ");
    Serial.print(outputValue);
  #endif

  sensors.requestTemperatures();
  temp_c = sensors.getTempCByIndex(0);
  #ifdef DEBUG_MODE
    Serial.print("  Temperature is: ");
    Serial.println(temp_c);
  #endif

  if (set_temp(outputValue)) {
    sensors.setResolution(9);
    sensors.requestTemperatures();
    int n = 0;
    while (!sensors.isConversionComplete()) n++;
    temp_c = sensors.getTempC(mainThermometer);
  }

  delay(500);
}

void printWifiStatus() {
  Serial.print("SSID: ");
  DEBUG_PRINTLN(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  (void)ip;
  Serial.print("IP Address: ");
  DEBUG_PRINTLN(ip);
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  DEBUG_PRINTLN(" dBm");
}

void routine_task(void) {
  digitalWrite(LED2, shouldHeat);

  if (shouldHeat) {
    updateMqtt(1);
  } else {
    updateMqtt(0);
  }

#ifdef DEBUG_MODE
  DEBUG_PRINTLN("Checking Schedule");
#endif
  compare_time();
}

void setupMQTT() {
  // MQTT Configuration
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  client.setKeepAlive(1800);

  while (!client.connected()) {
#ifdef DEBUG_MODE
    DEBUG_PRINTLN("Connecting to MQTT...");
#endif

    if (client.connect(uniqueID.c_str(), mqttUser, mqttPassword)) {
#ifdef DEBUG_MODE
      DEBUG_PRINTLN("connected");
#endif
    } else {
#ifdef DEBUG_MODE
      Serial.print("failed with state ");
      Serial.print(client.state());
#endif
      delay(2000);
      yield();
    }
  }

  Udp.begin(localPort);
  FlashMemory.read();
  int currentZone;
  currentZone = FlashMemory.buf[35];
  if (currentZone > 10 || currentZone < 3) {
    currentZone = 7;
  }
  int timechange = currentZone * 2;
  timeZone = currentZone - timechange;

#ifdef DEBUG_MODE
  DEBUG_PRINTLN("------ TZ ------");
  DEBUG_PRINTLN(currentZone);
  DEBUG_PRINTLN(timeZone);
#endif

  actionTopic = uniqueID + "/Action";
  scheduleTopic = uniqueID + "/Schedule";
  autoshutDownTopic = uniqueID + "/Autoshutdown";
  upgradeTopic = uniqueID + "/Upgrade";
  statusTopic = uniqueID + "/Status";
  debugTopic = uniqueID + "/Debug";

  publish_status = statusTopic;
  subscribe_action = actionTopic;
  subscribe_schedule = scheduleTopic;
  subscribe_autoshutdown = autoshutDownTopic;
  subscribe_upgrade = upgradeTopic;
  publish_debug = debugTopic;

#ifdef DEBUG_MODE
  DEBUG_PRINTLN("STARTING TASKER");
#endif
  client.publish(publish_status.c_str(), "0");
  client.subscribe(subscribe_action.c_str());
  client.subscribe(subscribe_schedule.c_str());
  client.subscribe(subscribe_autoshutdown.c_str());
  client.subscribe(subscribe_upgrade.c_str());
  timeClient.begin();
}
void updateMqtt(int status)
{
  
  String update;
  if (status == 0) {
    update = "0";
  }
  else {
    update = "1";
  }
  DEBUG_PRINTLN(publish_status);

  if (client.connected())
  {
    client.publish(publish_status.c_str(), update.c_str());
  }
}
void callback(char* topic, byte* payload, unsigned int length) {
  int case_state = 0;
  String data(topic);
  char char_recieve;
  String str_recieve;

#ifdef DEBUG_MODE
  Serial.print("Message arrived in topic: ");
  DEBUG_PRINTLN(topic);
  DEBUG_PRINTLN(data);
#endif
  if (data == subscribe_action) {
    case_state = 1;
  } else if (data == subscribe_schedule) {
    case_state = 2;
  } else if (data == subscribe_autoshutdown) {
    case_state = 3;
  } else if (data == subscribe_upgrade) {
    case_state = 4;
  }
  for (unsigned int i = 0; i < length; i++) {
    char_recieve = (char)payload[i];
    str_recieve += char_recieve;
  }
  switch (case_state) {
    case 1:
      switch ((char)payload[0]) {
        case '0':
          handleOff();
          FlashMemory.buf[SAVED_STATUS_ADDRESS] = 0;
          FlashMemory.update();
          if (shouldHeat) {
            updateMqtt(0);
          }

          break;

        case '1':
          handleOn();
          FlashMemory.buf[SAVED_STATUS_ADDRESS] = 1;
          FlashMemory.update();
          if (!shouldHeat) {
            updateMqtt(1);
          }
          break;

        default:
          break;
      }
      break;

    case 2:
      save_to_eeprom(str_recieve);
      blink_led(20);
      break;

    case 3:
      read_eeprom();
      blink_led(20);
      break;

    case 4:
      if (length > 0) {
        firmware_update(str_recieve);
      }
      break;

    default:
      break;
  }

#ifdef DEBUG_MODE
  DEBUG_PRINTLN();
  Serial.print("Message:");
  DEBUG_PRINTLN(str_recieve);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("-----------------------");
#endif
}

void firmware_update(String hostname) {
  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINTLN("Starting firmware update...");
    Serial.print("OTA Hostname: ");
    DEBUG_PRINTLN(hostname);

    IPAddress ota_ip;
    if (WiFi.hostByName(hostname.c_str(), ota_ip)) {
      Serial.print("Resolved OTA IP Address: ");
      DEBUG_PRINTLN(ota_ip);
      Serial.print("OTA Port: ");
      DEBUG_PRINTLN(OTA_PORT);

      if (OTA.beginCloud(ota_ip, OTA_PORT) == 0) {
        DEBUG_PRINTLN("Firmware update successful!");
      } else {
        DEBUG_PRINTLN("Firmware update failed!");
      }
    } else {
      DEBUG_PRINTLN("Failed to resolve OTA IP address.");
    }
  } else {
    DEBUG_PRINTLN("Not connected to WiFi");
  }
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
#ifdef DEBUG_MODE
    Serial.print("MQTT reconnect...");
#endif
    // Attempt to connect
    if (client.connect(uniqueID.c_str(), mqttUser, mqttPassword)) {
#ifdef DEBUG_MODE
      DEBUG_PRINTLN("connected!");
#endif
      client.subscribe(subscribe_action.c_str());
      client.subscribe(subscribe_schedule.c_str());
      client.subscribe(subscribe_autoshutdown.c_str());
      client.subscribe(subscribe_upgrade.c_str());
    } else {
#ifdef DEBUG_MODE
      Serial.print("failed, rc=");
      Serial.print(client.state());
      DEBUG_PRINTLN(" try again in 5 s");
#endif
      yield();
    }
  }
}

void handleOn()
{
#ifdef DEBUG_MODE
	DEBUG_PRINTLN("Relay ON");
#endif
	shouldHeat = true;
}

void handleOff()
{
#ifdef DEBUG_MODE
	DEBUG_PRINTLN("Relay OFF");
#endif
	shouldHeat = false;
}

void blink_led(int time)
{
	digitalWrite(LED_STATUS, HIGH);
	delay(time);
	digitalWrite(LED_STATUS, LOW);
	delay(time);
	digitalWrite(LED_STATUS, HIGH);
	delay(time);
	digitalWrite(LED_STATUS, LOW);
	delay(time);
	digitalWrite(LED_STATUS, HIGH);
	delay(time);
	digitalWrite(LED_STATUS, LOW);
	delay(time);
	digitalWrite(LED_STATUS, HIGH);
	delay(time);
	digitalWrite(LED_STATUS, LOW);
	delay(time);
	digitalWrite(LED_STATUS, HIGH);
	delay(time);
	digitalWrite(LED_STATUS, LOW);
	delay(time);
	digitalWrite(LED_STATUS, HIGH);
	delay(time);
	digitalWrite(LED_STATUS, LOW);
	delay(time); 
	digitalWrite(LED_STATUS, HIGH);
	delay(time);
	digitalWrite(LED_STATUS, LOW);
	delay(time);
}

bool set_temp(int knob_input)
{
  if (temp_c == DEVICE_DISCONNECTED_C) {
    DEBUG_PRINTLN("No Thermometer");
    heatOff();
    heatingState = false;
    return false;
  }
  
  if(wifiOnce && !Connection_Status){
    DEBUG_PRINTLN("Disconnect Detected");
    heatOff();
    heatingState = false;
    return false;
  }
  
  DEBUG_PRINTLN("Set Temp with Hysteresis");
  
  float targetTemp = 0;
  
  // Determine target temperature based on knob input
  switch (knob_input)
  {
    case 1:  targetTemp = 30 + calibration_offset; break;
    case 2:  targetTemp = 40 + calibration_offset; break;
    case 3:  targetTemp = 50 + calibration_offset + 5; break;
    case 4:  targetTemp = 60 + calibration_offset; break;
    case 5:  targetTemp = 70 + calibration_offset + 5; break;
    case 6:  targetTemp = 80 + calibration_offset; break;
    case 7:  targetTemp = 90 + calibration_offset; break;
    case 8:  targetTemp = 100 + calibration_offset; break;
    case 9:  targetTemp = 110 + calibration_offset; break;
    case 10: targetTemp = 120 + calibration_offset; break;
    default: targetTemp = default_temp; break;
  }
  // Amod Debug Code Started
float currentTemp = temp_c;           // DS18B20 reading
float target = targetTemp;            // set from knob
float hysteresis = 2.0;               // +/- hysteresis in degrees (adjust as needed)

// Compute simple hysteresis thresholds
float lowerThreshold = target - (hysteresis / 2.0);
float upperThreshold = target + (hysteresis / 2.0);

// DEBUG print for thresholds
Serial.print("DBG_THRESH lower=");
Serial.print(lowerThreshold);
Serial.print(" upper=");
Serial.println(upperThreshold);

// Determine shouldHeat based purely on instantaneous reading (not toggled state)
bool newShouldHeat = false;
if (currentTemp < lowerThreshold) {
  newShouldHeat = true;
} else if (currentTemp > upperThreshold) {
  newShouldHeat = false;
} else {
  // If within hysteresis band, keep previous heatingState to avoid rapid toggle
  newShouldHeat = heatingState ? true : false;
}

// Update shouldHeat variable used elsewhere
shouldHeat = newShouldHeat;

// Now set heatingState (you can keep or remove extra state variable)
heatingState = shouldHeat;

// Drive the transistor/relay pin according to heatingState
pinMode(transistorPin, OUTPUT); // ensure output mode set
// NOTE: If your relay is ACTIVE-LOW, swap HIGH/LOW below
digitalWrite(transistorPin, heatingState ? HIGH : LOW);

// EXTRA DEBUG
Serial.print("DBG_DECISION temp=");
Serial.print(currentTemp);
Serial.print(" target=");
Serial.print(target);
Serial.print(" shouldHeat=");
Serial.print(shouldHeat ? 1 : 0);
Serial.print(" heatingState=");
Serial.print(heatingState ? 1 : 0);
Serial.print(" transistorPin_read=");
Serial.println(digitalRead(transistorPin));
  // Amod Debug Code Ended
  // Implement hysteresis control logic
  if (!heatingState) {
    // Heater is currently OFF
    // Turn ON when temperature drops below (target - hysteresis/2)
    if (temp_c <= (targetTemp - hysteresis/2)) {
      heatingState = true;
      heat();
      #ifdef DEBUG_MODE
      Serial.print("My Heater ON - Temp: ");
      Serial.print(temp_c);
      Serial.print("°C, Target: ");
      Serial.print(targetTemp);
      Serial.print("°C, Lower threshold: ");
      Serial.println(targetTemp - hysteresis/2);
      #endif
    }
  } else {
    // Heater is currently ON
    // Turn OFF when temperature rises above (target + hysteresis/2)
    if (temp_c >= (targetTemp + hysteresis/2)) {
      heatingState = false;
      heatOff();
      #ifdef DEBUG_MODE
      Serial.print("Heater OFF - Temp: ");
      Serial.print(temp_c);
      Serial.print("°C, Target: ");
      Serial.print(targetTemp);
      Serial.print("°C, Upper threshold: ");
      Serial.println(targetTemp + hysteresis/2);
      #endif
    }
  }
  
  #ifdef DEBUG_MODE
  Serial.print("Current State - Heating: ");
  Serial.print(heatingState ? "ON" : "OFF");
  Serial.print(", Temp: ");
  Serial.print(temp_c);
  Serial.print("°C, Target: ");
  Serial.print(targetTemp);
  Serial.print("°C, Hysteresis: ±");
  Serial.println(hysteresis/2);
  #endif
  
  Serial.print("Heater Pin Value: ");
  DEBUG_PRINTLN(digitalRead(transistorPin));
  return true;
}
// bool set_temp(int knob_input)
// {
//   if (temp_c == DEVICE_DISCONNECTED_C) {
//     DEBUG_PRINTLN("No Thermometer");
//     heatOff();
//     return false;
//   }
//   if(wifiOnce && !Connection_Status){
//     DEBUG_PRINTLN("Disconnect Detected");
//     heatOff();
//     return false;
//   }
//   DEBUG_PRINTLN("Set Temp");
//   switch (knob_input)
//   {
//     case 1:
//       if (temp_c <= 30 + calibration_offset)
//       {
//          heat();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 1 ON");
// #endif
//       }
//       else
//       {
//         heatOff();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 1 OFF");
// #endif
//       }
//       break;
//     case 2:
//       if (temp_c <= 40 + calibration_offset)
//       {
//         // Turn on the mosfet
//          heat();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 2 ON");
// #endif
//       }
//       else
//       {
//         // Turn off the mosfet
//         heatOff();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 2 OFF");
// #endif
//       }
//       break;
//     case 3:
//       if (temp_c <= 50 + calibration_offset + 5) 
//       {
//          heat();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 3 ON");
// #endif
//       }
//       else
//       {
//         // Turn off the mosfet
//         heatOff();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 3 OFF");
// #endif
//       }
//       break;
//     case 4:
//       if (temp_c <= 60 + calibration_offset)
//       {
//         // Turn on the mosfet
//          heat();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 4 ON");
// #endif
//       }
//       else
//       {
//         heatOff();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 4 OFF");
// #endif
//       }
//       break;
//     case 5:
//       if (temp_c <= 70 + calibration_offset + 5) 
//       {
//          heat();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 5 ON");
// #endif
//       }
//       else
//       {
//         heatOff();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 5 OFF");
// #endif
//       }
//       break;

//     case 6:
//       if (temp_c <= 80 + calibration_offset)
//       {
//          heat();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 6 ON");
// #endif
//       }
//       else
//       {
//         heatOff();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 6 OFF");
// #endif
//       }
//       break;

//     case 7:
//       if (temp_c <= 90 + calibration_offset)
//       {
//          heat();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 7 ON");
// #endif
//       }
//       else
//       {
//         heatOff();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 7 OFF");
// #endif
//       }
//       break;

//     case 8:
//       if (temp_c <= 100 + calibration_offset)
//       {
//          heat();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 8 ON");
// #endif
//       }
//       else
//       {
//         heatOff();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 8 OFF");
// #endif
//       }
//       break;

//     case 9:
//       if (temp_c <= 110 + calibration_offset)
//       {
//          heat();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 9 ON");
// #endif
//       }
//       else
//       { 
//         heatOff();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 9 OFF");
// #endif
//       }
//       break;

//     case 10:
//       if (temp_c <= 120 + calibration_offset)
//       {
//          heat();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 10 ON");
// #endif
//       }
//       else
//       {
//         heatOff();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet Case 10 OFF");
// #endif
//       }
//       break;

//      default:
//       if (temp_c <= default_temp)
//       {
//          heat();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet default ON");
// #endif
//       }
//       else
//       {
//         heatOff();
// #ifdef DEBUG_MODE
//         DEBUG_PRINTLN("Mosfet default OFF");
// #endif
//       }
//       break;
//   }
//   Serial.print("Heater Pin Value: ");
//   DEBUG_PRINTLN(digitalRead(transistorPin));
//   return true;
// }


void heat(){
  #ifdef DEBUG_MODE
    DEBUG_PRINT("Value Of shouldHeat: ");
    DEBUG_PRINTLN(shouldHeat);
  #endif

  if(shouldHeat){
    #ifdef DEBUG_MODE
      DEBUG_PRINT("RELAY ON");
    #endif
    digitalWrite(transistorPin, HIGH);
  }
  else{
    #ifdef DEBUG_MODE
      DEBUG_PRINT("RELAY OFF");
    #endif
    digitalWrite(transistorPin, LOW);
  }
}

void heatOff(){
  digitalWrite(transistorPin, LOW);
}

void save_to_eeprom(String eepromStr)
{
  int st_hr_int_sun, st_min_int_sun, en_hr_int_sun, en_min_int_sun;
  int st_hr_int_mon, st_min_int_mon, en_hr_int_mon, en_min_int_mon;
  int st_hr_int_tue, st_min_int_tue, en_hr_int_tue, en_min_int_tue;
  int st_hr_int_wed, st_min_int_wed, en_hr_int_wed, en_min_int_wed;
  int st_hr_int_thur, st_min_int_thur, en_hr_int_thur, en_min_int_thur;
  int st_hr_int_fri, st_min_int_fri, en_hr_int_fri, en_min_int_fri;
  int st_hr_int_sat, st_min_int_sat, en_hr_int_sat, en_min_int_sat;
  int sun_int, mon_int, tue_int, wed_int, thur_int, fri_int, sat_int;


  int firstColon = eepromStr.indexOf(":");
  String st_hr_str_sun = eepromStr.substring(0, firstColon);
  String st_min_str_sun = eepromStr.substring(firstColon + 1, firstColon + 3);
  String en_hr_str_sun = eepromStr.substring(firstColon + 4, firstColon + 6);
  String en_min_str_sun = eepromStr.substring(firstColon + 7, firstColon + 9);

  String st_hr_str_mon = eepromStr.substring(firstColon + 10, firstColon + 12);
  String st_min_str_mon = eepromStr.substring(firstColon + 13, firstColon + 15);
  String en_hr_str_mon = eepromStr.substring(firstColon + 16, firstColon + 18);
  String en_min_str_mon = eepromStr.substring(firstColon + 19, firstColon + 21);

  String st_hr_str_tue = eepromStr.substring(firstColon + 22, firstColon + 24);
  String st_min_str_tue = eepromStr.substring(firstColon + 25, firstColon + 27);
  String en_hr_str_tue = eepromStr.substring(firstColon + 28, firstColon + 30);
  String en_min_str_tue = eepromStr.substring(firstColon + 31, firstColon + 33);

  String st_hr_str_wed = eepromStr.substring(firstColon + 34, firstColon + 36);
  String st_min_str_wed = eepromStr.substring(firstColon + 37, firstColon + 39);
  String en_hr_str_wed = eepromStr.substring(firstColon + 40, firstColon + 42);
  String en_min_str_wed = eepromStr.substring(firstColon + 43, firstColon + 45);

  String st_hr_str_thur = eepromStr.substring(firstColon + 46, firstColon + 48);
  String st_min_str_thur = eepromStr.substring(firstColon + 49, firstColon + 51);
  String en_hr_str_thur = eepromStr.substring(firstColon + 52, firstColon + 54);
  String en_min_str_thur = eepromStr.substring(firstColon + 55, firstColon + 57);

  String st_hr_str_fri = eepromStr.substring(firstColon + 58, firstColon + 60);
  String st_min_str_fri = eepromStr.substring(firstColon + 61, firstColon + 63);
  String en_hr_str_fri = eepromStr.substring(firstColon + 64, firstColon + 66);
  String en_min_str_fri = eepromStr.substring(firstColon + 67, firstColon + 69);

  String st_hr_str_sat = eepromStr.substring(firstColon + 70, firstColon + 72);
  String st_min_str_sat = eepromStr.substring(firstColon + 73, firstColon + 75);
  String en_hr_str_sat = eepromStr.substring(firstColon + 76, firstColon + 78);
  String en_min_str_sat = eepromStr.substring(firstColon + 79, firstColon + 81);

  String no_days_str = eepromStr.substring(firstColon + 82, firstColon + 90);

  String sun = no_days_str.substring(0, 1);
  String mon = no_days_str.substring(1, 2);
  String tue = no_days_str.substring(2, 3);
  String wed = no_days_str.substring(3, 4);
  String thur = no_days_str.substring(4, 5);
  String fri = no_days_str.substring(5, 6);
  String sat = no_days_str.substring(6, 7);
  String zone = no_days_str.substring(7, 8);


  st_hr_int_sun = st_hr_str_sun.toInt();
  st_min_int_sun = st_min_str_sun.toInt();
  en_hr_int_sun = en_hr_str_sun.toInt();
  en_min_int_sun = en_min_str_sun.toInt();

  st_hr_int_mon = st_hr_str_mon.toInt();
  st_min_int_mon = st_min_str_mon.toInt();
  en_hr_int_mon = en_hr_str_mon.toInt();
  en_min_int_mon = en_min_str_mon.toInt();

  st_hr_int_tue = st_hr_str_tue.toInt();
  st_min_int_tue = st_min_str_tue.toInt();
  en_hr_int_tue = en_hr_str_tue.toInt();
  en_min_int_tue = en_min_str_tue.toInt();

  st_hr_int_wed = st_hr_str_wed.toInt();
  st_min_int_wed = st_min_str_wed.toInt();
  en_hr_int_wed = en_hr_str_wed.toInt();
  en_min_int_wed = en_min_str_wed.toInt();

  st_hr_int_thur = st_hr_str_thur.toInt();
  st_min_int_thur = st_min_str_thur.toInt();
  en_hr_int_thur = en_hr_str_thur.toInt();
  en_min_int_thur = en_min_str_thur.toInt();

  st_hr_int_fri = st_hr_str_fri.toInt();
  st_min_int_fri = st_min_str_fri.toInt();
  en_hr_int_fri = en_hr_str_fri.toInt();
  en_min_int_fri = en_min_str_fri.toInt();

  st_hr_int_sat = st_hr_str_sat.toInt();
  st_min_int_sat = st_min_str_sat.toInt();
  en_hr_int_sat = en_hr_str_sat.toInt();
  en_min_int_sat = en_min_str_sat.toInt();

  sun_int = sun.toInt();
  mon_int = mon.toInt();
  tue_int = tue.toInt();
  wed_int = wed.toInt();
  thur_int = thur.toInt();
  fri_int = fri.toInt();
  sat_int = sat.toInt();
  int currentZone = zone.toInt();

  int change = currentZone * 2;
  timeZone = currentZone - change;



  for (int i = 0; i < 36; i++)
  {
    FlashMemory.buf[i] = 0;
  }
  FlashMemory.buf[0] = st_hr_int_sun;
  FlashMemory.buf[1] = st_min_int_sun;
  FlashMemory.buf[2] = en_hr_int_sun;
  FlashMemory.buf[3] = en_min_int_sun;

  FlashMemory.buf[4] = st_hr_int_mon;
  FlashMemory.buf[5] =st_min_int_mon;
  FlashMemory.buf[6] = en_hr_int_mon;
  FlashMemory.buf[7] = en_min_int_mon;

  FlashMemory.buf[8] = st_hr_int_tue;
  FlashMemory.buf[9] = st_min_int_tue;
  FlashMemory.buf[10] = en_hr_int_tue;
  FlashMemory.buf[11] = en_min_int_tue;

  FlashMemory.buf[12] = st_hr_int_wed;
  FlashMemory.buf[13] = st_min_int_wed;
  FlashMemory.buf[14] = en_hr_int_wed;
  FlashMemory.buf[15] = en_min_int_wed;

  FlashMemory.buf[16] = st_hr_int_thur;
  FlashMemory.buf[17] = st_min_int_thur;
  FlashMemory.buf[18] = en_hr_int_thur;
  FlashMemory.buf[19] = en_min_int_thur;

  FlashMemory.buf[20] = st_hr_int_fri;
  FlashMemory.buf[21] = st_min_int_fri;
  FlashMemory.buf[22] = en_hr_int_fri;
  FlashMemory.buf[23] = en_min_int_fri;

  FlashMemory.buf[24] = st_hr_int_sat;
  FlashMemory.buf[25] = st_min_int_sat;
  FlashMemory.buf[26] = en_hr_int_sat;
  FlashMemory.buf[27] = en_min_int_sat;

  FlashMemory.buf[28] = sun_int;
  FlashMemory.buf[29] = mon_int;
  FlashMemory.buf[30] = tue_int;
  FlashMemory.buf[31] = wed_int;
  FlashMemory.buf[32] = thur_int;
  FlashMemory.buf[33] = fri_int;
  FlashMemory.buf[34] = sat_int;
  FlashMemory.buf[35] = currentZone;
  FlashMemory.update();

#ifdef DEBUG_MODE
  DEBUG_PRINTLN();
  DEBUG_PRINTLN(st_hr_str_sun);
  DEBUG_PRINTLN(st_min_str_sun);
  DEBUG_PRINTLN(en_hr_str_sun);
  DEBUG_PRINTLN(en_min_str_sun);

  DEBUG_PRINTLN(st_hr_str_mon);
  DEBUG_PRINTLN(st_min_str_mon);
  DEBUG_PRINTLN(en_hr_str_mon);
  DEBUG_PRINTLN(en_min_str_mon);

  DEBUG_PRINTLN(st_hr_str_tue);
  DEBUG_PRINTLN(st_min_str_tue);
  DEBUG_PRINTLN(en_hr_str_tue);
  DEBUG_PRINTLN(en_min_str_tue);

  DEBUG_PRINTLN(st_hr_str_wed);
  DEBUG_PRINTLN(st_min_str_wed);
  DEBUG_PRINTLN(en_hr_str_wed);
  DEBUG_PRINTLN(en_min_str_wed);

  DEBUG_PRINTLN(st_hr_str_thur);
  DEBUG_PRINTLN(st_min_str_thur);
  DEBUG_PRINTLN(en_hr_str_thur);
  DEBUG_PRINTLN(en_min_str_thur);

  DEBUG_PRINTLN(st_hr_str_fri);
  DEBUG_PRINTLN(st_min_str_fri);
  DEBUG_PRINTLN(en_hr_str_fri);
  DEBUG_PRINTLN(en_min_str_fri);

  DEBUG_PRINTLN(st_hr_str_sat);
  DEBUG_PRINTLN(st_min_str_sat);
  DEBUG_PRINTLN(en_hr_str_sat);
  DEBUG_PRINTLN(en_min_str_sat);

  DEBUG_PRINTLN(sun);
  DEBUG_PRINTLN(mon);
  DEBUG_PRINTLN(tue);
  DEBUG_PRINTLN(wed);
  DEBUG_PRINTLN(thur);
  DEBUG_PRINTLN(fri);
  DEBUG_PRINTLN(sat);
  DEBUG_PRINTLN(zone);
#endif

  if (client.connected())
  {
    client.publish(publish_status.c_str(), "ok");
#ifdef DEBUG_MODE
    DEBUG_PRINTLN(publish_status);
    DEBUG_PRINTLN("Feedback schedule sent...");
#endif 

  }
}

void read_eeprom()
{
	byte value;

  FlashMemory.read();
	for (int i = 0; i <= 35; i++)
	{
#ifdef DEBUG_MODE
		DEBUG_PRINTLN();
#endif
		value = FlashMemory.buf[i];
#ifdef DEBUG_MODE
		Serial.print("Data ");
		Serial.print(i);
		Serial.print(":");
		Serial.print(value);
		Serial.print("\t");
#endif

	}
}

void clear_eeprom()
{
  FlashMemory.read();
	for (int i = 0; i < 36; i++)
	{
		FlashMemory.buf[i] = 0;
	}
  FlashMemory.update();
}

void firmware_update(IPAddress ota_ip) {

  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINTLN("Starting firmware update...");
    Serial.print("OTA IP Address: ");
    DEBUG_PRINTLN(ota_ip);
    Serial.print("OTA Port: ");
    DEBUG_PRINTLN(OTA_PORT);

    if (OTA.beginCloud(ota_ip, OTA_PORT) == 0) {
      DEBUG_PRINTLN("Firmware update successful!");
    } else {
      DEBUG_PRINTLN("Firmware update failed!");
    }
  } else {
    DEBUG_PRINTLN("Not connected to WiFi");
  }
}
void publishFirmwareVersion() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastVersionPublishTime >= versionPublishInterval || lastVersionPublishTime == 0) {

    String versionTopic = uniqueID + "/Version";
    String versionPayload = String(device_generation) + "." + String(firmware_version);
    DEBUG_PRINTLN("Publishing firmware version...");
    Serial.print("Topic: ");
    DEBUG_PRINTLN(versionTopic);
    Serial.print("Payload: ");
    DEBUG_PRINTLN(versionPayload);
    client.publish(versionTopic.c_str(), versionPayload.c_str());
    lastVersionPublishTime = currentMillis;
  }
}

void compare_time()
{
  FlashMemory.read();
  int hour_set = timeClient.getHours() + timeZone;
  int minute_set = timeClient.getMinutes();
  int day = timeClient.getDay();

  int set_hour_sun, set_min_sun, end_hour_sun, end_min_sun;
  int set_hour_mon, set_min_mon, end_hour_mon, end_min_mon;
  int set_hour_tue, set_min_tue, end_hour_tue, end_min_tue;
  int set_hour_wed, set_min_wed, end_hour_wed, end_min_wed;
  int set_hour_thur, set_min_thur, end_hour_thur, end_min_thur;
  int set_hour_fri, set_min_fri, end_hour_fri, end_min_fri;
  int set_hour_sat, set_min_sat, end_hour_sat, end_min_sat;
  boolean mon, tue, wed, thur, fri, sat, sun;

  set_hour_sun = FlashMemory.buf[0];
  set_min_sun = FlashMemory.buf[1];
  end_hour_sun = FlashMemory.buf[2];
  end_min_sun = FlashMemory.buf[3];

  set_hour_mon = FlashMemory.buf[4];
  set_min_mon = FlashMemory.buf[5];
  end_hour_mon = FlashMemory.buf[6];
  end_min_mon = FlashMemory.buf[7];

  set_hour_tue = FlashMemory.buf[8];
  set_min_tue = FlashMemory.buf[9];
  end_hour_tue = FlashMemory.buf[10];
  end_min_tue = FlashMemory.buf[11];

  set_hour_wed = FlashMemory.buf[12];
  set_min_wed = FlashMemory.buf[13];
  end_hour_wed = FlashMemory.buf[14];
  end_min_wed = FlashMemory.buf[15];

  set_hour_thur = FlashMemory.buf[16];
  set_min_thur = FlashMemory.buf[17];
  end_hour_thur = FlashMemory.buf[18];
  end_min_thur = FlashMemory.buf[19];

  set_hour_fri = FlashMemory.buf[20];
  set_min_fri = FlashMemory.buf[21];
  end_hour_fri = FlashMemory.buf[22];
  end_min_fri = FlashMemory.buf[23];

  set_hour_sat = FlashMemory.buf[24];
  set_min_sat = FlashMemory.buf[25];
  end_hour_sat = FlashMemory.buf[26];
  end_min_sat = FlashMemory.buf[27];

  sun   = ((FlashMemory.buf[28]) == 1) ? true : false;
  mon   = ((FlashMemory.buf[29]) == 1) ? true : false;
  tue   = ((FlashMemory.buf[30]) == 1) ? true : false;
  wed   = ((FlashMemory.buf[31]) == 1) ? true : false;
  thur  = ((FlashMemory.buf[32]) == 1) ? true : false;
  fri   = ((FlashMemory.buf[33]) == 1) ? true : false;
  sat   = ((FlashMemory.buf[34]) == 1) ? true : false;
  
  DEBUG_PRINTLN("--------");
  DEBUG_PRINTLN(sun);
  DEBUG_PRINTLN(mon);
  DEBUG_PRINTLN(tue);
  DEBUG_PRINTLN(wed);
  DEBUG_PRINTLN(thur);
  DEBUG_PRINTLN(fri);
  DEBUG_PRINTLN(sat);
  DEBUG_PRINTLN("--------");


#ifdef DEBUG_MODE
  Serial.print("Checking Day: ");
  DEBUG_PRINTLN(day);
  Serial.print("Checking Hour: ");
  DEBUG_PRINTLN(hour_set);
  Serial.print("Checking Minute: ");
  DEBUG_PRINTLN(minute_set);
#endif // !DEBUG_MODE

  switch (day)
  {
    case 0:
      if (sun)
      {
        if (set_hour_sun == hour_set && set_min_sun == minute_set)
        {
          if (!status_on)
          {
            handleOn();
            status_on = true;
            status_off = false;
          }

        }
        else if (end_hour_sun == hour_set && end_min_sun == minute_set)
        {
          if (!status_off)
          {
            handleOff();
            status_on = false;
            status_off = true;
          }

        }
      }
      break;
    case 1:
      if (mon)
      {
        if (set_hour_mon == hour_set && set_min_mon == minute_set)
        {
          if (!status_on)
          {
            handleOn();
            status_on = true;
            status_off = false;
          }

        }
        else if (end_hour_mon == hour_set && end_min_mon == minute_set)
        {
          if (!status_off)
          {
            handleOff();
            status_on = false;
            status_off = true;
          }

        }
      }
      break;
    case 2:
      if (tue)
      {
        DEBUG_PRINTLN("1");
        DEBUG_PRINTLN(set_hour_tue);
        DEBUG_PRINTLN(set_min_tue);
        if (set_hour_tue == hour_set && set_min_tue == minute_set)
        {
          DEBUG_PRINTLN("2");
          if (!status_on)
          {
            handleOn();
            status_on = true;
            status_off = false;
          }

        }
        else if (end_hour_tue == hour_set && end_min_tue == minute_set)
        {
          DEBUG_PRINTLN("3");
          if (!status_off)
          {
            handleOff();
            status_on = false;
            status_off = true;
          }

        }
      }
      break;
    case 3:
      DEBUG_PRINTLN("Schedule Debug: 1");
      if (wed)
      {
        DEBUG_PRINTLN("Schedule Debug: 2");
        if (set_hour_wed == hour_set && set_min_wed == minute_set)
        {
          DEBUG_PRINTLN("Schedule Debug: 3");
          if (!status_on)
          {
            DEBUG_PRINTLN("Schedule Debug: 4");
            handleOn();
            status_on = true;
            status_off = false;
          }

        }
        else if (end_hour_wed == hour_set && end_min_wed == minute_set)
        {
          DEBUG_PRINTLN("Schedule Debug: 5");
          if (!status_off)
          {
            DEBUG_PRINTLN("Schedule Debug: 6");
            handleOff();
            status_on = false;
            status_off = true;
          }

        }
      }
      break;
    case 4:
      if (thur)
      {
        if (set_hour_thur == hour_set && set_min_thur == minute_set)
        {
          if (!status_on)
          {
            handleOn();
            status_on = true;
            status_off = false;
          }

        }
        else if (end_hour_thur == hour_set && end_min_thur == minute_set)
        {
          if (!status_off)
          {
            handleOff();
            status_on = false;
            status_off = true;
          }

        }
      }
      break;
    case 5:
      if (fri)
      {
        if (set_hour_fri == hour_set && set_min_fri == minute_set)
        {
          if (!status_on)
          {
            handleOn();
            status_on = true;
            status_off = false;
          }

        }
        else if (end_hour_fri == hour_set && end_min_fri == minute_set)
        {
          if (!status_off)
          {
            handleOff();
            status_on = false;
            status_off = true;
          }

        }
      }
      break;
    case 6:
      if (sat)
      {
        if (set_hour_sat == hour_set && set_min_sat == minute_set)
        {
          if (!status_on)
          {
            handleOn();
            status_on = true;
            status_off = false;
          }

        }
        else if (end_hour_sat == hour_set && end_min_sat == minute_set)
        {
          if (!status_off)
          {
            handleOff();
            status_on = false;
            status_off = true;
          }

        }
      }
      break;
    default:
      break;
  }
}

const char PROVISION_PAGE[] PROGMEM = R"====(
<!DOCTYPE html>
<html>
<head>
<title>WiFi Setup</title>
<style>
body { font-family: Arial; text-align: center; margin-top: 40px; }
input { padding: 10px; width: 80%; max-width: 300px; margin: 8px; }
button { padding: 12px 25px; font-size: 16px; }
</style>
</head>
<body>
<h2>BW16 WiFi Configuration</h2>
<form action="/save" method="POST">
<input name="ssid" placeholder="WiFi SSID"><br>
<input name="pass" placeholder="WiFi Password"><br><br>
<button type="submit">Save</button>
</form>
</body>
</html>
)====";


void startProvisioningMode() {
    DEBUG_PRINTLN("=== Starting AP Provisioning Mode ===");

    provisioningActive = true;

    WiFi.mode(WIFI_AP);
    WiFi.softAP("Device-Setup", "12345678");

    IPAddress ip = WiFi.softAPIP();
    DEBUG_PRINT("AP IP Address: ");
    DEBUG_PRINTLN(ip);

    // Register handlers BEFORE begin()
    provisioningServer.on("/", HTTP_GET, []() {
        provisioningServer.send(200, "text/html", PROVISION_PAGE);
    });

    provisioningServer.on("/save", HTTP_POST, []() {
        String ssid = provisioningServer.arg("ssid");
        String pass = provisioningServer.arg("pass");

        DEBUG_PRINT("Received SSID: "); DEBUG_PRINTLN(ssid);
        DEBUG_PRINT("Received PASS: "); DEBUG_PRINTLN(pass);

        saveCredentialsToFlash(ssid, pass);

        provisioningServer.send(200, "text/plain",
                               "Saved! Device will reboot...");

        delay(2000);
        NVIC_SystemReset();
    });

    provisioningServer.begin();

    DEBUG_PRINTLN("Provisioning server started successfully.");
}




void handleProvisionPortal() {
    provisioningServer.send(200, "text/html", PROVISION_PAGE);
}

void handleProvisionSave() {
    if (!provisioningServer.hasArg("ssid") || !provisioningServer.hasArg("pass")) {
        provisioningServer.send(400, "text/plain", "Missing SSID or Password");
        return;
    }

    String ssid = provisioningServer.arg("ssid");
    String pass = provisioningServer.arg("pass");

    DEBUG_PRINT("Received SSID: "); DEBUG_PRINTLN(ssid);
    DEBUG_PRINT("Received PASS: "); DEBUG_PRINTLN(pass);

    saveCredentialsToFlash(ssid, pass);

    provisioningServer.send(200, "text/plain",
        "Credentials saved! Device will reboot in 3 seconds.");

    delay(3000);
    ESP.restart();   // on BW16 reset MCU
}

void handleNotFound() {
    provisioningServer.send(404, "text/plain", "Not found");
}



#define device_generation 1
#define firmware_version 14
#define DEBUG
#include "config.h"
#include "string.h"
//Include libraries
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#define DeviceGeneration
// Object Declaration
WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WiFiMulti WiFiMulti;
Ticker tasker;

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 4
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
DeviceAddress mainThermometer;


// Function Prototyping
void set_temp(int knob_input);
#define analogInPin A0
int sensorValue = 0;
int outputValue = 0;
int mappedOutput = 0;

int chrisPin1 = 12;
int chrisPin2 = 13;
int transistorPin = 14;
int default_temp = 30;
float temp_c;
int input = 0;
int calibration_offset = 5;
bool heating = false;
// Connection_Status
bool Connection_Status = 0;
int Connection_Counter = 0;

//MQTT Connection Status
bool MQTT_Status = 0;

// Mqtt Server Address
const char* mqttServer = "167.71.112.188";

// Mqtt Server Port
const int mqttPort = 1883;

// Credentials to connect
const char* mqttUser = "thenetzoptimize";
const char* mqttPassword = "!@#mqtt";

//strDateTime dateTime;

boolean status_on = false;
boolean status_off = false;

#define SAVED_STATUS_ADDRESS 64

// Topics Data
String uniqueID ;
//char* publish_status;                       // Status"
//char* subscribe_action;                     // Action"
//char* subscribe_schedule;                   // Schedule";
//char* subscribe_autoshutdown;               // Autoshutdown";
//char* subscribe_upgrade;                    // Upgrade";
char* clientID_value;
char actionBuf[30], scheduleBuf[30], autoshutDownBuf[30], upgradeBuf[30], statusBuf[30], debugBuf[30];

const char* publish_status = "003915A1/Status";
const char* subscribe_action = "003915A1/Action";
const char* subscribe_schedule = "003915A1/Schedule";
const char* subscribe_autoshutdown = "003915A1/Autoshutdown";
const char* subscribe_upgrade = "003915A1/Upgrade";
const char* publish_debug = "003915A1/Debug";

// NTP Time Protocol Variable Server --- Start
// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
int timeZone;  // Pacific Daylight Time (USA)

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// NTP Time Protocol Variable Server --- Stop
// These variables can be changed:

// Firmware Version
String current_version = "1.235";
String firm_version;
int update_status = 0;
int check_update = 1;

//safety shutoff
bool wifiOnce = false;

/************ Heating Control Variables **************/
long lastHeatingCycle = 0;
#define heatingCycleOn 200
#define heatingCycleDuration 30000 

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(512);

  pinMode(RELAY, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(transistorPin, OUTPUT);
  pinMode(chrisPin1, OUTPUT);
  pinMode(chrisPin2, OUTPUT);

//  analogReference(EXTERNAL);
  // Define GPIO Pin state here
  //    Setting up auto devID fetch from ChipID
  String chipID = String(ESP.getChipId(), HEX);
  chipID.toUpperCase();
  uint len = chipID.length();
  if (len < 8)
  {
    len = 8 - len;
    for (int i = 1; i <= len; i++)
    {
      uniqueID += "0";
    }
  }
  uniqueID = uniqueID + chipID;
  char clientID[30];
  (uniqueID + "123hjuy").toCharArray(clientID, 30);
  clientID_value = clientID;

  WiFi.mode(WIFI_STA);
  Serial.println("*********************************************");
  Serial.print("Device Id: ");
  Serial.println(uniqueID);
  Serial.print("Device Iteration: ");
  Serial.println(device_generation);
  Serial.print("Firmware Version: ");
  Serial.println(firmware_version);
  Serial.println("*********************************************");

  int reason = ESP.getResetInfoPtr()->reason;
  Serial.print("Reset Reason: ");
  Serial.println(reason);
  if(reason != 0 || reason != 6){
    int lastStatus = EEPROM.read(SAVED_STATUS_ADDRESS);
    if(lastStatus == 0){
      handleOff();
    }
    else{
      handleOn();
    }
  }
  else{
    handleOn();
  }

  sensors.begin();

  if (!sensors.getAddress(mainThermometer, 0)) Serial.println("Unable to find address for Device 0");

  //WiFi.disconnect();
}

// Add the main program code into the continuous loop() function
void loop()
{
  yield();
  if (WiFi.status() == WL_CONNECTED)
  {
    Connection_Status = 1;
    wifiOnce = true;
#ifdef DEBUG_MODE
    Serial.println("WiFi Active");
#endif
  }
  else {
    blink_led(1);
    Connection_Status = 0;
    MQTT_Status = 0;
    if (Connection_Counter <= 70)
    {
#ifdef DEBUG_MODE
      Serial.println("Waiting For Wifi");
#endif
      digitalWrite(LED_STATUS, HIGH);
      delay(1000);
      digitalWrite(LED_STATUS, LOW);
      if (WiFi.smartConfigDone()) {
        Connection_Status = 1;
        Connection_Counter = 0;
#ifdef DEBUG_MODE
        Serial.println("Smartconfig Success...");
#endif
      }
      if (Connection_Counter == 10) {
        WiFi.beginSmartConfig();
#ifdef DEBUG_MODE
        Serial.println("Begin Smart Config");
#endif
      }
      if (Connection_Counter == 70) {
        WiFi.stopSmartConfig();
#ifdef DEBUG_MODE
        Serial.println("Give Up on Smart Config");
#endif
      }
      Connection_Counter++;
    }
  }


  if (Connection_Status == 1)
  {
    if (MQTT_Status == 0) {
      setupMQTT();
      MQTT_Status = 1;
    }
    else{
      routine_task();
    }

    boolean connected = client.loop();
    if(!connected){
      reconnect();
    }

    if (check_update == 1)
    {
      firmware_update();
      //Serial.println("Upgrading Firmware");
      check_update = 0;
    }
  }



  sensorValue = analogRead(analogInPin) / 32;
  client.publish(publish_debug, String("Knob Value" + String(analogRead(analogInPin))).c_str());
  Serial.println(analogRead(analogInPin));
  mappedOutput = map(sensorValue, 0, 25, 0, 10);
  outputValue = constrain(mappedOutput, 0, 10);
  
#ifdef DEBUG_MODE
  Serial.print("Pot value: ");
  Serial.print(outputValue);
#endif

  //  // Send the command to get temperatures
  //  sensors.requestTemperatures();
  //  temp_c = sensors.getTempCByIndex(0);
  //#ifdef DEBUG_MODE
  //  Serial.print("  Temperature is: ");
  //  Serial.println(temp_c);
  //#endif
  //    set_temp(outputValue);

  sensors.setResolution(9);
  sensors.requestTemperatures();
  int n = 0;
  // wait until sensor ready, do some counting for fun.
  while (!sensors.isConversionComplete()) n++;
  temp_c = sensors.getTempC(mainThermometer);
#ifdef DEBUG_MODE
  Serial.print("  Temperature is: ");
  Serial.println(temp_c);
#endif
  set_temp(outputValue);

  delay(1000);
  yield();
}

void setupMQTT() {
  // MQTT Configuration
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
#ifdef DEBUG_MODE
    Serial.println("Connecting to MQTT...");
#endif

    if (client.connect(clientID_value, mqttUser, mqttPassword)) {

#ifdef DEBUG_MODE
      Serial.println("connected");
#endif
    }
    else {
#ifdef DEBUG_MODE
      Serial.print("failed with state ");
      Serial.print(client.state());
#endif
      delay(2000);
      yield();
    }
  }

  Udp.begin(localPort);
  int currentZone;
  currentZone = EEPROM.read(35);
  EEPROM.commit();
  if (currentZone > 10 || currentZone < 3)
  {
    currentZone = 7;
  }
  int timechange = currentZone * 2;
  timeZone = currentZone - timechange;

#ifdef DEBUG_MODE
  Serial.println(currentZone);
  Serial.println(timeZone);
#endif


  (uniqueID + "/Action").toCharArray(actionBuf, 30);
  (uniqueID + "/Schedule").toCharArray(scheduleBuf, 30);
  (uniqueID + "/Autoshutdown").toCharArray(autoshutDownBuf, 30);
  (uniqueID + "/Upgrade").toCharArray(upgradeBuf, 30);
  (uniqueID + "/Status").toCharArray(statusBuf, 16);
  (uniqueID + "/Debug").toCharArray(debugBuf, 16);

  publish_status = statusBuf;
  subscribe_action = actionBuf;
  subscribe_schedule = scheduleBuf;
  subscribe_autoshutdown = autoshutDownBuf;
  subscribe_upgrade = upgradeBuf;
  publish_debug = debugBuf;
  

#ifdef DEBUG_MODE
  Serial.println("STARTING TASKER");
#endif
  client.publish(publish_status, "0");
  client.subscribe(subscribe_action);
  client.subscribe(subscribe_schedule);
  client.subscribe(subscribe_autoshutdown);
  client.subscribe(subscribe_upgrade);

  setSyncProvider(getNtpTime);
  setSyncInterval(10);
}

// This function helps to avoid a problem with the latest Dallas temperature library.
int16_t millisToWaitForConversion(uint8_t bitResolution)
{
  switch (bitResolution) {
    case 9:
      return 94;
    case 10:
      return 188;
    case 11:
      return 375;
    default:
      return 750;
  }
}

void routine_task(void)
{
#ifdef DEBUG_MODE
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
  Serial.print("--");
  Serial.print(weekday());
  Serial.println();
#endif

  int pinStatus = digitalRead(RELAY);
  if (pinStatus == 1)
  {
    updateMqtt(1);
  }
  else
  {
    updateMqtt(0);
  }

#ifdef DEBUG_MODE
  Serial.println("Checking Schedule");
#endif

  compare_time();
}

void pincheck_task()
{

}

void callback(char* topic, byte* payload, unsigned int length) {

  int case_state = 0;
  String data(topic);
  char char_recieve;
  String str_recieve;

#ifdef DEBUG_MODE
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.println(data);
#endif

  if (data == subscribe_action)
  {
    case_state = 1;
  }
  else if (data == subscribe_schedule)
  {
    case_state = 2;
  }
  else if (data == subscribe_autoshutdown)
  {
    case_state = 3;
  }
  else if (data == subscribe_upgrade)
  {
    case_state = 4;
  }

  for (int i = 0; i < length; i++) {
    char_recieve = (char)payload[i];
    str_recieve += char_recieve;
  }

  switch (case_state)
  {
    case 1:
      switch ((char)payload[0])
      {
        case '0':
          handleOff();
          EEPROM.write(SAVED_STATUS_ADDRESS, 0);
          EEPROM.commit();
          if (digitalRead(RELAY) == 1)
          {
            updateMqtt(0);
          }

          break;

        case '1':
          handleOn();
          EEPROM.write(SAVED_STATUS_ADDRESS, 1);
          EEPROM.commit();
          if (digitalRead(RELAY) == 0)
          {
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
      if ((int)payload[0])
      {
        check_update = 1;
      }

      break;
    default:
      break;
  }

#ifdef DEBUG_MODE
  Serial.println();
  Serial.print("Message:");
  Serial.println(str_recieve);
  Serial.println();
  Serial.println("-----------------------");
#endif

}

//############################################
// MQTT 
// routine
//############################################
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
#ifdef DEBUG_MODE
    Serial.print("MQTT reconnect...");
#endif
    // Attempt to connect
    if (client.connect(clientID_value, mqttUser, mqttPassword))
    {
#ifdef DEBUG_MODE
      Serial.println("connected!");
#endif
      client.subscribe(subscribe_action);
      client.subscribe(subscribe_schedule);
      client.subscribe(subscribe_autoshutdown);
    }
    else
    {
#ifdef DEBUG_MODE
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 s");
#endif
      // Wait 5 seconds before retrying
      //delay(5000);
      yield();
    }
  }
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
  setSyncProvider(getNtpTime);
  setSyncInterval(300);


  for (int i = 0; i < 36; i++)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.write(0, st_hr_int_sun);
  EEPROM.write(1, st_min_int_sun);
  EEPROM.write(2, en_hr_int_sun);
  EEPROM.write(3, en_min_int_sun);

  EEPROM.write(4, st_hr_int_mon);
  EEPROM.write(5, st_min_int_mon);
  EEPROM.write(6, en_hr_int_mon);
  EEPROM.write(7, en_min_int_mon);

  EEPROM.write(8, st_hr_int_tue);
  EEPROM.write(9, st_min_int_tue);
  EEPROM.write(10, en_hr_int_tue);
  EEPROM.write(11, en_min_int_tue);

  EEPROM.write(12, st_hr_int_wed);
  EEPROM.write(13, st_min_int_wed);
  EEPROM.write(14, en_hr_int_wed);
  EEPROM.write(15, en_min_int_wed);

  EEPROM.write(16, st_hr_int_thur);
  EEPROM.write(17, st_min_int_thur);
  EEPROM.write(18, en_hr_int_thur);
  EEPROM.write(19, en_min_int_thur);

  EEPROM.write(20, st_hr_int_fri);
  EEPROM.write(21, st_min_int_fri);
  EEPROM.write(22, en_hr_int_fri);
  EEPROM.write(23, en_min_int_fri);

  EEPROM.write(24, st_hr_int_sat);
  EEPROM.write(25, st_min_int_sat);
  EEPROM.write(26, en_hr_int_sat);
  EEPROM.write(27, en_min_int_sat);

  EEPROM.write(28, sun_int);
  EEPROM.write(29, mon_int);
  EEPROM.write(30, tue_int);
  EEPROM.write(31, wed_int);
  EEPROM.write(32, thur_int);
  EEPROM.write(33, fri_int);
  EEPROM.write(34, sat_int);
  EEPROM.write(35, currentZone);
  EEPROM.commit();

#ifdef DEBUG_MODE
  Serial.println();
  Serial.println(st_hr_str_sun);
  Serial.println(st_min_str_sun);
  Serial.println(en_hr_str_sun);
  Serial.println(en_min_str_sun);

  Serial.println(st_hr_str_mon);
  Serial.println(st_min_str_mon);
  Serial.println(en_hr_str_mon);
  Serial.println(en_min_str_mon);

  Serial.println(st_hr_str_tue);
  Serial.println(st_min_str_tue);
  Serial.println(en_hr_str_tue);
  Serial.println(en_min_str_tue);

  Serial.println(st_hr_str_wed);
  Serial.println(st_min_str_wed);
  Serial.println(en_hr_str_wed);
  Serial.println(en_min_str_wed);

  Serial.println(st_hr_str_thur);
  Serial.println(st_min_str_thur);
  Serial.println(en_hr_str_thur);
  Serial.println(en_min_str_thur);

  Serial.println(st_hr_str_fri);
  Serial.println(st_min_str_fri);
  Serial.println(en_hr_str_fri);
  Serial.println(en_min_str_fri);

  Serial.println(st_hr_str_sat);
  Serial.println(st_min_str_sat);
  Serial.println(en_hr_str_sat);
  Serial.println(en_min_str_sat);

  Serial.println(sun);
  Serial.println(mon);
  Serial.println(tue);
  Serial.println(wed);
  Serial.println(thur);
  Serial.println(fri);
  Serial.println(sat);
  Serial.println(zone);
#endif

  if (client.connected())
  {
    client.publish(publish_status, "ok");
#ifdef DEBUG_MODE
    Serial.println(publish_status);
    Serial.println("Feedback schedule sent...");
#endif // !DEBUG_MODE

  }
}

void compare_time()
{
  // first parameter: Time zone; second parameter: 1 for European summer time; 2 for US daylight saving time (not implemented yet)
  //dateTime = NTPch.getTime(-1, 1); // get time from internal clock
  //NTPch.printDateTime(dateTime);

  int hour_set = hour();
  int minute_set = minute();
  int day = weekday();

  int set_hour_sun, set_min_sun, end_hour_sun, end_min_sun;
  int set_hour_mon, set_min_mon, end_hour_mon, end_min_mon;
  int set_hour_tue, set_min_tue, end_hour_tue, end_min_tue;
  int set_hour_wed, set_min_wed, end_hour_wed, end_min_wed;
  int set_hour_thur, set_min_thur, end_hour_thur, end_min_thur;
  int set_hour_fri, set_min_fri, end_hour_fri, end_min_fri;
  int set_hour_sat, set_min_sat, end_hour_sat, end_min_sat;
  boolean mon, tue, wed, thur, fri, sat, sun;

  set_hour_sun = EEPROM.read(0);
  set_min_sun = EEPROM.read(1);
  end_hour_sun = EEPROM.read(2);
  end_min_sun = EEPROM.read(3);

  set_hour_mon = EEPROM.read(4);
  set_min_mon = EEPROM.read(5);
  end_hour_mon = EEPROM.read(6);
  end_min_mon = EEPROM.read(7);

  set_hour_tue = EEPROM.read(8);
  set_min_tue = EEPROM.read(9);
  end_hour_tue = EEPROM.read(10);
  end_min_tue = EEPROM.read(11);

  set_hour_wed = EEPROM.read(12);
  set_min_wed = EEPROM.read(13);
  end_hour_wed = EEPROM.read(14);
  end_min_wed = EEPROM.read(15);

  set_hour_thur = EEPROM.read(16);
  set_min_thur = EEPROM.read(17);
  end_hour_thur = EEPROM.read(18);
  end_min_thur = EEPROM.read(19);

  set_hour_fri = EEPROM.read(20);
  set_min_fri = EEPROM.read(21);
  end_hour_fri = EEPROM.read(22);
  end_min_fri = EEPROM.read(23);

  set_hour_sat = EEPROM.read(24);
  set_min_sat = EEPROM.read(25);
  end_hour_sat = EEPROM.read(26);
  end_min_sat = EEPROM.read(27);

  sun   = ((EEPROM.read(28)) == 1) ? true : false;
  mon   = ((EEPROM.read(29)) == 1) ? true : false;
  tue   = ((EEPROM.read(30)) == 1) ? true : false;
  wed   = ((EEPROM.read(31)) == 1) ? true : false;
  thur  = ((EEPROM.read(32)) == 1) ? true : false;
  fri   = ((EEPROM.read(33)) == 1) ? true : false;
  sat   = ((EEPROM.read(34)) == 1) ? true : false;
  EEPROM.commit();

#ifdef DEBUG_MODE
  Serial.print("Checking Day: ");
  Serial.println(day);
#endif // !DEBUG_MODE

  switch (day)
  {
    case 1:
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
    case 2:
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
    case 3:
      if (tue)
      {
        if (set_hour_tue == hour_set && set_min_tue == minute_set)
        {
          if (!status_on)
          {
            handleOn();
            status_on = true;
            status_off = false;
          }

        }
        else if (end_hour_tue == hour_set && end_min_tue == minute_set)
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
    case 4:
      Serial.println("Schedule Debug: 1");
      if (wed)
      {
        Serial.println("Schedule Debug: 2");
        if (set_hour_wed == hour_set && set_min_wed == minute_set)
        {
          Serial.println("Schedule Debug: 3");
          if (!status_on)
          {
            Serial.println("Schedule Debug: 4");
            handleOn();
            status_on = true;
            status_off = false;
          }

        }
        else if (end_hour_wed == hour_set && end_min_wed == minute_set)
        {
          Serial.println("Schedule Debug: 5");
          if (!status_off)
          {
            Serial.println("Schedule Debug: 6");
            handleOff();
            status_on = false;
            status_off = true;
          }

        }
      }
      break;
    case 5:
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
    case 6:
      //      Serial.println("Schedule Debug: 1");
      if (fri)
      {
        //        Serial.println("Schedule Debug: 2");
        if (set_hour_fri == hour_set && set_min_fri == minute_set)
        {
          //          Serial.println("Schedule Debug: 3");
          if (!status_on)
          {
            //            Serial.println("Schedule Debug: 4");
            handleOn();
            status_on = true;
            status_off = false;
          }

        }
        else if (end_hour_fri == hour_set && end_min_fri == minute_set)
        {
          //          Serial.println("Schedule Debug: 5");
          if (!status_off)
          {
            //            Serial.println("Schedule Debug: 6");
            handleOff();
            status_on = false;
            status_off = true;
          }

        }
      }
      break;
    case 7:
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

void updateMqtt(int status)
{
  
  const char* update;
  if (status == 0) {
    update = "0";
  }
  else {
    update = "1";
  }
  Serial.println(publish_status);

  if (client.connected())
  {
    char copy [16];
     
    client.publish(publish_status, update);
  }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0); // discard any previously received packets
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
#ifdef DEBUG_MODE
      Serial.print("Timezone: ");
      Serial.print(timeZone);
      Serial.println();
#endif
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
#ifdef DEBUG_MODE
  Serial.println("No NTP Response :-(");
#endif
  return 0; // return 0 if unable to get the time
}

void set_temp(int knob_input)
{
  if (temp_c == DEVICE_DISCONNECTED_C) {
    Serial.println("No Thermometer");
    heatOff();
    return;
  }
  if(wifiOnce && !Connection_Status){
    Serial.println("Disconnect Detected");
    heatOff();
    return;
  }
  client.publish(publish_debug, String(temp_c - calibration_offset).c_str());
  client.publish(publish_debug, String(knob_input).c_str());

  Serial.println("Set Temp");
  switch (knob_input)
  {
    case 1:
      if (temp_c <= 30 + calibration_offset)
      {
        // Turn on the mosfet
         heat();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 1 ON");
#endif
      }
      else
      {
        // Turn off the mosfet
        heatOff();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 1 OFF");
#endif
      }
      break;
    case 2:
      if (temp_c <= 36 + calibration_offset)
      {
        // Turn on the mosfet
         heat();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 2 ON");
#endif
      }
      else
      {
        // Turn off the mosfet
        heatOff();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 2 OFF");
#endif
      }
      break;
    case 3:
      if (temp_c <= 42 + calibration_offset + 5) //tuning for april
      {
        // Turn on the mosfet
         heat();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 3 ON");
#endif
      }
      else
      {
        // Turn off the mosfet
        heatOff();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 3 OFF");
#endif
      }
      break;
    case 4:
      if (temp_c <= 48 + calibration_offset)
      {
        // Turn on the mosfet
         heat();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 4 ON");
#endif
      }
      else
      {
        // Turn off the mosfet
        heatOff();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 4 OFF");
#endif
      }
      break;
    case 5:
      if (temp_c <= 54 + calibration_offset + 5) //turning for april
      {
        // Turn on the mosfet
         heat();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 5 ON");
#endif
      }
      else
      {
        // Turn off the mosfet
        heatOff();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 5 OFF");
#endif
      }
      break;

    case 6:
      if (temp_c <= 60 + calibration_offset)
      {
        // Turn on the mosfet
         heat();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 6 ON");
#endif
      }
      else
      {
        // Turn off the mosfet
        heatOff();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 6 OFF");
#endif
      }
      break;

    case 7:
      if (temp_c <= 66 + calibration_offset)
      {
        // Turn on the mosfet
         heat();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 7 ON");
#endif
      }
      else
      {
        // Turn off the mosfet
        heatOff();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 7 OFF");
#endif
      }
      break;

    case 8:
      if (temp_c <= 72 + calibration_offset)
      {
        // Turn on the mosfet
         heat();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 8 ON");
#endif
      }
      else
      {
        // Turn off the mosfet
        heatOff();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 8 OFF");
#endif
      }
      break;

    case 9:
      if (temp_c <= 82 + calibration_offset)
      {
        // Turn on the mosfet
         heat();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 9 ON");
#endif
      }
      else
      {
        // Turn off the mosfet
        heatOff();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 9 OFF");
#endif
      }
      break;

    case 10:
      if (temp_c <= 90 + calibration_offset)
      {
        // Turn on the mosfet
         heat();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 10 ON");
#endif
      }
      else
      {
        // Turn off the mosfet
        heatOff();
#ifdef DEBUG_MODE
        Serial.println("Mosfet Case 10 OFF");
#endif
      }
      break;

    default:
      if (temp_c <= default_temp)
      {
        // Turn on the mosfet
         heat();
#ifdef DEBUG_MODE
        Serial.println("Mosfet default ON");
#endif
      }
      else
      {
        // Turn off the mosfet
        heatOff();
#ifdef DEBUG_MODE
        Serial.println("Mosfet default OFF");
#endif
      }
      break;
  }
  Serial.print("Heater Pin Value: ");
  Serial.println(digitalRead(transistorPin));

}

void heat(){
//    long now = millis();
//    if(now - lastHeatingCycle > heatingCycleDuration){
//      Serial.println("Heater On");
//      digitalWrite(transistorPin, HIGH);
//      lastHeatingCycle = now;
//    }
//    else if(now - lastHeatingCycle > 1000){
//      Serial.println("Heater Off");
//      digitalWrite(transistorPin, LOW);
//      digitalWrite(RELAY, LOW);
//    }
/*  digitalWrite(transistorPin, HIGH);
    digitalWrite(chrisPin1, LOW);
    digitalWrite(chrisPin2, LOW); */
    digitalWrite(RELAY, HIGH);    // ON (active-LOW relay)
    Serial.println("Heater: ON");
}

void heatOff(){
  /*digitalWrite(transistorPin, LOW);
  digitalWrite(chrisPin1, HIGH);
  digitalWrite(chrisPin2, HIGH); */
  digitalWrite(RELAY, LOW);    // ON (active-LOW relay)
  Serial.println("Heater: OFF");
//  if(digitalRead(transistorPin) == HIGH){
////    digitalWrite(RELAY, LOW);
////    delay(25);
////    digitalWrite(RELAY, HIGH); 
//  }
}

void firmware_update()
{
  Serial.println("CHECKING_FOR_UPDATES");
  if (WiFi.status() == WL_CONNECTED) {
    BearSSL::WiFiClientSecure UpdateClient;
    UpdateClient.setInsecure();
    String url = String("https://sfo2.digitaloceanspaces.com/mellafirmware/firmware/" + String(device_generation) + "/" + String(firmware_version + 1) + "/MellaFirmware.bin");
    //String url = String("http://192.168.50.168:8081/" + String(device_generation) + "/" + String(firmware_version + 1) + "/MellaFirmware.bin");
    Serial.println(url);
    digitalWrite(LED_STATUS, HIGH);
    t_httpUpdate_return ret = ESPhttpUpdate.update(UpdateClient, url);
    digitalWrite(LED_STATUS, LOW);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
#ifdef DEBUG_MODE
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
#endif
        break;

      case HTTP_UPDATE_NO_UPDATES:
#ifdef DEBUG_MODE
        Serial.println("HTTP_UPDATE_NO_UPDATES");
#endif
        break;

      case HTTP_UPDATE_OK:
#ifdef DEBUG_MODE
        Serial.println("HTTP_UPDATE_OK");
#endif
        break;
    }
  }
}

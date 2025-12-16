// config.h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#define DEBUG_MODE
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include "PubSubClient.h"
#include "control.h"
//#include <SNTPtime.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#define STATUS "Status" 
#define SCHEDULE "Schedule"
#define AUTOSHUTDOWN "Autoshutdown"

#define SSID		"Dan7e"
#define Password	"!@3Draciel"

#define RELAY		5
#define	LED_STATUS	10

#endif

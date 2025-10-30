// 
// 
// 

#include "control.h"
#include "config.h"

#define DEBUG_MODE
void handleOn()
{
#ifdef DEBUG_MODE
	Serial.println("Relay ON");
#endif
	digitalWrite(RELAY, HIGH);
}

void handleOff()
{
#ifdef DEBUG_MODE
	Serial.println("Relay OFF");
#endif
	digitalWrite(RELAY, LOW);
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
}

void read_eeprom()
{
	byte value;

	EEPROM.begin(36);
	for (int i = 0; i <= 35; i++)
	{
#ifdef DEBUG_MODE
		Serial.println();
#endif
		value = EEPROM.read(i);
#ifdef DEBUG_MODE
		Serial.print("Data ");
		Serial.print(i);
		Serial.print(":");
		Serial.print(value);
		Serial.print("\t");
#endif

	}
	EEPROM.end();
}

void clear_eeprom()
{
	EEPROM.begin(36);
	for (int i = 0; i < 36; i++)
	{
		EEPROM.write(i, 0);
	}
	EEPROM.commit();
}

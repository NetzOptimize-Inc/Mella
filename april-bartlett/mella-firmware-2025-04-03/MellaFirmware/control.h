// control.h

#ifndef _CONTROL_h
#define _CONTROL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

void handleOn();
void handleOff();
void blink_led(int time);
void read_eeprom();
void clear_eeprom();


#endif

// handlingWeb.h

#ifndef _HANDLINGWEB_h
#define _HANDLINGWEB_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <IotWebConfAsync.h>
#include <WebSerial.h>
#include "Scene.h"
#include "heater.h"
#include "NTPSettings.h"

#define SERIAL_WEB_SERIAL(...) \
    do { \
        Serial.print(__VA_ARGS__); \
        WebSerial.print(__VA_ARGS__); \
    } while(0)

#define SERIAL_WEB_SERIALLN(...) \
    do { \
        Serial.println(__VA_ARGS__); \
        WebSerial.println(__VA_ARGS__); \
    } while(0)
#define SERIAL_WEB_SERIALF(fmt, ...) \
    do { \
        Serial.printf(fmt, __VA_ARGS__); \
        char _buf[128]; \
        snprintf(_buf, sizeof(_buf), fmt, __VA_ARGS__); \
        WebSerial.print(_buf); \
    } while(0)

extern bool ShouldReboot;
extern bool ConfigChanged;

extern AsyncIotWebConf iotWebConf;

void setupWebHandling();
void loopWebHandling();

#endif


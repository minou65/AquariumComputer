// handlingWeb.h

#ifndef _HANDLINGWEB_h
#define _HANDLINGWEB_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <IotWebConfAsync.h>
#include "Scene.h"
#include "heater.h"
#include "NTPSettings.h"

extern bool ShouldReboot;
extern bool ConfigChanged;

extern AsyncIotWebConf iotWebConf;

void setupWebHandling();
void loopWebHandling();

#endif


// handlingWeb.h

#ifndef _HANDLINGWEB_h
#define _HANDLINGWEB_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <IotWebConfAsync.h>

extern bool ShouldReboot;
extern bool ConfigChanged;

extern AsyncIotWebConf iotWebConf;

void setupWebHandling();
void loopWebHandling();

#endif


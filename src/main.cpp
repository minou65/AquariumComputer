/*
 Name:		ESP32_AquariumComputer.ino
 Created:	14.12.2025 15:52:45
 Author:	andy

 WEMOS D1 MINI ESP32
*/

#include "HeapMonitor.h"
#include <Arduino.h>

// Build-local candidate keeps failed builds from changing src/version.h.
#include <build_version.h>
#include "NTPSettings.h"
#include "ntp.h"
#include "Scene.h"
#include "heater.h"
#include "handlingWeb.h"
#include "neotimer.h"

#include <EventLogger.h>

char Version[] = VERSION_STR; // Manufacturer's Software version code

static Neotimer sceneTimer = Neotimer(5000); // 5 Sekunden
NTPClient ntpClient;

void setup() {
	Serial.begin(115200);
	while (!Serial) {
		delay(1);
	}

	eventLogger.logf(EventLogger::Status::INFO, "AquariumComputer v%s started", Version);
	
	setupSceneHardware();
	setupHeaterHardware();
	setupWebHandling();

	eventLogger.log("Setting up NTP client...", EventLogger::Status::INFO);
	if (ntpConfig.useNtpServer()) {
		ntpClient.begin(ntpConfig.ntpServer(), ntpConfig.timeZone(), 0);
		eventLogger.logf(EventLogger::Status::INFO, 1, "NTP server: %s, Timezone: %d", ntpConfig.ntpServer(), ntpConfig.timeZone());
	}
	else {
		eventLogger.log("NTP not used", EventLogger::Status::INFO, 1);
	}

	beginHeapMonitor();
	setHeapWarningThresholds(30000, 90);
}

void loop() {
	loopHeapMonitor();
	loopWebHandling();
	loopHeater();

	if (iotWebConf.getState() == iotwebconf::OnLine) {
		if (ntpClient.isInitialized()) {
			ntpClient.process();
		}
	}
	
	if (sceneTimer.repeat() || updateOutputs || ConfigChanged) {
		int minutes_ = ntpClient.getMinutesSinceMidnight();
		applyScenes(minutes_);
		updateOutputs = false;
	}

	if (ConfigChanged){
		if (ntpConfig.useNtpServer()) {
			ntpClient.begin(ntpConfig.ntpServer(), ntpConfig.timeZone(), 0);
		}

		ConfigChanged = false;
		eventLogger.log("Configuration changed, NTP client reinitialized", EventLogger::Status::OK);
	}

	if (ShouldReboot) {
		eventLogger.log("Rebooting...", EventLogger::Status::DEBUG);

		// Save time before reboot
		if (ntpClient.isInitialized() && ntpClient.isValidTime()) {
			ntpClient.saveTimeBeforeReboot();
		}

		delay(1000);
		ESP.restart();
	}


}

/*
 Name:		ESP32_AquariumComputer.ino
 Created:	14.12.2025 15:52:45
 Author:	andy

 WEMOS D1 MINI ESP32
*/

#include <Arduino.h>

#include "version.h"
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

	eventLogger.logf("AquariumComputer v%s started", Version, EventLogger::Status::INFO);
	
	setupSceneHardware();
	eventLogger.log("Scene hardware setup complete", EventLogger::Status::OK);

	setupHeaterHardware();
	eventLogger.log("Heater hardware setup complete", EventLogger::Status::OK);

	setupWebHandling();
	eventLogger.log("Web handling setup complete", EventLogger::Status::OK);

	eventLogger.log("Setting up NTP client...", EventLogger::Status::INFO);
	if (ntpConfig.useNtpServer()) {
		ntpClient.begin(ntpConfig.ntpServer(), ntpConfig.timeZone(), 0);
	}
	else {
		eventLogger.log("NTP not used", EventLogger::Status::INFO);
	}
}

void loop() {
	loopWebHandling();

	if (iotWebConf.getState() == iotwebconf::OnLine) {
		if (ntpClient.isInitialized()) {
			ntpClient.process();
		}
	}

	loopHeater();
	
	if (sceneTimer.repeat() || updateOutputs || ConfigChanged) {
		Scene* scene_ = &scenes[0];
		int minutes_ = ntpClient.getMinutesSinceMidnight();;

		while (scene_ != nullptr) {
			scene_->setCurrentScene(minutes_);
			scene_ = (Scene*)scene_->getNext();
		}
		updateOutputs = false;
	}

	if (ConfigChanged){
		Serial.println("Configuration changed, saving to flash and rebooting...");

		if (ntpConfig.useNtpServer()) {
			ntpClient.begin(ntpConfig.ntpServer(), ntpConfig.timeZone(), 0);
		}

		ConfigChanged = false;
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

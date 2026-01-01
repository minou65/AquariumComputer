/*
 Name:		ESP32_AquariumComputer.ino
 Created:	14.12.2025 15:52:45
 Author:	andy
*/

#include <Arduino.h>

#include "version.h"
#include "NTPSettings.h"
#include "ntp.h"
#include "Scene.h"
#include "heater.h"
#include "handlingWeb.h"
#include "neotimer.h"

char Version[] = VERSION_STR; // Manufacturer's Software version code

static Neotimer sceneTimer = Neotimer(60000); // 60 Sekunden
NTPClient ntpClient;

uint8_t speedFactor = 1;
static unsigned long startMillis = 0;
static int scaledMinutesOffset = 0;

void setup() {
	Serial.begin(115200);
	while (!Serial) {
		delay(1);
	}
	Serial.println("AquariumComputer v" + String(Version) + " started");
	
	setupSceneHardware();
	setupHeaterHardware();
	setupWebHandling();

	updateSceneTimer();

	if (ntpConfig.useNtpServer()) {
		ntpClient.begin(ntpConfig.ntpServer(), ntpConfig.timeZone(), 0);
	}
	else {
		Serial.println(F("NTP not used"));
	}
}

void setScaledMinutes(int value) {
	startMillis = millis();
	scaledMinutesOffset = value % 1440;
}

int getScaledMinutes(int scale) {
	unsigned long elapsedMillis_ = millis() - startMillis;
	unsigned long scaledMinutes_ = (elapsedMillis_ / (60000 / scale));
	return (scaledMinutesOffset + scaledMinutes_) % 1440;
}

int getMinutes() {
	time_t now_ = time(nullptr);
	struct tm* timeinfo_ = localtime(&now_);
	int nowMinutes_ = timeinfo_->tm_hour * 60 + timeinfo_->tm_min;
	return nowMinutes_;
}

void updateSceneTimer() {
	unsigned long interval = 60000 / speedFactor;
	sceneTimer.start(interval);
	Serial.println("Scene timer updated to interval: " + String(interval) + " ms");
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
		int minutes_ = getMinutes();
		if (speedFactor > 1) {
			minutes_ = getScaledMinutes(speedFactor);
			char time_[6];
			snprintf(time_, sizeof(time_), "%02d:%02d", minutes_ / 60, minutes_ % 60);
			SERIAL_WEB_SERIALLN("Actual minutes: " + String(minutes_) + " (" + String(time_) + ")");
		}

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
		SERIAL_WEB_SERIALLN("Rebooting...");

		delay(1000);
		ESP.restart();
	}


}

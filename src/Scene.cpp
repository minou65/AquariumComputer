// Scene.cpp
// Implementation of the Scene class

#include "Scene.h"
#include <ESP32PWM.h>

// Pin definitions for the MOSFETs
const int MOSFET1_PIN = 18;
const int MOSFET2_PIN = 19;
const int MOSFET3_PIN = 21;

// Pin definition for the relay
const int RELAY_PIN = 22;

// PWM configuration
const int PWM_FREQ = 5000; // 5 kHz
const int PWM_RESOLUTION = 12; // 12 bit (0-4095)

ESP32PWM Channels[3];

OutputStatus outputStatus[OUTPUT_COUNT]{
	OUTPUT_AUTO,
	OUTPUT_AUTO,
	OUTPUT_AUTO,
	OUTPUT_AUTO
}; // 3 channels + relay

bool updateOutputs = true;

void setupHardware() {
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);

	Channels[0].attachPin(MOSFET1_PIN, PWM_FREQ, PWM_RESOLUTION);
	Channels[1].attachPin(MOSFET2_PIN, PWM_FREQ, PWM_RESOLUTION);
	Channels[2].attachPin(MOSFET3_PIN, PWM_FREQ, PWM_RESOLUTION);

	Channels[0].write(0);
	Channels[1].write(0);
	Channels[2].write(0);

	pinMode(RELAY_PIN, OUTPUT);
	digitalWrite(RELAY_PIN, LOW);
}

uint16_t getChannelValue(uint8_t channel) {
	if (channel >= 1 && channel <= 3) {
		return Channels[channel - 1].getDutyScaled() * 100;
	}
	return 0;
}

bool isRelayActive() {
	return digitalRead(RELAY_PIN) == HIGH;
}

// --- Scene ---
Scene::Scene(const char* id)
	: _id(id),
	ChainedParameterGroup(id, id)
{
	snprintf(_TimeValue, STRING_LEN, "00:00");
	snprintf(_CH1Value, NUMBER_LEN, "0");
	snprintf(_CH2Value, NUMBER_LEN, "0");
	snprintf(_CH3Value, NUMBER_LEN, "0");
	snprintf(_RelayValue, STRING_LEN, "0");
	snprintf(_JumpValue, STRING_LEN, "0");

	snprintf(_TimeId, STRING_LEN, "%s-time", _id.c_str());
	snprintf(_CH1Id, STRING_LEN, "%s-ch1", _id.c_str());
	snprintf(_CH2Id, STRING_LEN, "%s-ch2", _id.c_str());
	snprintf(_CH3Id, STRING_LEN, "%s-ch3", _id.c_str());
	snprintf(_RelayId, STRING_LEN, "%s-relay", _id.c_str());
	snprintf(_JumpId, STRING_LEN, "%s-jump", _id.c_str());

	this->addItem(&this->_TimeParam);
	this->addItem(&this->_CH1Param);
	this->addItem(&this->_CH2Param);
	this->addItem(&this->_CH3Param);
	this->addItem(&this->_RelayParam);
	this->addItem(&this->_JumpParam);
}

void Scene::applyDefaultValue() {
	_TimeParam.applyDefaultValue();
	_CH1Param.applyDefaultValue();
	_CH2Param.applyDefaultValue();
	_CH3Param.applyDefaultValue();
	_JumpParam.applyDefaultValue();
	_RelayParam.applyDefaultValue();
	this->setActive(false);
}

uint16_t Scene::getChannelValue(uint8_t channel) {
	switch (channel) {
	case 1:
		return atoi(_CH1Value);
	case 2:
		return atoi(_CH2Value);
	case 3:
		return atoi(_CH3Value);
	default:
		return 0;
	}
}

const char* Scene::getTimeString() const {
	return _TimeValue;
}

int Scene::getTimeMinutes() const {
	int h = 0, m = 0;
	sscanf(_TimeValue, "%2d:%2d", &h, &m);
	return h * 60 + m;
}

bool Scene::isJumpEnabled() const {
	return strcmp(_JumpValue, "1") == 0 || strcmp(_JumpValue, "true") == 0;
}

bool Scene::isRelayEnabled() const {
	return strcmp(_RelayValue, "1") == 0
		|| strcmp(_RelayValue, "true") == 0
		|| strcmp(_RelayValue, "selected") == 0;
}

void Scene::setCurrentScene(int minutes) {
	int duration, elapsed;
	getSceneTiming(minutes, duration, elapsed);

	if (elapsed >= 0 && elapsed < duration) {
		for (uint8_t channel_ = 1; channel_ <= 3; ++channel_) {

			if (outputStatus[channel_ - 1] == OUTPUT_OFF) {
				Channels[channel_ - 1].writeScaled(0.0f);
				continue;
			} else if (outputStatus[channel_ - 1] == OUTPUT_ON) {
				Channels[channel_ - 1].writeScaled(1.0f);
				continue;
			}

			float value_ = getInterpolatedChannelValue(channel_, minutes);
			Channels[channel_ - 1].writeScaled(value_);
		}

		if (outputStatus[3] == OUTPUT_OFF) {
			digitalWrite(RELAY_PIN, LOW);
		} else if (outputStatus[3] == OUTPUT_ON) {
			digitalWrite(RELAY_PIN, HIGH);
		}
		else {
			digitalWrite(RELAY_PIN, isRelayEnabled() ? HIGH : LOW);
		}
		
	}
}

float Scene::getInterpolatedChannelValue(uint8_t channel, int minutes) {
	int duration, elapsed;
	getSceneTiming(minutes, duration, elapsed);

	Scene* next_ = findNextActive();
	if (next_ != nullptr && next_->isJumpEnabled()) {
		return getChannelValue(channel) / 100.0f;
	}

	float startPoint_ = getChannelValue(channel) / 100.0f;
	float endPoint_ = next_ ? next_->getChannelValue(channel) / 100.0f : startPoint_;

	float value_ = startPoint_;
	if (duration > 0) {
		value_ = startPoint_ + (endPoint_ - startPoint_) * (elapsed / (float)duration);
	}
	return value_;
}

Scene* Scene::findNextActive() {
	// Search for the next active item after the current one
	Scene* next = static_cast<Scene*>(getNext());
	while (next != nullptr) {
		if (next->isActive()) {
			return next;
		}
		next = static_cast<Scene*>(next->getNext());
	}
	// If none found, search from the start of the list up to (but not including) this
	Scene* iter = &scenes[0];
	while (iter != this) {
		if (iter->isActive()) {
			return iter;
		}
		iter = static_cast<Scene*>(iter->getNext());
		if (iter == nullptr) break; // Safety check
	}
	// No active item found
	return nullptr;
}

// Returns duration and elapsed (by reference) for a given minute value, handling day wrap-around
void Scene::getSceneTiming(int minutes, int& duration, int& elapsed) const {
	int currentSceneMinutes_ = getTimeMinutes();
	int nextSceneMinutes_ = 24 * 60; // Default: 24:00
	Scene* next_ = const_cast<Scene*>(this)->findNextActive();
	if (next_ != nullptr) {
		nextSceneMinutes_ = next_->getTimeMinutes();
	}
	duration = (nextSceneMinutes_ - currentSceneMinutes_ + 1440) % 1440;
	elapsed = (minutes - currentSceneMinutes_ + 1440) % 1440;
}

Scene scenes[MAX_SCENES] = {
	Scene("Scene1"),
	Scene("Scene2"),
	Scene("Scene3"),
	Scene("Scene4"),
	Scene("Scene5"),
	Scene("Scene6"),
	Scene("Scene7"),
	Scene("Scene8"),
	Scene("Scene9"),
	Scene("Scene10"),
	Scene("Scene11"),
	Scene("Scene12"),
	Scene("Scene13"),
	Scene("Scene14"),
	Scene("Scene15")
};

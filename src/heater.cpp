// 
// 
// 

#include "heater.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#include <EventLogger.h>

// Pin definition for the temperature sensor
const int ONE_WIRE_BUS = 32;

// Pin definition for the relay
const int RELAY_PIN = 25;

// Setup a OneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass OneWire reference to Dallas Temperature
DallasTemperature sensors(&oneWire);

TaskHandle_t taskHandle;

HeaterStatus heaterStatus = HEATER_AUTO;

void getTemperature(void* parameter) {
    while (true) {
        sensors.requestTemperatures();
        heater.setTemperature(sensors.getTempCByIndex(0));
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 Sekunde warten
    }
}

void setupHeaterHardware() {
    // Start up the library
    sensors.begin();
    sensors.setResolution(12);

    // Set relay pin as output
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Turn off heater initially

    eventLogger.log("Heater hardware setup complete", EventLogger::Status::OK);
	eventLogger.log("    Used pin for relay: " + String(RELAY_PIN), EventLogger::Status::DEBUG);

    xTaskCreatePinnedToCore(
        getTemperature, /* Function to implement the task */
        "TaskHandle", /* Name of the task */
        10000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        0,  /* Priority of the task */
        &taskHandle,  /* Task handle. */
        0 /* Core where the task should run */
    );
}

void loopHeater(){
	heater.process();
}

Heater::Heater()
    : iotwebconf::ParameterGroup("HeaterGroup", "Heater") {
    snprintf(_maxTemperatureValue, sizeof(_maxTemperatureValue), "%s", "25.0");
    snprintf(_hysteresisValue, sizeof(_hysteresisValue), "%s", "0.5");

    addItem(&_maxTemperatureParam);
    addItem(&_hysteresisParam);
}

void Heater::applyDefaultValue() {
    _maxTemperatureParam.applyDefaultValue();
    _hysteresisParam.applyDefaultValue();
}

float Heater::maxTemperature() const {
    return String(_maxTemperatureValue).toFloat();
}
float Heater::hysteresis() const {
    return String(_hysteresisValue).toFloat();
}

void Heater::setTemperature(float temperature) {
    _currentTemperature = temperature;
}

float Heater::getCurrentTemperature() const {
    return _currentTemperature;
}

bool Heater::isHeating() const {
    return digitalRead(RELAY_PIN) == HIGH;
}

void Heater::process() {
    static int previousRelayState_ = -1;
    static bool previousSensorError_ = false;
    static HeaterStatus previousHeaterStatus_ = static_cast<HeaterStatus>(255);

    if (previousRelayState_ == -1) {
        previousRelayState_ = digitalRead(RELAY_PIN);
    }

    // Moduswechsel loggen
    if (heaterStatus != previousHeaterStatus_) {
        if (heaterStatus == HEATER_AUTO) {
            eventLogger.log("Heater set to AUTO by user", EventLogger::Status::INFO);
        } else if (heaterStatus == HEATER_ON) {
            eventLogger.log("Heater forced ON by user", EventLogger::Status::INFO);
        } else if (heaterStatus == HEATER_OFF) {
            eventLogger.log("Heater forced OFF by user", EventLogger::Status::INFO);
        }
        previousHeaterStatus_ = heaterStatus;
    }

    // Manuelle Modi zuerst behandeln (kein Sensor-Error-Spam in OFF/ON)
    if (heaterStatus == HEATER_OFF) {
        if (previousRelayState_ != LOW) {
            digitalWrite(RELAY_PIN, LOW);
            previousRelayState_ = LOW;
        }
        previousSensorError_ = false;
        return;
    }

    if (heaterStatus == HEATER_ON) {
        if (previousRelayState_ != HIGH) {
            digitalWrite(RELAY_PIN, HIGH);
            previousRelayState_ = HIGH;
        }
        previousSensorError_ = false;
        return;
    }

    // AUTO: Sensordaten validieren
    bool disconnected_ = (_currentTemperature == DEVICE_DISCONNECTED_C);
    bool invalid85_ = (_currentTemperature > 84.9f && _currentTemperature < 85.1f); // DS18B20 default/invalid
    bool sensorError_ = disconnected_ || invalid85_;

    if (sensorError_) {
        if (previousRelayState_ != LOW) {
            digitalWrite(RELAY_PIN, LOW);
            previousRelayState_ = LOW;
        }
        if (!previousSensorError_) {
            eventLogger.log(
                disconnected_
                    ? "Heater sensor error (disconnected), turning off heater"
                    : "Heater sensor invalid reading (85.0C), turning off heater",
                EventLogger::Status::ERROR);
        }
        previousSensorError_ = true;
        return;
    }

    previousSensorError_ = false;

    float lowerThreshold_ = maxTemperature() - (hysteresis() / 2.0f);
    float upperThreshold_ = maxTemperature() + (hysteresis() / 2.0f);

    if (_currentTemperature < lowerThreshold_) {
        if (previousRelayState_ != HIGH) {
            digitalWrite(RELAY_PIN, HIGH);
            previousRelayState_ = HIGH;
            eventLogger.logf(
                EventLogger::Status::INFO,
                "Heater turned ON. Current temperature: %.1f \xC2\xB0" "C",
                _currentTemperature);
        }
    }
    else if (_currentTemperature > upperThreshold_) {
        if (previousRelayState_ != LOW) {
            digitalWrite(RELAY_PIN, LOW);
            previousRelayState_ = LOW;
            eventLogger.logf(
                EventLogger::Status::INFO,
                "Heater turned OFF. Current temperature: %.1f \xC2\xB0" "C",
                _currentTemperature);
        }
    }
}


Heater heater;
// 
// 
// 

#include "heater.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin definition for the temperature sensor
const int ONE_WIRE_BUS = 23;

// Pin definition for the relay
const int RELAY_PIN = 24;

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
    if (_currentTemperature == DEVICE_DISCONNECTED_C) {
        // Sensor error, turn off heater
        digitalWrite(RELAY_PIN, LOW);
        return;
	}

    if(HeaterStatus::HEATER_OFF == heaterStatus) {
        digitalWrite(RELAY_PIN, LOW); // Turn off heater
        return;
	}

    if (HeaterStatus::HEATER_ON == heaterStatus) {
        heaterStatus = HeaterStatus::HEATER_AUTO;
        return;
    }

    if (_currentTemperature < (maxTemperature() - hysteresis()/2)) {
        digitalWrite(RELAY_PIN, HIGH); // Turn on heater
    } else if (_currentTemperature > (maxTemperature() + hysteresis()/2)) {
        digitalWrite(RELAY_PIN, LOW); // Turn off heater
	}
}


Heater heater;
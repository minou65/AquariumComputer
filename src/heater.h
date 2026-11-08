// heater.h

#ifndef _HEATER_h
#define _HEATER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <IotWebConfParameter.h>
#include "neotimer.h"

#define HEATER_NUMBER_LEN 6
#define HEATER_STRING_LEN 20

enum HeaterStatus {
	HEATER_AUTO = 0,
    HEATER_ON,
    HEATER_OFF

};
extern HeaterStatus heaterStatus;

// Schaltet die periodische Sensor-Meldung (alle 30s) im Web-Konsolen-Stream ein/aus
extern volatile bool temperatureVerboseLogging;

// Erlaubt im manuellen Modus (HEATER_ON) trotz defektem/fehlendem Temperatursensor
// weiterzuheizen. Standardmäßig deaktiviert (sicherer Default), da ohne gültige
// Temperaturmessung keine Überhitzungserkennung möglich ist.
extern volatile bool heaterIgnoreSensorErrorWhenForcedOn;


class Heater : public iotwebconf::ParameterGroup {
public:
    Heater();

    void applyDefaultValue();
    float maxTemperature() const;
    float hysteresis() const;

    void setTemperature(float temperature);
    float getCurrentTemperature() const;
    bool isHeating() const;
    unsigned long lastRelaySwitchMillis() const;

    void process();

private:
    char _maxTemperatureValue[HEATER_NUMBER_LEN] = { 0 };
    char _hysteresisValue[HEATER_NUMBER_LEN] = { 0 };
    Neotimer _relayRefreshTimer = Neotimer(60000);
    Neotimer _sensorCheckTimer = Neotimer(10000);
    volatile float _currentTemperature = 0.0;
    unsigned long _lastRelaySwitchMillis = 0;

    iotwebconf::NumberParameter _maxTemperatureParam = iotwebconf::NumberParameter(
        "Max Temperatur (°C)", "MaxTemperatureParam", _maxTemperatureValue, HEATER_NUMBER_LEN, "25.0", "17.0..30.0", "min='17.0' max='30.0' step='0.1'");
    iotwebconf::NumberParameter _hysteresisParam = iotwebconf::NumberParameter(
        "Hysterese (°C)", "HysteresisParam", _hysteresisValue, HEATER_NUMBER_LEN, "2.0", "0..5", "min='0.0' max='5.0' step='0.1'");

    // Setzt das Relais auf den gewünschten Zustand, aber nur wenn sich der
    // Zustand tatsächlich ändert. Aktualisiert dabei den internen Zustand
    // sowie den Zeitstempel des letzten Schaltvorgangs.
    void setRelay(int* previousRelayState, bool high);

    // Prüft, ob der aktuelle Temperaturwert einen Sensorfehler darstellt
    // (Sensor getrennt oder DS18B20-Standard-/Fehlerwert 85.0 °C).
    static bool isSensorErrorReading(float temperature, bool* disconnected);
};

extern Heater heater;

#endif

void setupHeaterHardware();
void loopHeater();

// heater.h

#ifndef _HEATER_h
#define _HEATER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <IotWebConfParameter.h>

#define HEATER_NUMBER_LEN 6
#define HEATER_STRING_LEN 20

enum HeaterStatus {
	HEATER_AUTO = 0,
    HEATER_ON,
    HEATER_OFF

};
extern HeaterStatus heaterStatus;

class Heater : public iotwebconf::ParameterGroup {
public:
    Heater();

    void applyDefaultValue();
    float maxTemperature() const;
    float hysteresis() const;

    void setTemperature(float temperature);
    float getCurrentTemperature() const;
    bool isHeating() const;

    void process();

private:
    char _maxTemperatureValue[HEATER_NUMBER_LEN] = { 0 };
    char _hysteresisValue[HEATER_NUMBER_LEN] = { 0 };

	float _currentTemperature = 0.0;

    iotwebconf::NumberParameter _maxTemperatureParam = iotwebconf::NumberParameter(
        "Max Temperatur (°C)", "MaxTemperatureParam", _maxTemperatureValue, HEATER_NUMBER_LEN, "25.0", "17.0..30.0", "min='17.0' max='30.0' step='0.1'");
    iotwebconf::NumberParameter _hysteresisParam = iotwebconf::NumberParameter(
        "Hysterese (°C)", "HysteresisParam", _hysteresisValue, HEATER_NUMBER_LEN, "2.0", "0..5", "min='0.0' max='5.0' step='0.1'");
};

extern Heater heater;

#endif

void setupHeaterHardware();
void loopHeater();

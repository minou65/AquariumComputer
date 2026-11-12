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
volatile bool heaterIgnoreSensorErrorWhenForcedOn = false;
volatile bool temperatureVerboseLogging = false;
DeviceAddress sensorAddress;
bool sensorAddressKnown = false;

#define SENSOR_ERROR_THRESHOLD 5 // Anzahl aufeinanderfolgender Sensorfehler, bevor der Sensor als "verloren" betrachtet wird
#define SENSOR_ERROR_INTERVAL 2000 // Intervall in Millisekunden zwischen den Sensorabfragen
#define SENSOR_REPORT_INTERVAL 15 * 1000 // Intervall in Millisekunden für die Ausgabe der Sensorwerte im Web-Stream
#define SENSOR_RESOLUTION 12 // Auflösung des DS18B20-Sensors (9-12 Bit)

void getTemperature(void* parameter) {
    const int kConsecutiveFailuresBeforeReinit_ = 5; // ~10 Sekunden bei 2s Intervall
    int consecutiveFailures_ = 0;
    unsigned long lastReportMillis_ = 0;
    const unsigned long kReportIntervalMs_ = SENSOR_REPORT_INTERVAL; // alle 15 Sekunden Sensorwert ausgeben (sofern aktiviert)

    while (true) {
        bool requestOk_ = sensors.requestTemperatures();
        float temperature_ = sensors.getTempCByIndex(0);
        bool disconnected_ = (temperature_ == DEVICE_DISCONNECTED_C);

        if (disconnected_) {
            consecutiveFailures_++;
            if (consecutiveFailures_ >= kConsecutiveFailuresBeforeReinit_) {
                eventLogger.log("Temperature sensor lost, re-initializing OneWire bus", EventLogger::Status::ERROR);

                // Bus zurücksetzen und Geräte neu suchen,
                // falls der Sensor durch eine Störung "verschwunden" ist
                oneWire.reset();
                sensors.begin();
                sensors.setResolution(SENSOR_RESOLUTION);
                sensorAddressKnown = sensors.getAddress(sensorAddress, 0);

                consecutiveFailures_ = 0;
            }
        }
        else {
            consecutiveFailures_ = 0;
        }

        heater.setTemperature(temperature_);

        if (temperatureVerboseLogging) {
            unsigned long now_ = millis();
            if (now_ - lastReportMillis_ >= kReportIntervalMs_) {
                lastReportMillis_ = now_;
                if (disconnected_) {
                    eventLogger.webPrintf(
                        EventLogger::Status::DEBUG,
                        "Temperature sensor: disconnected (ms since last relay switch: %lu)",
                        millis() - heater.lastRelaySwitchMillis());
                }
                else {
                    eventLogger.webPrintf(
                        EventLogger::Status::DEBUG,
                        "Temperature sensor: %.4f \xC2\xB0" "C (requestOk=%d)",
                        temperature_, requestOk_ ? 1 : 0);
                }
            }
        }

        vTaskDelay(SENSOR_ERROR_INTERVAL / portTICK_PERIOD_MS); // 2 Sekunden warten
    }
}

void setupHeaterHardware() {
    eventLogger.log("Setting up heater hardware...", EventLogger::Status::INFO);
    // Start up the library
    sensors.begin();
    sensors.setResolution(SENSOR_RESOLUTION);
    sensorAddressKnown = sensors.getAddress(sensorAddress, 0);

    // Set relay pin as output
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Turn off heater initially
        
    eventLogger.log("Used pin for relay: " + String(RELAY_PIN), EventLogger::Status::DEBUG, 1);

    uint8_t deviceCount_ = sensors.getDeviceCount();
    bool parasitePower_ = sensors.isParasitePowerMode();
    eventLogger.logf(
		EventLogger::Status::DEBUG, 1,
        "OneWire devices found: %d, address known: %s, parasite power: %s, resolution: %d bit",
        deviceCount_, sensorAddressKnown ? "yes" : "no", parasitePower_ ? "yes" : "no", sensors.getResolution());


    xTaskCreatePinnedToCore(
        getTemperature, /* Function to implement the task */
        "TaskHandle", /* Name of the task */
        10000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        0,  /* Priority of the task */
        &taskHandle,  /* Task handle. */
        0 /* Core where the task should run (away from loop()/WebServer on Core 1) */
    );
	eventLogger.webPrintf(EventLogger::Status::INFO, 1, "Temperature sensor task created on core %d", xPortGetCoreID());

    eventLogger.log("Heater hardware setup complete", EventLogger::Status::OK);
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
	_relayRefreshTimer.start();
	_sensorCheckTimer.start();
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

void Heater::setRelay(int* previousRelayState, bool high) {
    int newState_ = high ? HIGH : LOW;
    if (*previousRelayState != newState_) {
        digitalWrite(RELAY_PIN, newState_);
        *previousRelayState = newState_;
        _lastRelaySwitchMillis = millis();
    }
}

bool Heater::isSensorErrorReading(float temperature, bool* disconnected) {
    bool disconnected_ = (temperature == DEVICE_DISCONNECTED_C);
    bool invalid85_ = (temperature > 84.9f && temperature < 85.1f); // DS18B20 default/invalid
    if (disconnected != nullptr) {
        *disconnected = disconnected_;
    }
    return disconnected_ || invalid85_;
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

unsigned long Heater::lastRelaySwitchMillis() const {
    return _lastRelaySwitchMillis;
}

void Heater::process() {
    static int previousRelayState_ = -1;
    static bool previousSensorError_ = false;
    static HeaterStatus previousHeaterStatus_ = static_cast<HeaterStatus>(255);
    static int consecutiveSensorErrors_ = 0;
    const int kSensorErrorThreshold_ = SENSOR_ERROR_THRESHOLD;

    if (previousRelayState_ == -1) {
        previousRelayState_ = digitalRead(RELAY_PIN);
    }

    // Sicherstellen, dass der Pin regelmässig neu geschrieben wird, auch wenn
    // sich der Soll-Zustand nicht geändert hat. Schützt gegen Glitches, bei
    // denen der physische Pegel oder das Relais vom zuletzt gesetzten
    // Software-Zustand abweicht, ohne dass ein Zustandswechsel erkannt würde.
    if (_relayRefreshTimer.repeat()) {
        digitalWrite(RELAY_PIN, previousRelayState_);
    }

    // Moduswechsel loggen
    if (heaterStatus != previousHeaterStatus_) {
        if (heaterStatus == HEATER_AUTO) {
            eventLogger.log("Heater set to AUTO by user", EventLogger::Status::INFO);
        }
        else if (heaterStatus == HEATER_ON) {
            eventLogger.log("Heater forced ON by user", EventLogger::Status::INFO);
        }
        else if (heaterStatus == HEATER_OFF) {
            eventLogger.log("Heater forced OFF by user", EventLogger::Status::INFO);
        }
        previousHeaterStatus_ = heaterStatus;
    }

    // Manuelle Modi werden sofort abgearbeitet (keine Verzögerung durch den Sensor-Timer)
    if (heaterStatus == HEATER_OFF) {
        setRelay(&previousRelayState_, false);
        previousSensorError_ = false;
        consecutiveSensorErrors_ = 0;
        return;
    }

    if (heaterStatus == HEATER_ON) {
        bool disconnected_ = false;
        bool sensorErrorReading_ = isSensorErrorReading(_currentTemperature, &disconnected_);

        if (sensorErrorReading_ && !heaterIgnoreSensorErrorWhenForcedOn) {
            // Sicherheitsabschaltung: ohne gültige Temperaturmessung kein manuelles Heizen,
            // es sei denn, der Nutzer hat dies ausdrücklich per Befehl erlaubt.
            setRelay(&previousRelayState_, false);
            if (!previousSensorError_) {
                eventLogger.log(
                    disconnected_
                    ? "Heater sensor error (disconnected) while forced ON, turning off heater for safety"
                    : "Heater sensor invalid reading (85.0C) while forced ON, turning off heater for safety",
                    EventLogger::Status::ERROR);
            }
            previousSensorError_ = true;
            return;
        }

        if (sensorErrorReading_ && !previousSensorError_) {
            eventLogger.log(
                "Heater forced ON despite sensor error (override active by user command)",
                EventLogger::Status::WARN);
        }

        setRelay(&previousRelayState_, true);
        previousSensorError_ = sensorErrorReading_;
        consecutiveSensorErrors_ = 0;
        return;
    }

    // AUTO: Temperatur wird nur alle 10 Sekunden geprüft, um auf kurze
    // PWM-bedingte Störungen nicht überempfindlich zu reagieren.
    if (!_sensorCheckTimer.repeat()) {
        return;
    }

    const float currentTemperature_ = _currentTemperature;
    bool disconnected_ = false;
    bool sensorErrorReading_ = isSensorErrorReading(currentTemperature_, &disconnected_);

    if (sensorErrorReading_) {
        consecutiveSensorErrors_++;

        if (consecutiveSensorErrors_ >= kSensorErrorThreshold_) {
            setRelay(&previousRelayState_, false);
            if (!previousSensorError_) {
                eventLogger.log(
                    disconnected_
                    ? "Heater sensor error (disconnected), turning off heater"
                    : "Heater sensor invalid reading (85.0C), turning off heater",
                    EventLogger::Status::ERROR);
            }
            previousSensorError_ = true;
        }
        // Sonst: noch innerhalb der Entprellungs-Toleranz, weder loggen noch Relais ändern
        return;
    }

    // Guter Messwert: Fehlerzähler zurücksetzen
    consecutiveSensorErrors_ = 0;
    previousSensorError_ = false;

    float lowerThreshold_ = maxTemperature() - (hysteresis() / 2.0f);
    float upperThreshold_ = maxTemperature() + (hysteresis() / 2.0f);

    if (currentTemperature_ < lowerThreshold_) {
        if (previousRelayState_ != HIGH) {
            setRelay(&previousRelayState_, true);
            eventLogger.logf(
                EventLogger::Status::INFO,
                "Heater turned ON. Current temperature: %.1f \xC2\xB0" "C",
                currentTemperature_);
        }
    }
    else if (currentTemperature_ > upperThreshold_) {
        if (previousRelayState_ != LOW) {
            setRelay(&previousRelayState_, false);
            eventLogger.logf(
                EventLogger::Status::INFO,
                "Heater turned OFF. Current temperature: %.1f \xC2\xB0" "C",
                currentTemperature_);
        }
    }
}

Heater heater;
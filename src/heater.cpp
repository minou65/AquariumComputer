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
    const unsigned long kReportIntervalMs_ = SENSOR_REPORT_INTERVAL; // alle 30 Sekunden Sensorwert ausgeben (sofern aktiviert)

    while (true) {
        bool requestOk_ = sensors.requestTemperatures();
        float temperature_ = sensors.getTempCByIndex(0);

        if (temperature_ == DEVICE_DISCONNECTED_C) {
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
                if (temperature_ == DEVICE_DISCONNECTED_C) {
                    eventLogger.webPrintln("Temperature sensor: disconnected");
                }
                else {
                    String rawInfo_ = "n/a";
                    if (sensorAddressKnown) {
                        uint8_t scratchPad_[9];
                        bool crcOk_ = sensors.readScratchPad(sensorAddress, scratchPad_);
                        char hexBuf_[28];
                        snprintf(hexBuf_, sizeof(hexBuf_), "%02X%02X crc=%s",
                            scratchPad_[0], scratchPad_[1], crcOk_ ? "OK" : "FAIL");
                        rawInfo_ = String(hexBuf_);
                    }
                    eventLogger.webPrintln(
                        "Temperature sensor: " + String(temperature_, 4) + " \xC2\xB0" "C"
                        + " (requestOk=" + String(requestOk_ ? "1" : "0")
                        + ", scratchpad=" + rawInfo_ + ")");
                }
            }
        }

        vTaskDelay(SENSOR_ERROR_INTERVAL / portTICK_PERIOD_MS); // 2 Sekunden warten
    }
}

void setupHeaterHardware() {
    // Start up the library
    sensors.begin();
    sensors.setResolution(SENSOR_RESOLUTION);
    sensorAddressKnown = sensors.getAddress(sensorAddress, 0);

    // Set relay pin as output
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Turn off heater initially

    eventLogger.log("Heater hardware setup complete", EventLogger::Status::OK);
    eventLogger.log("    Used pin for relay: " + String(RELAY_PIN), EventLogger::Status::DEBUG);

    uint8_t deviceCount_ = sensors.getDeviceCount();
    bool parasitePower_ = sensors.isParasitePowerMode();
    eventLogger.logf(
        EventLogger::Status::DEBUG,
        "    OneWire devices found: %d, address known: %s, parasite power: %s, resolution: %d bit",
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
    static int consecutiveSensorErrors_ = 0;
    const int kSensorErrorThreshold_ = SENSOR_ERROR_THRESHOLD; // ~10 Sekunden bei 2 Sekunden Messintervall, bevor reagiert wird

    if (previousRelayState_ == -1) {
        previousRelayState_ = digitalRead(RELAY_PIN);
    }

    // Sicherstellen, dass der Pin regelmässig neu geschrieben wird, auch wenn
    // sich der Soll-Zustand nicht geändert hat. Schützt gegen Glitches, bei
    // denen der physische Pegel oder das Relais vom zuletzt gesetzten
    // Software-Zustand abweicht, ohne dass ein Zustandswechsel erkannt würde.
    bool forceRelayRefresh_ = _relayRefreshTimer.repeat();
    if (forceRelayRefresh_) {
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

    // Manuelle Modi zuerst behandeln (kein Sensor-Error-Spam in OFF/ON)
    if (heaterStatus == HEATER_OFF) {
        if (previousRelayState_ != LOW) {
            digitalWrite(RELAY_PIN, LOW);
            previousRelayState_ = LOW;
        }
        previousSensorError_ = false;
        consecutiveSensorErrors_ = 0;
        return;
    }

    if (heaterStatus == HEATER_ON) {
        if (previousRelayState_ != HIGH) {
            digitalWrite(RELAY_PIN, HIGH);
            previousRelayState_ = HIGH;
        }
        previousSensorError_ = false;
        consecutiveSensorErrors_ = 0;
        return;
    }

    // AUTO: Sensordaten validieren
    bool disconnected_ = (_currentTemperature == DEVICE_DISCONNECTED_C);
    bool invalid85_ = (_currentTemperature > 84.9f && _currentTemperature < 85.1f); // DS18B20 default/invalid
    bool sensorErrorReading_ = disconnected_ || invalid85_;

    if (sensorErrorReading_) {
        consecutiveSensorErrors_++;
    }
    else {
        // Einzelner guter Wert setzt den Fehlerzähler sofort zurück
        consecutiveSensorErrors_ = 0;
        previousSensorError_ = false;
    }

    if (consecutiveSensorErrors_ >= kSensorErrorThreshold_) {
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

    if (sensorErrorReading_) {
        // Noch innerhalb der Entprellungs-Toleranz: weder loggen noch Relais ändern
        return;
    }

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
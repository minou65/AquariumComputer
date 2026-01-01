// 
// 
// 
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#include "handlingWeb.h"
#include <DNSServer.h>

#include <IotWebConfAsyncUpdateServer.h>
#include <IotWebConfAsync.h>
#include <IotWebRoot.h>
#include <Preferences.h>

#include "html.h"
#include "favicon.h"

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "123456789";

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "001"
extern char Version[];

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN  -1

// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN GPIO_NUM_23
#define ON_LEVEL HIGH

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "AquariumController";


DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebServerWrapper asyncWebServerWrapper(&server);
AsyncUpdateServer AsyncUpdater;

NtpConfig ntpConfig = NtpConfig();

AsyncIotWebConf iotWebConf(thingName, &dnsServer, &asyncWebServerWrapper, wifiInitialApPassword, CONFIG_VERSION);

bool ShouldReboot = false;
bool ConfigChanged = false;

extern uint8_t speedFactor;

// -- Method declarations.
void handleRoot(AsyncWebServerRequest* request);
void handleDateTime(AsyncWebServerRequest* request);
void handleData(AsyncWebServerRequest* request);
void handlePost(AsyncWebServerRequest* request);
void handleAPPasswordMissingPage(iotwebconf::WebRequestWrapper* webRequestWrapper);
void handleSSIDNotConfiguredPage(iotwebconf::WebRequestWrapper* webRequestWrapper);
void handleConfigSaved();
void handleConfigSavedPage(iotwebconf::WebRequestWrapper* webRequestWrapper);
void handleConnectWifi(const char* ssid, const char* password);
iotwebconf::WifiAuthInfo* handleConnectWifiFailure();
void handleWificonnected();
void handleWebSerialCommand(const String& command);
void handleOnProgress(size_t prg, size_t sz);

extern void updateSceneTimer();
extern void setScaledMinutes(int value);

class CustomHtmlFormatProvider : public iotwebconf::OptionalGroupHtmlFormatProvider {
protected:
    String getFormEnd() override {
        return OptionalGroupHtmlFormatProvider::getFormEnd() + String(FPSTR(html_form_end));
    }
    String getEnd() override {
        return String(FPSTR(html_end_template)) + HtmlFormatProvider::getEnd();

    }
};
CustomHtmlFormatProvider customHtmlFormatProvider;

class MyHtmlRootFormatProvider : public HtmlRootFormatProvider {
public:
    String getHtmlTableRowClass(String name, String htmlclass, String id) {
        String s_ = F("<tr><td align=\"left\">{n}</td><td align=\"left\"><span id=\"{id}\" class=\"{c}\"></span></td></tr>\n");
        s_.replace("{n}", name);
        s_.replace("{c}", htmlclass);
        s_.replace("{id}", id);
        return s_;
    }

protected:
    String getStyleInner() override {
        String s_ = HtmlRootFormatProvider::getStyleInner();
        s_ += String(FPSTR(html_output_styles));
        return s_;
    }

    String getScriptInner() override {
        String s_ = HtmlRootFormatProvider::getScriptInner();

        s_.replace("{millisecond}", "5000");
        s_ += String(FPSTR(html_script));
        return s_;
    }
};

const char PREF_NAMESPACE[] = "OutputStatus";

void saveOutputStatus() {
    Preferences prefs_;
    prefs_.begin(PREF_NAMESPACE, false);
    for (int i_ = 0; i_ < OUTPUT_COUNT; ++i_) {
        String key_ = "out" + String(i_);
        prefs_.putUChar(key_.c_str(), static_cast<uint8_t>(outputStatus[i_]));
    }
    for (int i_ = 0; i_ < 3; ++i_) {
        String key_ = "ch" + String(i_);
        prefs_.putFloat(key_.c_str(), ChannelsBrigthness[i_]);
    }
	prefs_.putUChar("heater", static_cast<uint8_t>(heaterStatus));
    prefs_.end();
}

void loadOutputStatus() {
    Preferences prefs_;
    prefs_.begin(PREF_NAMESPACE, true);
    for (int i_ = 0; i_ < OUTPUT_COUNT; ++i_) {
        String key_ = "out" + String(i_);
        outputStatus[i_] = static_cast<OutputStatus>(prefs_.getUChar(key_.c_str(), OUTPUT_AUTO));
    }
    for (int i_ = 0; i_ < 3; ++i_) {
        String key_ = "ch" + String(i_);
        ChannelsBrigthness[i_] = prefs_.getFloat(key_.c_str(), 1.0f);
	}
	heaterStatus = static_cast<HeaterStatus>(prefs_.getUChar("heater", HEATER_AUTO));
    prefs_.end();
}


void setupWebHandling() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("starting up...");

    for (int i = 0; i < MAX_SCENES - 1; ++i) { 
        scenes[i].setNext(&scenes[i + 1]);
    }


    iotWebConf.setStatusPin(STATUS_PIN, ON_LEVEL);
    iotWebConf.setConfigPin(CONFIG_PIN);

    iotWebConf.setHtmlFormatProvider(&customHtmlFormatProvider);

   iotWebConf.addParameterGroup(&ntpConfig);
   iotWebConf.addParameterGroup(&heater);
   for (int i = 0; i < MAX_SCENES; ++i) {
	   iotWebConf.addParameterGroup(&scenes[i]);
   }

    iotWebConf.getApTimeoutParameter()->visible = true;

    iotWebConf.setConfigSavedCallback(&handleConfigSaved);
    iotWebConf.setConfigSavedPage(&handleConfigSavedPage);

    iotWebConf.setConfigAPPasswordMissingPage(&handleAPPasswordMissingPage);
    iotWebConf.setConfigSSIDNotConfiguredPage(&handleSSIDNotConfiguredPage);
    
    iotWebConf.setWifiConnectionCallback(&handleWificonnected);
    iotWebConf.setWifiConnectionHandler(&handleConnectWifi);
    iotWebConf.setWifiConnectionFailedHandler(&handleConnectWifiFailure);

    iotWebConf.init();


    iotWebConf.setupUpdateServer(
        [](const char* updatePath) { AsyncUpdater.setup(&server, updatePath, handleOnProgress); },
        [](const char* userName, char* password) { AsyncUpdater.updateCredentials(userName, password); });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { 
        handleRoot(request); 
        }
    );
    server.on("/config", HTTP_ANY, [](AsyncWebServerRequest* request) {
        auto* asyncWebRequestWrapper_ = new AsyncWebRequestWrapper(request);
        iotWebConf.handleConfig(asyncWebRequestWrapper_);
        }
    );
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response_ = request->beginResponse(200, "text/html",
            "<html>"
            "<head>"
            "<meta http-equiv=\"refresh\" content=\"15; url=/\">"
            "<title>Rebooting...</title>"
            "</head>"
            "<body>"
            "Please wait while the device is rebooting...<br>"
            "You will be redirected to the homepage shortly."
            "</body>"
            "</html>");
        request->client()->setNoDelay(true); // Disable Nagle's algorithm so the client gets the response immediately
        request->send(response_);
        ShouldReboot = true;
        }
    );

    server.on("/DateTime", HTTP_GET, [](AsyncWebServerRequest* request) { handleDateTime(request); });
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest* request) { handleData(request); });
    server.on("/post", HTTP_ANY, [](AsyncWebServerRequest* request) {
        handlePost(request);
        }
    );
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response_ = request->beginResponse_P(200, "image/x-icon", favicon_ico, sizeof(favicon_ico));
        request->send(response_);
        }
    );
    server.onNotFound([](AsyncWebServerRequest* request) {
        AsyncWebRequestWrapper asyncWebRequestWrapper_(request);
        iotWebConf.handleNotFound(&asyncWebRequestWrapper_);
        }
    );

    WebSerial.begin(&server, "/webserial");
    WebSerial.onMessage([](uint8_t* data, size_t len) {
        String command = "";
        for (size_t i = 0; i < len; i++) command += (char)data[i];
        handleWebSerialCommand(command);
        }
    );

	loadOutputStatus();
}

void loopWebHandling() {
    iotWebConf.doLoop();
    ArduinoOTA.handle();

    if (!ShouldReboot) {
        ShouldReboot = AsyncUpdater.isFinished();
    }
}

void handlePost(AsyncWebServerRequest* request) {

    if (request->hasParam("reset", true)) {
        String value_ = request->getParam("reset", true)->value();

		Scene* scene = &scenes[0];
        while (scene != nullptr) {
            scene->applyDefaultValue();
            scene = (Scene*)scene->getNext();
        }
		ntpConfig.applyDefaultValue();
        heater.applyDefaultValue();
    }

    auto setOutputStatus = [](const String& value) -> OutputStatus {
        if (value == "on") return OUTPUT_ON;
        if (value == "off") return OUTPUT_OFF;
        return OUTPUT_AUTO;
        };

    for (int i = 0; i < 3; ++i) {
        String param = "ch" + String(i + 1);
        if (request->hasParam(param, true)) {
            String value_ = request->getParam(param, true)->value();
            outputStatus[i] = setOutputStatus(value_);
        }
    }
    if (request->hasParam("relay", true)) {
        String value_ = request->getParam("relay", true)->value();
        outputStatus[3] = setOutputStatus(value_);
    }

    if (request->hasParam("heater", true)) {
        String value_ = request->getParam("heater", true)->value();
        if (value_ == "on") {
            heaterStatus = HEATER_ON;
        }
        else if (value_ == "off") {
            heaterStatus = HEATER_OFF;
        }
        else {
            heaterStatus = HEATER_AUTO;
        }
    }

    for (int i_ = 0; i_ < 3; ++i_) {
        String brightParam_ = "brightnessCh" + String(i_ + 1);
        if (request->hasParam(brightParam_, true) && outputStatus[i_] == OUTPUT_ON) {
            String valStr_ = request->getParam(brightParam_, true)->value();
            int val_ = valStr_.toInt();
            if (val_ < 0) val_ = 0;
            if (val_ > 100) val_ = 100;
			setChannelValue(i_ + 1, val_);
        }
    }

	saveOutputStatus();
	updateOutputs = true;

    request->redirect("/");
}

void handleDateTime(AsyncWebServerRequest* request) {
    ESP_LOGD("handleDateTime", "Time requested");

    // Get the current time
    time_t now_ = time(nullptr);
    struct tm* timeinfo_ = localtime(&now_);
    char timeStr_[20];
    char dateStr_[20];
    strftime(timeStr_, sizeof(timeStr_), "%H:%M:%S", timeinfo_);
    strftime(dateStr_, sizeof(dateStr_), "%Y-%m-%d", timeinfo_);

    request->send(200, "text/plain", timeStr_);
}

void handleData(AsyncWebServerRequest* request) {
    String json_ = "{";
    json_ += "\"RSSI\":\"" + String(WiFi.RSSI()) + "\"";

    // Outputs-Subnode
    json_ += ",\"outputs\":{";
    for (uint8_t ch_ = 1; ch_ <= 3; ++ch_) {
        if (ch_ > 1) json_ += ",";
        json_ += "\"ch" + String(ch_) + "\":" + String(getChannelValue(ch_));
    }
    json_ += ",\"relay\":\"";
    json_ += isRelayActive() ? "ON" : "OFF";
    json_ += "\"}";

    json_ += ",\"heater\":{";
    json_ += "\"enabled\":" + String(heater.isHeating() ? "true" : "false");
	json_ += ",\"currentTemperature\":" + String(heater.getCurrentTemperature(), 1);
    json_ += ",\"maxTemperature\":" + String(heater.maxTemperature(), 1);
    json_ += ",\"hysteresis\":" + String(heater.hysteresis(), 1);
    json_ += "}";

    json_ += "}";
    request->send(200, "application/json", json_);
}

void handleAPPasswordMissingPage(iotwebconf::WebRequestWrapper* webRequestWrapper) {
    String content_;
    MyHtmlRootFormatProvider fp_;

    content_ += fp_.getHtmlHead(iotWebConf.getThingName());
    content_ += fp_.getHtmlStyle();
    content_ += fp_.getHtmlHeadEnd();
    content_ += "<body>";
    content_ += "You should change the default AP password to continue.";
    content_ += fp_.addNewLine(2);
    content_ += "Return to <a href='config'>configuration page</a>.";
    content_ += fp_.addNewLine(1);
    content_ += "Return to <a href='/'>home page</a>.";
    content_ += "</body>";
    content_ += fp_.getHtmlEnd();

    webRequestWrapper->sendHeader("Content-Length", String(content_.length()));
    webRequestWrapper->send(200, "text/html", content_);
}

void handleSSIDNotConfiguredPage(iotwebconf::WebRequestWrapper* webRequestWrapper) {
    webRequestWrapper->sendHeader("Location", "/", true);
    webRequestWrapper->send(302, "text/plain", "SSID not configured");
}

void handleConfigSaved() {
    ArduinoOTA.setHostname(iotWebConf.getThingName());
	ConfigChanged = true;
}

void handleConfigSavedPage(iotwebconf::WebRequestWrapper* webRequestWrapper) {
    webRequestWrapper->sendHeader("Location", "/", true);
    webRequestWrapper->send(302, "text/plain", "Config saved");
}

void handleRoot(AsyncWebServerRequest* request) {
    AsyncWebRequestWrapper asyncWebRequestWrapper(request);
    if (iotWebConf.handleCaptivePortal(&asyncWebRequestWrapper)) {
        return;
    }
    MyHtmlRootFormatProvider fp_;

    String response_ = "";
    response_ += fp_.getHtmlHead(iotWebConf.getThingName());
    response_ += fp_.getHtmlStyle();
    response_ += fp_.getHtmlHeadEnd();
    response_ += fp_.getHtmlScript();

    response_ += fp_.getHtmlTable();
    response_ += fp_.getHtmlTableRow() + fp_.getHtmlTableCol();

    response_ += F("<fieldset align=left style=\"border: 1px solid\">\n");
    response_ += F("<table border=\"0\" align=\"center\" width=\"100%\">\n");
    response_ += F("<tr><td align=\"left\"><span id=\"DateTimeValue\">not valid</span></td></td><td align=\"right\"><span id=\"RSSIValue\">-100</span></td></tr>\n");
    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlFieldsetEnd();

    response_ += F("<fieldset align=left style=\"border: 1px solid\">\n");
    response_ += F("<legend>Aktuelle Werte</legend>\n");
    response_ += F("<table border=\"0\" align=\"center\" width=\"100%\">\n");

    for (uint8_t ch_ = 1; ch_ <= 3; ++ch_) {
        String id_ = "Channel" + String(ch_) + "Value";
        response_ += fp_.getHtmlTableRowSpan(String("Channel ") + String(ch_), "no data", id_);
    }

    response_ += fp_.getHtmlTableRowSpan("Relay", "no data", "RelayValue");
	response_ += fp_.getHtmlTableRowSpan("Temperature", "no data", "TemperatureValue");
	response_ += fp_.getHtmlTableRowSpan("Heater", "no data", "HeaterValue");

    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlFieldsetEnd();

    response_ += F("<fieldset align=left style=\"border: 1px solid\">\n");
    response_ += F("<legend>Manual Output Control</legend>\n");
    response_ += F("<table border=\"0\" align=\"center\" width=\"100%\">\n");
    for (uint8_t ch_ = 0; ch_ < 3; ++ch_) {
        String id_ = "Channel" + String(ch_ + 1);
        response_ += "<tr><td>" + id_ + "</td><td>";
        response_ += "<div class='btn-group' id='" + id_ + "Btns'>";
        OutputStatus outputStatus_ = outputStatus[ch_];
        response_ += "<button class='output-btn on" + String(outputStatus_ == OUTPUT_ON ? " active" : "") + "' data-ch='" + String(ch_ + 1) + "' data-state='on'>On</button>";
        response_ += "<button class='output-btn off" + String(outputStatus_ == OUTPUT_OFF ? " active" : "") + "' data-ch='" + String(ch_ + 1) + "' data-state='off'>Off</button>";
        response_ += "<button class='output-btn auto" + String(outputStatus_ == OUTPUT_AUTO ? " active" : "") + "' data-ch='" + String(ch_ + 1) + "' data-state='auto'>Auto</button>";
        int brightnessValue_ = (int)(getChannelValue(ch_ + 1));
        int inputValue_ = (outputStatus_ == OUTPUT_ON) ? brightnessValue_ : 0;
        response_ += "<input type='number' min='0' max='100' id='brightnessCh" + String(ch_ + 1) + "' name='brightnessCh" + String(ch_ + 1) + "' value='" + String(inputValue_) + "' style='width:40px; margin-left:10px;'";
        if (outputStatus_ != OUTPUT_ON) {
            response_ += " disabled";
        }
        response_ += ">";
        response_ += "</div>";
        response_ += "</td></tr>";
    }
    response_ += "<tr><td>Relay</td><td>";
    response_ += "<div class='btn-group' id='RelayBtns'>";
    OutputStatus relayStatus_ = outputStatus[3];
    response_ += "<button class='output-btn on" + String(relayStatus_ == OUTPUT_ON ? " active" : "") + "' data-ch='relay' data-state='on'>On</button>";
    response_ += "<button class='output-btn off" + String(relayStatus_ == OUTPUT_OFF ? " active" : "") + "' data-ch='relay' data-state='off'>Off</button>";
    response_ += "<button class='output-btn auto" + String(relayStatus_ == OUTPUT_AUTO ? " active" : "") + "' data-ch='relay' data-state='auto'>Auto</button>";
    response_ += "</div></td></tr>";

    // Heater Buttons
    response_ += "<tr><td>Heater</td><td>";
    response_ += "<div class='btn-group' id='HeaterBtns'>";
    HeaterStatus heaterStatus_ = heaterStatus;
    response_ += "<button class='output-btn on" + String(heaterStatus_ == HEATER_ON ? " active" : "") + "' data-ch='heater' data-state='on'>On</button>";
    response_ += "<button class='output-btn off" + String(heaterStatus_ == HEATER_OFF ? " active" : "") + "' data-ch='heater' data-state='off'>Off</button>";
    response_ += "<button class='output-btn auto" + String(heaterStatus_ == HEATER_AUTO ? " active" : "") + "' data-ch='heater' data-state='auto'>Auto</button>";
    response_ += "</div></td></tr>";


    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlFieldsetEnd();

    response_ += fp_.addNewLine(2);

    response_ += fp_.getHtmlTable();

    response_ += fp_.getHtmlTableRowText("Open <a href='/config'>configuration page</a>.");
    response_ += fp_.getHtmlTableRowText("Open <a href='/webserial' target='_blank'>WebSerial Console</a> for live logs.");
    response_ += fp_.getHtmlTableRowText(fp_.getHtmlVersion(Version));
    response_ += fp_.getHtmlTableEnd();

    response_ += fp_.getHtmlTableColEnd() + fp_.getHtmlTableRowEnd();
    response_ += fp_.getHtmlTableEnd();
    response_ += fp_.getHtmlEnd();

    request->send(200, "text/html", response_);

}

void handleConnectWifi(const char* ssid, const char* password) {
    Serial.println("Connecting to WiFi ...");
    WiFi.begin(ssid, password);
}

iotwebconf::WifiAuthInfo* handleConnectWifiFailure() {
    static iotwebconf::WifiAuthInfo auth_;
    auth_ = iotWebConf.getWifiAuthInfo();
    return &auth_;
}

void handleWificonnected() {
    ArduinoOTA.begin();
	ArduinoOTA.setHostname(iotWebConf.getThingName());
}

void handleWebSerialCommand(const String& command) {
    if (command.startsWith("scale ")) {
        int value = command.substring(6).toInt();
        if (value >= 1 && value <= 60) {
            speedFactor = value;
            updateSceneTimer();
            SERIAL_WEB_SERIALLN("Speed factor set to: " + String(speedFactor));
        }
        else {
            SERIAL_WEB_SERIALLN("Invalid scale value. Use 1-60.");
        }
    }
    if (command.startsWith("setminutes ")) {
        int value = command.substring(10).toInt();
        if (value >= 0 && value < 1440) {
            if (speedFactor > 1) {
				setScaledMinutes(value);
                SERIAL_WEB_SERIALLN("Minutes set to: " + String(value));
			} else {
				SERIAL_WEB_SERIALLN("Speed factor is 1, cannot set minutes.");
			}
            
        } else {
            SERIAL_WEB_SERIALLN("Invalid minutes value. Use 0-1439.");
        }
        updateSceneTimer();
    }
    else {
        SERIAL_WEB_SERIALLN("Unknown command.");
    }
}

void handleOnProgress(size_t prg, size_t sz) {
    static size_t lastPrinted_ = 0;
    size_t currentPercent_ = (prg * 100) / sz;

    if (currentPercent_ % 5 == 0 && currentPercent_ != lastPrinted_) {
        Serial.printf("Progress: %d%%\n", currentPercent_);
        lastPrinted_ = currentPercent_;
    }
}
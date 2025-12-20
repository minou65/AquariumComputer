// NTPSettings.h

#ifndef _NTPSETTINGS_h
#define _NTPSETTINGS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <IotWebConfOptionalGroup.h>

#define STRING_LEN 32
#define NUMBER_LEN 5

class NtpConfig : public iotwebconf::ParameterGroup {
public:
    NtpConfig()
        : iotwebconf::ParameterGroup("TimeSourceGroup", "Time Source") {
        snprintf(_UseNtpValue, sizeof(_UseNtpValue), "%s", "1");
        snprintf(_NtpServerValue, sizeof(_NtpServerValue), "%s", "pool.ntp.org");
        snprintf(_TimeZoneValue, sizeof(_TimeZoneValue), "%s", "CET-1CEST,M3.5.0,M10.5.0/3");

        addItem(&_UseNtpParam);
        addItem(&_NtpServerParam);
        addItem(&_TimeZoneParam);
    }

    void applyDefaultValue() {
        _UseNtpParam.applyDefaultValue();
        _NtpServerParam.applyDefaultValue();
        _TimeZoneParam.applyDefaultValue();
    }

    bool useNtpServer() const {
        return (strcmp(_UseNtpValue, "selected") == 0);
    }
    String ntpServer() const { return String(_NtpServerValue); }
    String timeZone() const { return String(_TimeZoneValue); }

private:
    char _UseNtpValue[STRING_LEN] = { 0 };
    char _NtpServerValue[STRING_LEN] = { 0 };
    char _TimeZoneValue[STRING_LEN] = { 0 };

    iotwebconf::CheckboxParameter _UseNtpParam = iotwebconf::CheckboxParameter("Use NTP server", "UseNTPServerParam", _UseNtpValue, STRING_LEN, true);
    iotwebconf::TextParameter _NtpServerParam = iotwebconf::TextParameter("NTP server (FQDN or IP address)", "NTPServerParam", _NtpServerValue, STRING_LEN, "pool.ntp.org");
    iotwebconf::TextParameter _TimeZoneParam = iotwebconf::TextParameter("POSIX timezones string", "TimeOffsetParam", _TimeZoneValue, STRING_LEN, "CET-1CEST,M3.5.0,M10.5.0/3");
};


extern NtpConfig ntpConfig;

#endif


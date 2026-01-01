// Scene.h

#ifndef _SCENE_h
#define _SCENE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <IotWebConfOptionalGroup.h>

#define STRING_LEN 32
#define NUMBER_LEN 5

#define MAX_SCENES 15
#define OUTPUT_COUNT 4 // 3 channels + relay

enum OutputStatus {
    OUTPUT_AUTO = 0,
    OUTPUT_ON,
    OUTPUT_OFF
}; 
extern OutputStatus outputStatus[OUTPUT_COUNT]; // 3 channels + relay
extern float ChannelsBrigthness[3];
extern bool updateOutputs;

void setupSceneHardware();
void setChannelValue(uint8_t channel, uint16_t value);
uint16_t getChannelValue(uint8_t channel);
bool isRelayActive();


class Scene : public iotwebconf::ChainedParameterGroup {
public:
       Scene(const char* id);
       void applyDefaultValue() override;
       uint16_t getChannelValue(uint8_t channel);
       const char* getTimeString() const;
       int getTimeMinutes() const;
       bool isJumpEnabled() const;
	   bool isRelayEnabled() const;

       void setCurrentScene(int minutes);
private:
    float getInterpolatedChannelValue(uint8_t channel, int minutes);
    Scene* findNextActive();
    void getSceneTiming(int minutes, int& duration, int& elapsed) const;
    
    String _id;
	
    char _TimeValue[STRING_LEN] = { 0 };
    char _CH1Value[NUMBER_LEN] = { 0 };
    char _CH2Value[NUMBER_LEN] = { 0 };
    char _CH3Value[NUMBER_LEN] = { 0 };
	char _RelayValue[STRING_LEN] = { 0 };
    char _JumpValue[STRING_LEN] = { 0 };

    char _TimeId[STRING_LEN];
    char _CH1Id[STRING_LEN] = { 0 };
    char _CH2Id[STRING_LEN] = { 0 };
    char _CH3Id[STRING_LEN] = { 0 };
	char _RelayId[STRING_LEN] = { 0 };
    char _JumpId[STRING_LEN] = { 0 };

    iotwebconf::TimeParameter _TimeParam = iotwebconf::TimeParameter("Time", _TimeId, _TimeValue, STRING_LEN, "00:00");
    iotwebconf::NumberParameter _CH1Param = iotwebconf::NumberParameter("Channel 1", _CH1Id, _CH1Value, NUMBER_LEN, "0", "0..100", "min='0' max='100' step='1'");
    iotwebconf::NumberParameter _CH2Param = iotwebconf::NumberParameter("Channel 2", _CH2Id, _CH2Value, NUMBER_LEN, "0", "0..100", "min='0' max='100' step='1'");
    iotwebconf::NumberParameter _CH3Param = iotwebconf::NumberParameter("Channel 3", _CH3Id, _CH3Value, NUMBER_LEN, "0", "0..100", "min='0' max='100' step='1'");
    iotwebconf::CheckboxParameter _RelayParam = iotwebconf::CheckboxParameter("Relay", _RelayId, _RelayValue, STRING_LEN, false);
    iotwebconf::CheckboxParameter _JumpParam = iotwebconf::CheckboxParameter("Jump", _JumpId, _JumpValue, STRING_LEN, false);
};

extern Scene scenes[MAX_SCENES];


#endif

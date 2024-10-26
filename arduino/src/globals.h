#include <Arduino.h>
#include <FastLED.h>

#ifdef DEBUG
#define DBGPRINT(x) Serial.println(String("[DEBUG]> ") + x)
#else
#define DBGPRINT(x) 
#endif

struct Channel {
    int id; // unique id
    bool active; // whether the channel is active
    bool muted;
    int volume; // 0% - 100%
    CRGB cMuted, cUnmuted, cInactive;
};

struct Button {
    int id; // same as Channel.id
    int pin;
    bool pressed;
};

struct Potentiometer {
    int raw; // raw value (0-255)
};

void handleButtons();
void buttonPressed(int id);
void buttonReleased(int id);

void readPotentiometers();

void parseInput(String &data);

void updateLEDs();

void parseValues(String data, int values[]);

void sendMuteState(Channel &ch);
void sendVolumeState(Channel &ch);
void sendVolumeStateAll();

void cancelStandbyMode();
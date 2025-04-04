#include <Arduino.h>
#include <FastLED.h>

#define DEBUG // must be above globals.h
#include "globals.h"

#define COM_BAUD_RATE 115200
#define COM_CONFIG SERIAL_8E1
#define COM_READ_TIMEOUT 50
#define BUFFER_SIZE 100

const int NUM_CHANNELS = 6;
const int LED_PIN = 6;
const int buttonPins[NUM_CHANNELS] = {8, 7, 5, 4, 3, 2};
const int potentiometerPins[NUM_CHANNELS] = {A2, A3, A4, A5, A6, A7};
const unsigned long STANDBY_DELAY = 7000;

Channel channel[NUM_CHANNELS];
Button button[NUM_CHANNELS];
Potentiometer potentiometers[NUM_CHANNELS];
CRGB leds[NUM_CHANNELS];

unsigned long nextStadby;
uint8_t hue;
bool connected;

char serialBuffer[BUFFER_SIZE];
int serialBufferPos;

/******** Setup function ********/
void setup() {
    connected = false;
    for (int i = 0; i < BUFFER_SIZE; i++) serialBuffer[i] = 0;
    serialBufferPos = 0;

    // Serial setup
    Serial.begin(COM_BAUD_RATE, COM_CONFIG);
    Serial.setTimeout(COM_READ_TIMEOUT);

    // LEDs setup
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_CHANNELS);
    FastLED.setBrightness(50);
    for (int i = 0; i < NUM_CHANNELS; i++) leds[i] = CRGB::Black;
    FastLED.show();

    for (int i = 0; i < NUM_CHANNELS; i++) {
        pinMode(button[i].pin, INPUT);

        channel[i] = {i, false, false, 0, CRGB::Red, CRGB::Green, CRGB::Black};
        button[i] = {i, buttonPins[i], false};
        potentiometers[i].raw = 0;
    }

    nextStadby = 0;
    hue = 0;

    // blink all LEDs
    delay(500);
    for (int i = 0; i < NUM_CHANNELS; i++) leds[i] = CRGB::Blue;
    FastLED.show();
    delay(20);
    for (int i = 0; i < NUM_CHANNELS; i++) leds[i] = CRGB::Black;
    FastLED.show();
    delay(500);

    Serial.println("READY");
}

/******** Loop function ********/
void loop() {
    handleButtons();

    readPotentiometers();

    // handle Serial I/O
    for (; (serialBufferPos < BUFFER_SIZE) && Serial.available(); serialBufferPos++) {
        connected = true;
        char c = Serial.read();
        serialBuffer[serialBufferPos] = c;

        if (serialBufferPos == BUFFER_SIZE-1 || c == '\n') {
            serialBuffer[serialBufferPos] = '\0';
            String data(serialBuffer);
            data.trim(); // delete '\r' from the end of the string
            DBGPRINT("[RX]: '" + data + "'");
            
            parseInput(data);

            serialBufferPos = -1;
            serialBuffer[0] = '\0';
        }
    }

    // after some time of inactivity, switch to "standby mode"
    if (millis() >= nextStadby) {
        updateLEDs();
    }

    FastLED.show(); // update the LEDs
    delay(10);
}

void handleButtons() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        Button &btn = button[i];

        if (digitalRead(btn.pin) == HIGH) {
            if (!btn.pressed) {
                btn.pressed = true;
                buttonPressed(btn.id);
            }
        } else {
            if (btn.pressed) {
                btn.pressed = false;
                buttonReleased(btn.id);
            }
        }
    }
}

void buttonPressed(int id) {
    Channel &ch = channel[id];

    if (ch.active) {
        ch.muted = !ch.muted;
        sendMuteState(ch);
    }
    
    cancelStandbyMode();
}

void buttonReleased(int id) {
    cancelStandbyMode();
}

void readPotentiometers() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        int raw = analogRead(potentiometerPins[i]) >> 2; // ignore 2 least significant bits because of the noise caused by the LEDs
        Potentiometer &pot = potentiometers[i];
        if (abs(pot.raw - raw) > 1) {
            // user changed the value
            pot.raw = raw;
            int val = round((100.0 / 255.0) * pot.raw); // map raw value (0-255) to (0-100)
            if (abs(val - channel[i].volume) > 0) {
                channel[i].volume = val;
                if (connected) {
                    sendVolumeState(channel[i]);
                    cancelStandbyMode();
                }
            }
        }
    }
}

void parseInput(String& data) {
    if (data.indexOf('!') == 0) {
        // command
        if (data.charAt(1) == 'A') { // channel active data
            if (data.charAt(2) == ':') { // all channels at once
                int values[NUM_CHANNELS];
                parseValues(data.substring(3, data.length()), values);

                String dbgstr;
                for (int i = 0; i < NUM_CHANNELS; i++) dbgstr += String(values[i]) + " ";
                DBGPRINT("active data: " + dbgstr);

                for (int i = 0; i < NUM_CHANNELS; i++) {
                    channel[i].active = values[i] == 1;
                }
            } else {
                // TODO
            }
        } else if (data.charAt(1) == 'M') { // channel mute data
            if (data.charAt(2) == ':') { // all channels at once
                int values[NUM_CHANNELS];
                parseValues(data.substring(3, data.length()), values);

                String dbgstr;
                for (int i = 0; i < NUM_CHANNELS; i++) dbgstr += String(values[i]) + " ";
                DBGPRINT("mute data: " + dbgstr);

                for (int i = 0; i < NUM_CHANNELS; i++) {
                    channel[i].muted = values[i] == 1;
                }
            } else {
                // TODO
            }
        }
        cancelStandbyMode();
    } else if (data.indexOf('?') == 0) {
        // request
        if (data.charAt(1) == 'V') { // send volume data
            sendVolumeStateAll();
        }
    } else {
        // invalid input
    }
}

void updateLEDs() {
    if (millis() >= nextStadby) {
        // standby mode - display the rainbow effect
        fill_rainbow(leds, NUM_CHANNELS, hue, 15);
        hue--;
    } else {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            Channel &ch = channel[i];
            Button &btn = button[i];
            if (!ch.active) {
                leds[ch.id] = btn.pressed ? CRGB::Blue : ch.cInactive;
            }
            else if (ch.muted)
                leds[ch.id] = ch.cMuted;
            else
                leds[ch.id] = ch.cUnmuted;
        }
    }
}

void parseValues(String data, int values[]) {
    int counter = 0;
	int pos = 0, prev = -1;
	while (1) {
		pos = data.indexOf('|', prev+1);
		if (pos == -1) break;
		values[counter] = data.substring(prev+1, pos).toInt();
		counter++;
		prev = pos;
	}
}

void sendMuteState(Channel &ch) {
    Serial.println("!M" + String(ch.id) + ":" + (ch.muted ? "1" : "0"));
}

void sendVolumeState(Channel &ch) {
    Serial.println("!V" + String(ch.id) + ":" + String(ch.volume));
}

void sendVolumeStateAll() {
    String str = "!V:";
    for (int i = 0; i < NUM_CHANNELS; i++) {
        str += String(channel[i].volume) + "|";
    }
    Serial.println(str);
}

void cancelStandbyMode() {
    nextStadby = millis() + STANDBY_DELAY;
    updateLEDs();
}
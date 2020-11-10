#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>    //Library to store the potentiometer values
#include <Servo.h>

// LEDs
#define pinLEDs 2
#define qtyLEDs 7
#define typeOfLEDs WS2812B    //type of LEDs i'm using
CRGB LEDs[qtyLEDs];

byte mode = 0;
bool buttonpress = 0;
bool lastbuttonpress = 0;
bool done = true;

byte red = 0;
byte green = 0;
byte blue = 0;

long time = 0;
#define debounceTime 150

void setup() {
  FastLED.addLeds<typeOfLEDs, pinLEDs, GRB>(LEDs, qtyLEDs);    //declare LEDs
  mode = min(mode, 3);
}

void loop() {
  buttonpress = false;
  if (digitalRead(1) == HIGH) buttonpress = true;
  if (millis() - time > debounceTime) {
    if (buttonpress != lastbuttonpress && !buttonpress) {
      changeMode();
      time = millis();
    }
  }
  switch (mode) {
    case 0:
      // do something
      break;
    case 1:
      // do something
      break;
    case 2:
      // do something
      break;
  }
  lastbuttonpress = buttonpress;
}

void changeMode() {
  if (mode != 2)
    mode++;
  else
    mode = 0;
}

void openPetals() {
  
}

void colorFadeInto(byte r, byte g, byte b) {
  if (r > red)
  else if (r < red)
  else
  if (g > green)
  else if (g < green)
  else
  if (b > blue)
  else if (b < blue)
  else
}
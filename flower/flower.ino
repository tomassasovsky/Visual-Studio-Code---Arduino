//#include <BlynkSimpleEsp8266.h>
#include "FastLED.h"
#define qtyLEDs 7
#define typeOfLEDs WS2812B
#define COLOR_ORDER GRB
CRGB LEDs[qtyLEDs];
#define pinLEDs D2
int r, g, b;
byte mode = 0;

#define MODE_SLEEPING 0
#define MODE_BLOOM 1
#define MODE_BLOOMING 2
#define MODE_BLOOMED 3
#define MODE_FADE 4
#define MODE_FADING 5
#define MODE_FADED 6
#define MODE_FALLINGASLEEP 7

void setup(){
  Serial.begin(9600);
  FastLED.addLeds<typeOfLEDs, pinLEDs, COLOR_ORDER>(LEDs, qtyLEDs);
}

void loop(){
  switch (mode) {
    case :
      // do something
      break;
    case :
      // do something
      break;
    default:
      // do something
  }
  servoUpdate();
}

void ledsTransition(int r, int g, int b){
  for (int i = 0; i < qtyLEDs; i++) {
    LEDs[i] = CRGB(r, g, b);
  }
  FastLED.show();
}

void servoUpdate(int position) {
  if (position > servo.read())
}

boolean openPetals() {
  if (servoPosition >= SERVO_OPEN)
    return true;
  servoPosition++;
  servo.write(servoPosition);
  return false;
}

boolean closePetals() {
  if (servoPosition <= SERVO_CLOSED)
    return true;
  servoPosition--;
  servo.write(servoPosition);
  return false;
}
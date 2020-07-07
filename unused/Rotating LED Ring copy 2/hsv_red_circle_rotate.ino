// Tutorial 8b. Programmable NeoPixel RGB LED ring
#include "FastLED.h"

// Variables that remain constant
const byte pinLEDs = 2; // Digital output pin to LED ring
const byte qtyLEDs = 21; // Number of LEDs
struct CRGB LEDs[qtyLEDs]; // Declare an array that stores each LED's data

byte ringPosition = 0; // Array position of LED that will be lit (0 to 23 = 24)

void setup(){
  FastLED.addLeds<NEOPIXEL, pinLEDs>(LEDs, qtyLEDs);
}

void loop(){
  int changeHue = 45; //yellow is 35/50 = 45, red is 0, green is 96
  FastLED.setBrightness(255);
  EVERY_N_MILLISECONDS(40){    
    fadeToBlackBy(LEDs, qtyLEDs -5, 64);  // Dims the LEDs by 64/256 (1/4) and thus sets the trail's length    
    LEDs[ringPosition] = CHSV(changeHue, 255, 255); // Sets the LED's hue according to the potentiometer rotation    
    ringPosition++; // Shifts all LEDs one step in the currently active direction    
    if (ringPosition == qtyLEDs - 5){ // If one end is reached, reset the position to loop around
      ringPosition = 0;
    }
    FastLED.show(); // Finally, display all LED's data (= illuminate the LED ring)
  }
}
#include <EEPROM.h>
#include <FastLED.h>

void setup(){
  Serial.begin(9600);
  Serial.println(EEPROM.read(0));
  Serial.println(EEPROM.read(1));
  Serial.println(EEPROM.read(2));
  Serial.println(EEPROM.read(3));
}
void loop(){

}
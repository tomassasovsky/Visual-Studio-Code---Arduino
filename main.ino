#define potPin A0
unsigned int potVal = 0;         //current pot value
unsigned int lastPotVal = 0;     //previous pot value

void setup(){
  Serial.begin(9600);
  pinMode(potPin, INPUT);
}
void loop(){
  potVal = analogRead(potPin); // Divide by 8 to get range of 0-127 for midi
  unsigned int diffPot = abs(lastPotVal - potVal);
  
  if (diffPot > 30){// If the value does not = the last value the following command is made. This is because the pot has been turned. Otherwise the pot remains the same and no midi message is output.
    lastPotVal = potVal;
    if (potVal/8 > 123) potVal = 1023;
    if (potVal < 40) potVal = 0;
    Serial.println(potVal); // Value read from potentiometer
  }
}
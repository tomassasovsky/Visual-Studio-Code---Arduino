//* In this code, i'll be testing the MIDI input and the rotary encoder


unsigned int volTr1 = 127;
unsigned int volTr2 = 127;
unsigned int volTr3 = 127;
unsigned int volTr4 = 127;

int selectedTrack = 1;

#define clockPin A0
#define dataPin A1
#define swPin A2

unsigned int lastState;
unsigned int state;


void setup(){
  pinMode(clockPin, INPUT_PULLUP);
  pinMode(dataPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);
  lastState = digitalRead(clockPin);
}

void loop(){
  MIDI.read();
  state = digitalRead(dataPin);
  
  // the button part goes in the debounce
  if(swPin == LOW && selectedTrack != 4) selectedTrack++;
  else if (swPin == LOW && selectedTrack == 4) selectedTrack = 1;
  
  if(state != lastState){
    if (digitalRead(dataPin) != state){
      if (selectedTrack == 1 && volTr1 != 127) volTr1++;
      if (selectedTrack == 2 && volTr2 != 127) volTr2++;
      if (selectedTrack == 3 && volTr3 != 127) volTr3++;
      if (selectedTrack == 4 && volTr4 != 127) volTr4++;
    }
    else {
      if (selectedTrack == 1 && volTr1 != 0) volTr1--;
      if (selectedTrack == 2 && volTr2 != 0) volTr2--;
      if (selectedTrack == 3 && volTr3 != 0) volTr3--;
      if (selectedTrack == 4 && volTr4 != 0) volTr4--;
    }
  }
  lastState = state;
}

bool connectedMIDI(){
  while (Serial.available() > 2) {
    if (Serial.available()){
      byte channelByte = Serial.read();
      byte valueByte = Serial.read();
      byte velocityByte = Serial.read();
      if (channelByte == 1 && valueByte == 29 && velocityByte == 127) return true;
      else return false;
    }
  }
}
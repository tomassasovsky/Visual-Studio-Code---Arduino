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

bool volumeChanging = false;

void setup(){
  pinMode(clockPin, INPUT_PULLUP);
  pinMode(dataPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);
  lastState = digitalRead(clockPin);
}

void loop(){
  state = digitalRead(dataPin);
  
  // the button part goes in the debounce
  if(swPin == LOW && selectedTrack != 4) selectedTrack++;
  else if (swPin == LOW && selectedTrack == 4) selectedTrack = 1;
  
  constrain(volTr1, 0, 127);
  constrain(volTr2, 0, 127);
  constrain(volTr3, 0, 127);
  constrain(volTr4, 0, 127);

  if(state != lastState){
    if (digitalRead(dataPin) != state){
      if (selectedTrack == 1) volTr1++;
      if (selectedTrack == 2) volTr2++;
      if (selectedTrack == 3) volTr3++;
      if (selectedTrack == 4) volTr4++;
      volumeChanging = true;
    }
    else {
      if (selectedTrack == 1) volTr1--;
      if (selectedTrack == 2) volTr2--;
      if (selectedTrack == 3) volTr3--;
      if (selectedTrack == 4) volTr4--;
      volumeChanging = true;
    }
  } else volumeChanging = false;

  lastState = state;
  if (volumeChanging) sendVolume();
}

void sendVolume(){
  Serial.write(176);    //176 = CC Command
  Serial.write(selectedTrack);    // Which value: if the selected Track is 1, the value sent will be 1; If it's 2, the value 2 will be sent and so on.
  if (selectedTrack == 1) Serial.write(volTr1);    //This sends the velocity, which goes from 0 to 127.
  if (selectedTrack == 2) Serial.write(volTr2);
  if (selectedTrack == 3) Serial.write(volTr3);
  if (selectedTrack == 4) Serial.write(volTr4);

}

/*bool connectedMIDI(){
  while (Serial.available() > 2) {
    if (Serial.available()){
      byte channelByte = Serial.read();
      byte valueByte = Serial.read();
      byte velocityByte = Serial.read();
      if (channelByte == 1 && valueByte == 29 && velocityByte == 127) return true;
      else return false;
    }
  }
}*/
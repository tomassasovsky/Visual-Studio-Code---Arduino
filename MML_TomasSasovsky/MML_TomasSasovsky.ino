// TODO: Add message confirmation for MIDI signals with the MIDI Read function (MIDI Library) (not even close)
//! TODO: Finish fixing the FocusLock bugs (far away)

#include <FastLED.h>    //library for controlling the LEDs
#include <EEPROM.h>    //Library to store the potentiometer values

#define pinLEDs 2
#define qtyLEDs 19
#define typeOfLEDs WS2812B    //type of LEDs i'm using
#define nonRingLEDs 7
#define modeLED 12
#define Tr1LED 13
#define Tr2LED 14
#define Tr3LED 15
#define Tr4LED 16
#define clearLED 17
#define X2LED 18
CRGB LEDs[qtyLEDs]; 

// PINS FOR BUTTONS
#define recPlayButton 3
#define stopButton 4
#define undoButton 5
#define modeButton 6
#define track1Button 7
#define track2Button 8
#define track3Button 9
#define track4Button 10
#define clearButton 11
#define X2Button 12
#define dataPin A0
#define clockPin A1
#define swPin A2

unsigned int lastState;
unsigned int state;
unsigned int counterLEDs = 0;
byte vol[4];
byte notMutedInput;

unsigned long time = 0;    //the last time the output pin was toggled
unsigned long time2 = 0;
unsigned long timeVolume = 0;
unsigned long timeSleepMode = 0;
unsigned long lastLEDsMillis = 0;
#define debounceTime 150    //the debounce time, increase if the output flickers
#define doublePressClearTime 750

char State[4] = {'E', 'E', 'E', 'E'};

//the next tags are to make it simpler to select a track when used in a function
#define TR1 0
#define TR2 1
#define TR3 2
#define TR4 3

byte ringPosition = 0;  //the led ring has 12 leds. This variable is to set the position in which the first lit led is during the animation.
int setRingColour = 0;    //yellow is 35/50 = 45, red is 0, green is 96
#define ringSpeed  54.6875    //46.875 62.5

/*
when Rec/Play is pressed in Play Mode, every track gets played, no matter if they are empty or not.
because of this, we need to keep an even tighter tracking of each of the track's states
so if Rec/Play is pressed in Play Mode and, for example, the track 3 is empty, the corresponding variable (Tr3PlayedWRecPlay) turns true
likewise, when the button Stop is pressed, every track gets muted, so every one of these variables turn false.
*/
bool PlayedWRecPlay[4] = {false, false, false, false};
bool PressedInStop[4] = {false, false, false, false};    //when a track is pressed-in-stop, it gets focuslocked
bool previousPlay = false;    //this variable is for when the stop mode has been used and RecPlay is pressed twice. Basically, if the focuslock function is active in any of the tracks, when pressed once, the focuslocked ones get played. If it gets pressed again, every track gets played

/*
Selected track, to record on it without having to press the individual track.
Plus, if you record using the RecPlayButton, you will also overdub, unlike pressing the track button, 
which would only record and then play (no overdub).
*/

byte selectedTrack = TR1;    //this variable is to keep constant track of the selected track
byte firstRecordedTrack;    //the firstRecordedTrack variable is for when you start recording but the time is not set in Mobius, so that you cant start recording on another track until the first is played.
bool firstRecording = true;    //this track is only true when the pedal has not been used to record yet.
bool stopMode = false;
bool stopModeUsed = false;
bool doublePressClear = false;
bool playMode = false;    //LOW = Rec mode, HIGH = Play Mode
bool sleepMode = false;
bool startLEDsDone = false;
bool ledsON = false;

String buttonpress = "";    //To store the button pressed at the time
String lastbuttonpress = "";    //To store the previous button pressed

#define midichannel 0x90

void setup() {
  //Serial.begin(38400);    //to use LoopMIDI and Hairless MIDI Serial
  Serial.begin(31250);    //to use only the Arduino UNO (Atmega16u2). You must upload another bootloader to use it as a native MIDI-USB device.
  FastLED.addLeds<typeOfLEDs, pinLEDs, GRB>(LEDs, qtyLEDs);    //declare LEDs
  for (int i = 0; i < 4; i++) vol[i] = EEPROM.read(i);
  for (int i = 3; i <= 12; i++) pinMode(i, INPUT_PULLUP);
  pinMode(dataPin, INPUT_PULLUP);
  pinMode(clockPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);
  lastState = digitalRead(clockPin);
  sendNote(0x1F);    //resets the pedal
  muteAllInputsButSelected();
}

void loop() {
  if ((State[TR1] != 'E' || State[TR2] != 'E' || State[TR3] != 'E' || State[TR4] != 'E') && !stopMode) ringLEDs();    //the led ring spins when the pedal has been used.
  if (State[TR1] == 'E' && State[TR2] == 'E' && State[TR3] == 'E' && State[TR4] == 'E' && !firstRecording) reset();   //reset the pedal when every track is empty but it has been used (for example, when the only track playing is Tr1 and you clear it, it resets the whole pedal)
  if (digitalRead(clearButton)) doublePressClear = false;
  
  if (PressedInStop[TR1] || PressedInStop[TR2] || PressedInStop[TR3] || PressedInStop[TR4]) stopModeUsed = true;
  else stopModeUsed = false;
  
  if (State[selectedTrack] == 'R' && firstRecording) firstRecordedTrack = selectedTrack;
  else firstRecordedTrack = 5;

  if (!startLEDsDone) startLEDs();
  else setLEDs();

  if (millis() - timeVolume > 5000){
    if (firstRecording){
      sendAllVolumes();
      muteAllInputsButSelected();
      timeVolume = millis();
    }
  }

  if (millis() - timeSleepMode > 90000) {
    if (firstRecording && State[TR1] == 'E' && State[TR2] == 'E' && State[TR3] == 'E' && State[TR4] == 'E') {
      sleepMode = true;
      timeSleepMode = millis();
    }
  }

  selectedTrack = constrain(selectedTrack, 0, 3);
  
  buttonpress = "released";   // release the variable buttonpress every time the arduino loops
  // set the variable buttonpress to a recognizable name when a button is pressed:
  if (digitalRead(recPlayButton) == LOW) buttonpress = "RecPlay";
  if (digitalRead(stopButton) == LOW) buttonpress = "Stop";
  if (digitalRead(undoButton) == LOW) buttonpress = "Undo";
  if (digitalRead(track1Button) == LOW) buttonpress = "Track1";
  if (digitalRead(track2Button) == LOW) buttonpress = "Track2";
  if (digitalRead(track3Button) == LOW) buttonpress = "Track3";
  if (digitalRead(track4Button) == LOW) buttonpress = "Track4";
  if (digitalRead(clearButton) == LOW) buttonpress = "Clear";
  if (digitalRead(X2Button) == LOW) buttonpress = "X2";
  if (digitalRead(modeButton) == LOW) buttonpress	= "Mode";
  if (digitalRead(swPin) == LOW) buttonpress = "NextTrack";

  if (millis() - time > debounceTime){    //debounce for the buttons
    
    if (buttonpress != lastbuttonpress && buttonpress != "released"){    //if the button pressed changes
      if (buttonpress == "Mode" && canChangeModeOrClearTrack()){ //if the mode Button is pressed and mode can be changed
        if (!playMode){    //if the current mode is Record Mode
          sendNote(0x29);    //send a note so that the pedal knows we've change modes
          playMode = true;    //enter play mode
        }else{
          sendNote(0x2B);
          playMode = false;    //entering rec mode
        }
        //if any of the tracks is recording when pressing the button, play them:
        for (int i = 0; i < 4; i++){
          if (State[i] != 'M' && State[i] != 'E') State[i] = 'P';
          PressedInStop[i] = false;
        }
        // reset the focuslock function:
        stopModeUsed = false;
        previousPlay = false;
        stopMode = false;
        doublePressClear = false;
        sleepMode = false;
        timeSleepMode = millis();
        time = millis();
      }
      if (buttonpress == "Undo" && firstRecordedTrack == 5){    //if the Undo button is pressed
        sendNote(0x22);    //send a note that undoes the last thing it did
        for (int i = 0; i < 4; i++){
          if (State[i] == 'R' || State[i] == 'O') State[i] = 'P'; //play a track if it is being recorded
        }
        doublePressClear = false;
        previousPlay = false;
        sleepMode = false;
        timeSleepMode = millis();
        time = millis();
      }
      if (buttonpress == "NextTrack" && selectedTrack < TR4 && canStopTrack()) selectedTrack++, sendNote(0x32), sleepMode = false, timeSleepMode = millis(), time = millis();
      else if (buttonpress == "NextTrack" && selectedTrack == TR4 && canStopTrack()) selectedTrack = TR1, sendNote(0x32), sleepMode = false, timeSleepMode = millis(), time = millis();
      if (buttonpress == "X2") {
        if ((State[selectedTrack] == 'P' || State[selectedTrack] == 'M' || State[selectedTrack] == 'E') && !firstRecording) {
          sendNote(0x20);
          doublePressClear = false;
          previousPlay = false;
          for (int i = 0; i < 4; i++){
            if (State[i] == 'M') State[i] = 'P'; //play a track if it is muted
            else if (State[i] == 'E') PlayedWRecPlay[i] = "true";
          }
        }
        time = millis();
      }
      if (!playMode){    //if the current mode is Record Mode
        if (buttonpress == "Stop" && canStopTrack()) recModeStop();
        if (buttonpress == "Track1" && (firstRecordedTrack == TR1 || firstRecordedTrack == 5)) pressedTrack(TR1);
        if (buttonpress == "Track2" && (firstRecordedTrack == TR2 || firstRecordedTrack == 5))  pressedTrack(TR2);
        if (buttonpress == "Track3" && (firstRecordedTrack == TR3 || firstRecordedTrack == 5))  pressedTrack(TR3);
        if (buttonpress == "Track4" && (firstRecordedTrack == TR4 || firstRecordedTrack == 5)) pressedTrack(TR4);
        if (buttonpress == "RecPlay") RecPlay();   //if the Rec/Play button is pressed
      }
      if (playMode){    //if the current mode is Play Mode
        if (buttonpress == "Stop"){    //if the Stop button is pressed
          sendNote(0x2A);    //send the note
          for (int i = 0; i < 4; i++){
            if (State[i] != 'E') State[i] = 'M'; //if another track is being recorded, stop and play it
            PlayedWRecPlay[i] = false;
          }
          stopMode = true;    //make the stopMode variable and mode true
          previousPlay = false;
          doublePressClear = false;
          sleepMode = false;
          timeSleepMode = millis();
          time = millis();
        }
        if (buttonpress == "RecPlay"){   //if the Rec/Play button is pressed
          sendNote(0x31);    //send the note
          if (stopMode) ringPosition = 0;    //if we are in stop mode, make the ring spin from the position 0
          if (!stopModeUsed){    //if stop Mode hasn't been used, just play every track that isn't empty: 
            for (int i = 0; i < 4; i++){
              if (State[i] != 'E') State[i] = 'P';
              else if (State[i] == 'E') PlayedWRecPlay[i] = true;
            }
          } else if (!previousPlay && stopModeUsed){    //if stop mode has been used and the previous pressed button isn't RecPlay
            if (!PressedInStop[TR1] && !PressedInStop[TR2] && !PressedInStop[TR3] && !PressedInStop[TR4]){ //if none of the tracks has been pressed in stop, play every track that isn't empty:
              for (int i = 0; i < 4; i++){
                if (State[i] != 'E') State[i] = 'P';
                else if (State[i] == 'E') PlayedWRecPlay[i] = true;
              }
            } else {    //if a track has been pressed and it isn't empty, play it
              for (int i = 0; i < 4; i++){
                if (PressedInStop[i]) State[i] = 'P';
              }
            }
            previousPlay = true;
          } else if (previousPlay && stopModeUsed){    //if the previous pressed button is RecPlay, play every track that is not empty:
            for (int i = 0; i < 4; i++){
              if (State[i] != 'E') State[i] = 'P';
              else if (State[i] == 'E') PlayedWRecPlay[i] = true;
            }
            previousPlay = false;
          }
          stopMode = false;
          sleepMode = false;
          timeSleepMode = millis();
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "Track1" && State[TR1] != 'E') PlayModePressedTrack(TR1), sendNote(0x2C);
        if (buttonpress == "Track2" && State[TR2] != 'E') PlayModePressedTrack(TR2), sendNote(0x2D); 
        if (buttonpress == "Track3" && State[TR3] != 'E') PlayModePressedTrack(TR3), sendNote(0x2E);
        if (buttonpress == "Track4" && State[TR4] != 'E') PlayModePressedTrack(TR4), sendNote(0x2F);
      }
      //here end the button presses
    }
    lastbuttonpress = buttonpress;
  }
  if (playMode || !playMode){
    if (millis() - time2 > doublePressClearTime){    //debounce
      if (buttonpress == "Clear" && !doublePressClear){
        if (canChangeModeOrClearTrack() && !stopMode){    //if we can clear the selected track, do it
          sendNote(0x1E);
          if (State[selectedTrack] == 'P') State[selectedTrack] = 'E', PlayedWRecPlay[selectedTrack] = false, PressedInStop[selectedTrack] = false;
          doublePressClear = true;    //activate the function that resets everything if we press the Clear button again
          previousPlay = false;
        }else if (canChangeModeOrClearTrack() && stopMode) doublePressClear = true;
        else if (!canChangeModeOrClearTrack()) reset();
        sleepMode = false;
        timeSleepMode = millis();
        time2 = millis();
      } else if (buttonpress == "Clear" && doublePressClear) reset();    //reset everything when Clear is pressed twice
    }
  }
  state = digitalRead(clockPin);
  if (state != lastState){
    if (digitalRead(dataPin) != state){
      if (vol[selectedTrack] < 127){
        vol[selectedTrack]++;
      }
    }
    else {
      if(vol[selectedTrack] > 0){
        vol[selectedTrack]--;
      }
    }
    sleepMode = false;
    timeSleepMode = millis();
    sendVolume();
  }
  lastState = state;
}

void sendNote(int pitch){
  Serial.write(midichannel);    //sends the notes in the selected channel
  Serial.write(pitch);    //sends note
  Serial.write(0x45);    //medium velocity = Note ON
}

void setLEDs(){
  if (digitalRead(!clearButton)){    //turn ON the Clear LED when the button is pressed and the pedal has been used
    if (!firstRecording){
      if (State[selectedTrack] == 'P') LEDs[clearLED] = CRGB(0,0,255);
      else if (State[selectedTrack] != 'P' && State[selectedTrack] != 'E') LEDs[clearLED] = CRGB(255,0,0);
    } else LEDs[clearLED] = CRGB(255,0,0);
  }
  if (digitalRead(clearButton)) LEDs[clearLED] = CRGB(0,0,0);    //turn OFF the Clear LED when the button is NOT pressed
  
  if (digitalRead(!X2Button) && !firstRecording) LEDs[X2LED] = CRGB(0,0,255);    //turn ON the X2 LED when the button is pressed and the pedal has been used
  else if (digitalRead(!X2Button) && firstRecording) LEDs[X2LED] = CRGB(255,0,0);
  if (digitalRead(X2Button)) LEDs[X2LED] = CRGB(0,0,0);    //turn OFF the X2 LED when the button is NOT pressed

  if (!sleepMode) {
    if (!playMode) LEDs[modeLED] = CRGB(255,0,0);    //when we are in Record mode, the LED is red
    else LEDs[modeLED] = CRGB(0,255,0);    //when we are in Play mode, the LED is green
  } else LEDs[modeLED] = CRGB(0,0,0);    //when we are in sleep mode

  if (State[TR1] == 'R' || State[TR1] == 'O') LEDs[Tr1LED] = CRGB(255,0,0);    //when track 1 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR1] == 'P') LEDs[Tr1LED] = CRGB(0,255,0);    //when track 1 is playing, turn on it's LED, colour green
  else if (State[TR1] == 'M' || State[TR1] == 'E') LEDs[Tr1LED] = CRGB(0,0,0);    //when track 1 is muted or empty, turn off it's LED
  
  if (State[TR2] == 'R' || State[TR2] == 'O') LEDs[Tr2LED] = CRGB(255,0,0);    //when track 2 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR2] == 'P') LEDs[Tr2LED] = CRGB(0,255,0);    //when track 2 is playing, turn on it's LED, colour green
  else if (State[TR2] == 'M' || State[TR2] == 'E') LEDs[Tr2LED] = CRGB(0,0,0);    //when track 2 is muted or empty, turn off it's LED
  
  if (State[TR3] == 'R' || State[TR3] == 'O') LEDs[Tr3LED] = CRGB(255,0,0);    //when track 3 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR3] == 'P') LEDs[Tr3LED] = CRGB(0,255,0);    //when track 3 is playing, turn on it's LED, colour green
  else if (State[TR3] == 'M' || State[TR3] == 'E') LEDs[Tr3LED] = CRGB(0,0,0);    //when track 3 is muted or empty, turn off it's LED
  
  if (State[TR4] == 'R' || State[TR4] == 'O') LEDs[Tr4LED] = CRGB(255,0,0);    //when track 4 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR4] == 'P') LEDs[Tr4LED] = CRGB(0,255,0);    //when track 4 is playing, turn on it's LED, colour green
  else if (State[TR4] == 'M' || State[TR4] == 'E') LEDs[Tr4LED] = CRGB(0,0,0);    //when track 4 is muted or empty, turn off it's LED

  //the setRingColour variable sets the LED Ring's colour. 0 is red, 60 is yellow(ish), 96 is green.
  if ((State[TR1] == 'R' || State[TR2] == 'R' || State[TR3] == 'R' || State[TR4] == 'R') && firstRecording) setRingColour = 0;    //sets the led ring to red (recording)
  else if ((State[TR1] == 'R' || State[TR2] == 'R' || State[TR3] == 'R' || State[TR4] == 'R') && !firstRecording) setRingColour = 60;    //sets the led ring to yellow (overdubbing)
  else if (State[TR1] == 'O' || State[TR2] == 'O' || State[TR3] == 'O' || State[TR4] == 'O') setRingColour = 60;    //sets the led ring to yellow (overdubbing)
  else if (((State[TR1] == 'M' || State[TR1] == 'E') && (State[TR2] == 'M' || State[TR2] == 'E') && (State[TR3] == 'M' || State[TR3] == 'E') && (State[TR4] == 'M' || State[TR4] == 'E')) && !firstRecording && !PlayedWRecPlay[TR1] && !PlayedWRecPlay[TR2] && !PlayedWRecPlay[TR3] && !PlayedWRecPlay[TR4]) stopMode = true;    //stop the spinning ring animation when all tracks are muted
  else if (firstRecording) setRingColour = 0;
  else setRingColour = 96;    //sets the led ring to green (playing)
  
  FastLED.show();    //updates the led states
}

void reset(){    //function to reset the pedal
  sendNote(0x1F);    //sends note that resets Mobius
  selectedTrack = TR1;    //selects the 1rst track
  muteAllInputsButSelected();
  sendAllVolumes();
  sleepMode = false;
  timeSleepMode = millis();
  //empties the tracks:
  for (int i = 0; i < 4; i++){
    State[i] = 'E';
    PressedInStop[i] = false;
    PlayedWRecPlay[i] = false;
  }
  stopModeUsed = false;
  stopMode = false;
  firstRecording = true;    //sets the variable firstRecording to true
  previousPlay = false;   
  sendNote(0x2B);    //sends the Record Mode note
  playMode = false;    //into record mode
  //sends potentiometer value:
  ringPosition = 0;    //resets the ring position
  startLEDs();    //animation to show the pedal has been reset
  doublePressClear = false;
  startLEDsDone = false;
  time = millis();
}

void startLEDs() {    //animation to show the pedal has been reset
  if (millis() - lastLEDsMillis > 150){
    if (counterLEDs % 2 == 0 && counterLEDs < 6) fill_solid(LEDs, qtyLEDs, CRGB::Red), counterLEDs++;
    else if (counterLEDs % 2 != 0 && counterLEDs < 6) FastLED.clear(), counterLEDs++;
    FastLED.show();
    if (counterLEDs == 6) startLEDsDone = true, counterLEDs = 0;
    lastLEDsMillis = millis();
  }
  setRingColour = 0;    //sets the led ring colour to green
}

void ringLEDs(){    //function that makes the spinning animation for the led ring
  FastLED.setBrightness(255);
  EVERY_N_MILLISECONDS(ringSpeed){    //this gets an entire spin in 3/4 of a second (0.75s) Half a second would be 31.25, a second would be 62.5
    fadeToBlackBy(LEDs, qtyLEDs - nonRingLEDs, 70); //Dims the LEDs by 64/256 (1/4) and thus sets the trail's length. Also set limit to the ring LEDs quantity and exclude the mode and track ones (qtyLEDs-7)
    LEDs[ringPosition] = CHSV(setRingColour, 255, 255);    //Sets the LED's hue according to the potentiometer rotation    
    ringPosition++;    //Shifts all LEDs one step in the currently active direction    
    if (ringPosition == qtyLEDs - nonRingLEDs) ringPosition = 0;    //If one end is reached, reset the position to loop around
    FastLED.show();    //Finally, display all LED's data (illuminate the LED ring)
  }
}

void sendAllVolumes(){
  for (int i = 0; i <= TR4; i++){
    Serial.write(176);    //176 = CC Command
    Serial.write(i);    // Which value: if the selected Track is 1, the value sent will be 1; If it's 2, the value 2 will be sent and so on.
    Serial.write(vol[i]);
  }
}

void sendVolume(){
  Serial.write(176);    //176 = CC Command
  Serial.write(selectedTrack);    // Which value: if the selected Track is 1, the value sent will be 1; If it's 2, the value 2 will be sent and so on.
  Serial.write(vol[selectedTrack]);
  EEPROM.write(selectedTrack, vol[selectedTrack]);
}

void muteAllInputsButSelected(){
  if (selectedTrack == TR1) notMutedInput = 5;
  else if (selectedTrack == TR2) notMutedInput = 6;
  else if (selectedTrack == TR3) notMutedInput = 7;
  else if (selectedTrack == TR4) notMutedInput = 8;
  for (int i = 5; i <= 8; i++) {
    if (i != notMutedInput) {
      Serial.write(176);
      Serial.write(i);
      Serial.write(0);
    }
  }
}

void RecPlay(){
  muteAllInputsButSelected();
  sendVolume();
  sendNote(0x27);    //send the note
  stopMode = false;
  if (firstRecording){
    if (State[selectedTrack] != 'R') State[selectedTrack] = 'R';    //if the track is either empty or playing, record
    else State[selectedTrack] = 'O', firstRecording = false;    //if the track is recording, overdub
  } else {
    if (State[selectedTrack] == 'P' || State[selectedTrack] == 'E'){
      for (int i = 0; i < 4; i++){
        if (State[i] == 'R' || State[i] == 'O') State[i] = 'P'; //if another track is being recorded, stop and play it
      }
      State[selectedTrack] = 'O';   //if it's playing or empty, overdub
    } else if (State[selectedTrack] == 'R') {
      State[selectedTrack] = 'P';    
      if (selectedTrack != TR4) selectedTrack++;    //if the track is recording, play it and select the next track
      else selectedTrack = TR1;
    }
    else if (State[selectedTrack] == 'O') {
      State[selectedTrack] = 'P';
      if (selectedTrack != TR4) selectedTrack++;    //if the track is overdubbing, play and select the next track
      else selectedTrack = TR1;
    } 
    else if (State[selectedTrack] == 'M') State[selectedTrack] = 'R';    //if the track is muted, record
  }
  sleepMode = false;
  timeSleepMode = millis();
  time = millis();
  doublePressClear = false;
}

void pressedTrack(byte track){
  selectedTrack = track;    //select the track 1
  muteAllInputsButSelected();
  if (track == TR1) sendNote(0x23);    //send the note
  else if (track == TR2) sendNote(0x24);    //send the note
  else if (track == TR3) sendNote(0x25);    //send the note
  else if (track == TR4) sendNote(0x26);    //send the note
  stopMode = false;
  if (State[track] == 'P' || State[track] == 'E' || State[track] == 'M'){    //if it's playing, empty or muted, record it
    for (int i = 0; i < 4; i++){
      if (State[i] == 'R' || State[i] == 'O') State[i] = 'P'; //if another track is being recorded, stop and play it
    }
    State[track] = 'R';
  } else if (State[track] == 'R') State[track] = 'P', firstRecording = false;    //if the track is recording, play it
  else if (State[track] == 'O') State[track] = 'P', firstRecording = false;    //if the track is overdubbing, play it
  doublePressClear = false;
  sleepMode = false;
  timeSleepMode = millis();
  time = millis();
}

void recModeStop(){
  if (State[selectedTrack] != 'O' && State[selectedTrack] != 'R' && State[selectedTrack] != 'E'){
    sendNote(0x28);    //send the note
    State[selectedTrack] = 'M';
    doublePressClear = false;
  }
  sleepMode = false;
  timeSleepMode = millis();
  time = millis();
}

void PlayModePressedTrack(byte track) {
  if (stopMode){
    PressedInStop[track] = !PressedInStop[track];    //toggle between the track being pressed in stop mode and not
    selectedTrack = track;    //select the track
    doublePressClear = false;
    time = millis();
  } else {
    if (PressedInStop[track] && State[track] == 'P') previousPlay = true, State[track] = 'M';
    else if (PressedInStop[track] && State[track] == 'M') previousPlay = true, State[track] = 'P';
    else if (!PressedInStop[track]){    //if it wasn't pressed in stop
      if (State[track] == 'P') State[track] = 'M';    //and it is playing, mute it
      else if (State[track] == 'M'){    //else if it is muted, play it
      //reset everything in stopMode:
        State[track] = 'P';
        stopModeUsed = false;
        for (int i = 0; i < 4; i++){
          PressedInStop[i] = false;
        }
      }
      selectedTrack = track;    //select the track
      doublePressClear = false;
      sleepMode = false;
      timeSleepMode = millis();
      time = millis();
    }
  }
}

bool canStopTrack(){
  if ((State[selectedTrack] == 'R' || State[selectedTrack] == 'O') && firstRecording) return false;
  else return true;
}

bool canChangeModeOrClearTrack(){
  if (firstRecordedTrack == 5) return true;
  else return false;
}
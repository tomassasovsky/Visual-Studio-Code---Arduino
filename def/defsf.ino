// TODO Change the potentiometer for a rotary encoder (almost done)
// TODO: Add variables for the potentiometer value for each track (done)
// TODO: Configure the encoder switch to toggle between tracks (select the next track) (i think it works)
// TODO: Add message confirmation for MIDI signals with the MIDI Read function (MIDI Library) (not even close)
//! TODO: Finish fixing the FocusLock bugs (far away)

#include <FastLED.h>    //library for controlling the LEDs

#define pinLEDs 2
#define qtyLEDs 19
#define typeOfLEDs WS2812B    //type of LEDs i'm using
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
#define nonRingLEDs 7

unsigned int vol[4] = {127, 127, 127, 127};

#define clockPin A0
#define dataPin A1
#define swPin A2

unsigned int lastState;
unsigned int state;

int redOn;

bool volumeChanging = false;

bool playMode = LOW;    //LOW = Rec mode, HIGH = Play Mode
long time = 0;    //the last time the output pin was toggled
long time2 = 0;
#define debounceTime 150    //the debounce time, increase if the output flickers (was 150)
#define doublePressClearTime 750

String State[4] = {"empty", "empty", "empty", "empty"};

#define TR1 0
#define TR2 1
#define TR3 2
#define TR4 3

/*
when Rec/Play is pressed in Play Mode, every track gets played, no matter if they are empty or not.
because of this, we need to keep an even tighter tracking of each of the track's states
so if Rec/Play is pressed in Play Mode and, for example, the track 3 is empty, the corresponding variable (Tr3PlayedWRecPlay) turns true
likewise, when the button Stop is pressed, every track gets muted, so every one of these variables turn false.
*/
bool PlayedWRecPlay[4] = {false, false, false, false};
bool PressedInStop[4] = {false, false, false, false};    //when a track is pressed-in-stop, it gets focuslocked
bool previousPlay = false;    //this variable is for when the stop mode has been used and RecPlay is pressed twice
bool canStopTrack;
/*
Selected track, to record on it without having to press the individual track.
Plus, if you record using the RecPlayButton, you will also overdub, unlike pressing the track button, 
which would only record and then play (no overdub).
*/

int selectedTrack = TR1;

//the firstTrack variable is for when you start recording but the time is not set in Mobius, so that you cant start recording on another track until the first is played.
int firstTrack;
bool firstRecording = true;
bool stopMode = false;
bool stopModeUsed = false;
bool canClearTrack;
bool doublePressClear = false;

String buttonpress = "";    //To store the button pressed at the time
String lastbuttonpress = "";    //To store the previous button pressed
#define midichannel 0x90

#define modeLED 12
#define Tr1LED 13
#define Tr2LED 14
#define Tr3LED 15
#define Tr4LED 16
#define clearLED 17
#define X2LED 18

byte ringPosition = 0;    //Array position of LED that will be lit (0 to 15)
int setHue = 96;    //yellow is 35/50 = 45, red is 0, green is 96
#define ringSpeed  54.6875    //46.875 62.5

void setup() {
  //Serial.begin(38400);    //to use LoopMIDI and Hairless MIDI Serial
  Serial.begin(31250);    //to use only the Arduino UNO (Atmega16u2). You must upload another bootloader to use it as a native MIDI-USB device.
  FastLED.addLeds<typeOfLEDs, pinLEDs, GRB>(LEDs, qtyLEDs);    //declare LEDs
  pinMode(recPlayButton, INPUT_PULLUP);    //declare buttons as INPUT_PULLUP
  pinMode(stopButton, INPUT_PULLUP);
  pinMode(undoButton, INPUT_PULLUP);
  pinMode(modeButton, INPUT_PULLUP);
  pinMode(track1Button, INPUT_PULLUP);
  pinMode(track2Button, INPUT_PULLUP);
  pinMode(track3Button, INPUT_PULLUP);
  pinMode(track4Button, INPUT_PULLUP);
  pinMode(clearButton, INPUT_PULLUP);
  pinMode(X2Button, INPUT_PULLUP);
  pinMode(clockPin, INPUT_PULLUP);
  pinMode(dataPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);
  lastState = digitalRead(clockPin);
  sendNote(0x1F);    //resets the pedal
  sendNote(0x2B);    //gets into Record Mode
  startLEDs();    //animation for the LEDs when the pedal is reset
  setLEDs();    //matches the LEDs states with the tracks states
}

void loop() {
  buttonpress = "released";   //release the variable buttonpress every time the void loops
  
  if ((State[TR1] != "empty" || State[TR2] != "empty" || State[TR3] != "empty" || State[TR4] != "empty") && !stopMode) ringLEDs();    //the led ring spins when the pedal is recording, overdubbing or playing.
  if ((State[selectedTrack] == "recording" || State[selectedTrack] == "overdubbing") && firstRecording) canStopTrack = false;
  else canStopTrack = true;    //you can only clear the selected track and change between modes when you are not recording for the first time.
  if (firstTrack == 5) canClearTrack = true;
  else canClearTrack = false;
  if (State[TR1] == "empty" && State[TR2] == "empty" && State[TR3] == "empty" && State[TR4] == "empty" && !firstRecording) reset();   //reset the pedal when every track is empty but it has been used (for example, when the only track playing is Tr1 and you clear it, it resets the whole pedal)
  if (PressedInStop[TR1] || PressedInStop[TR2] || PressedInStop[TR3] || PressedInStop[TR4]) stopModeUsed = true;
  else stopModeUsed = false;
  if (digitalRead(clearButton) == HIGH) doublePressClear = false;

  if (State[selectedTrack] == "recording" && firstRecording) firstTrack = selectedTrack;
  else firstTrack = 5;

  state = digitalRead(dataPin);
  
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

  if (millis() - time > debounceTime){    //debounce
    if (buttonpress != lastbuttonpress && buttonpress != "released"){    //in any mode
      if (buttonpress == "Mode" && firstTrack == 5){ //if modeButton is pressed and mode can be changed
        if (playMode == LOW){    //if the current mode is Record Mode
          sendNote(0x29);    //send a note so that the pedal knows we've change modes
          playMode = HIGH;    //enter play mode
        }else{
          sendNote(0x2B);
          playMode = LOW;    //entering rec mode
        }
        //if any of the tracks is recording when pressing the button, play them:
        for (int i = 0; i < 4; i++){
          if (State[i] != "muted" && State[i] != "empty") State[i] = "playing";
          PressedInStop[i] = false;
        }
        // reset the focuslock function:
        stopModeUsed = false;
        previousPlay = false;
        stopMode = false;
        doublePressClear = false;
        time = millis();
      }
      if (buttonpress == "Undo" && firstTrack == 5){    //if the Undo button is pressed
        sendNote(0x22);    //send a note that undoes the last thing it did
        for (int i = 0; i < 4; i++){
          if (State[i] == "recording" || State[i] == "overdubbing") State[i] = "playing"; //play a track if it is being recorded
        }
        doublePressClear = false;
        previousPlay = false;
        time = millis();
      }
      if (buttonpress == "NextTrack" && selectedTrack != TR4) selectedTrack++;
      else if (buttonpress == "NextTrack" && selectedTrack == TR4) selectedTrack = TR1;
      if (buttonpress == "X2") {
        if ((State[selectedTrack] == "playing" || State[selectedTrack] == "muted" || State[selectedTrack] == "empty") && !firstRecording) {
          sendNote(0x20);
          doublePressClear = false;
          previousPlay = false;
          for (int i = 0; i < 4; i++){
            if (State[i] == "muted") State[i] = "playing"; //play a track if it is muted
            else if (State[i] == "empty") PlayedWRecPlay[i] = "true";
          }
        }
        time = millis();
      }
      if (playMode == LOW){    //if the current mode is Record Mode
        if (buttonpress == "Stop" && canStopTrack){    //if Stop is pressed
          if (State[selectedTrack] != "overdubbing" && State[selectedTrack] != "recording" && State[selectedTrack] != "empty"){
            sendNote(0x28);    //send the note
            State[selectedTrack] = "muted";
            doublePressClear = false;
          }
          time = millis();
        }
        if (buttonpress == "Track1" && (firstTrack == 0 || firstTrack == 5)){    //if the track 1 button is pressed
          sendNote(0x23);    //send the note
          stopMode = false;
          if (State[TR1] == "playing" || State[TR1] == "empty" || State[TR1] == "muted"){    //if it's playing, empty or muted, record it
            for (int i = 0; i < 4; i++){
              if (State[i] == "recording" || State[i] == "overdubbing") State[i] = "playing"; //if another track is being recorded, stop and play it
            }
            State[TR1] = "recording";
          }else if (State[TR1] == "recording") State[TR1] = "playing", firstRecording = false;    //if the track is recording, play it
          else if (State[TR1] == "overdubbing") State[TR1] = "playing", firstRecording = false;    //if the track is overdubbing, play it
          selectedTrack = TR1;    //select the track 1
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "Track2" && (firstTrack == 1 || firstTrack == 5)){    //if the track 1 button is pressed
          sendNote(0x24);    //send the note
          stopMode = false;
          if (State[TR2] == "playing" || State[TR2] == "empty" || State[TR2] == "muted"){    //if it's playing, empty or muted, record it
            for (int i = 0; i < 4; i++){
              if (State[i] == "recording" || State[i] == "overdubbing") State[i] = "playing"; //if another track is being recorded, stop and play it
            }
            State[TR2] = "recording";
          }else if (State[TR2] == "recording") State[TR2] = "playing", firstRecording = false;    //if the track is recording, play it
          else if (State[TR2] == ("overdubbing")) State[TR2] = "playing", firstRecording = false;    //if the track is overdubbing, play it
          selectedTrack = TR2;    //select the track 1
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "Track3" && (firstTrack == 2 || firstTrack == 5)){    //if the track 1 button is pressed
          sendNote(0x25);    //send the note
          stopMode = false;
          if (State[TR3] == "playing" || State[TR3] == "empty" || State[TR3] == "muted"){    //if it's playing, empty or muted, record it
            for (int i = 0; i < 4; i++){
              if (State[i] == "recording" || State[i] == "overdubbing") State[i] = "playing"; //if another track is being recorded, stop and play it
            }
            State[TR3] = "recording";
          }else if (State[TR3] == "recording") State[TR3] = "playing", firstRecording = false;    //if the track is recording, play it
          else if (State[TR3] == ("overdubbing")) State[TR3] = "playing", firstRecording = false;    //if the track is overdubbing, play it
          selectedTrack = TR3;    //select the track 1
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "Track4" && (firstTrack == 3 || firstTrack == 5)){    //if the track 1 button is pressed
          sendNote(0x26);    //send the note
          stopMode = false;
          if (State[TR4] == "playing" || State[TR4] == "empty" || State[TR4] == "muted"){    //if it's playing, empty or muted, record it
            for (int i = 0; i < 4; i++){
              if (State[i] == "recording" || State[i] == "overdubbing") State[i] = "playing"; //if another track is being recorded, stop and play it
            }
            State[TR4] = "recording";
          }else if (State[TR4] == "recording") State[TR4] = "playing", firstRecording = false;    //if the track is recording, play it
          else if (State[TR4] == "overdubbing") State[TR4] = "playing", firstRecording = false;    //if the track is overdubbing, play it
          selectedTrack = TR4;    //select the track 1
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "RecPlay"){    //if the Rec/Play button is pressed
          sendNote(0x27);    //send the note
          stopMode = false;
          if (firstRecording){
            if (selectedTrack == TR1){    //if the 1st track is selected and it's the first time recording
              if (State[TR1] == "playing" || State[TR1] == "empty") State[TR1] = "recording";    //if the track is either empty or playing, record
              else if (State[TR1] == "recording") State[TR1] = "overdubbing", firstRecording = false;    //if the track is recording, overdub
              else if (State[TR1] == "overdubbing") State[TR1] = "playing", selectedTrack = TR2;    //if the track is overdubbing, play and select the next track
              else if (State[TR1] == "muted") State[TR1] = "playing";    //if the track is muted, play it
            }
            else if (selectedTrack == TR2){
              if (State[TR2] == "playing" || State[TR2] == "empty") State[TR2] = "recording";    //if the track is either empty or playing, record
              else if (State[TR2] == "recording") State[TR2] = "overdubbing", firstRecording = false;    //if the track is recording, overdub
              else if (State[TR2] == "overdubbing") State[TR2] = "playing", selectedTrack = TR3;    //if the track is overdubbing, play and select the next track
              else if (State[TR2] == "muted") State[TR2] = "playing";    //if the track is muted, play it
            }
            else if (selectedTrack == TR3){    //if the 3rd track is selected and it's the first time recording
              if (State[TR3] == "playing" || State[TR3] == "empty") State[TR3] = "recording";    //if the track is either empty or playing, record
              else if (State[TR3] == "recording") State[TR3] = "overdubbing", firstRecording = false;    //if the track is recording, overdub
              else if (State[TR3] == "overdubbing") State[TR3] = "playing", selectedTrack = TR4;    //if the track is overdubbing, play and select the next track
              else if (State[TR3] == "muted") State[TR3] = "playing";    //if the track is muted, play it
            }
            else if (selectedTrack == TR4){    //if the 3rd track is selected and it's the first time recording
              if (State[TR4] == "playing" || State[TR4] == "empty") State[TR4] = "recording";    //if the track is either empty or playing, record
              else if (State[TR4] == "recording") State[TR4] = "overdubbing", firstRecording = false;    //if the track is recording, overdub
              else if (State[TR4] == "overdubbing") State[TR4] = "playing", selectedTrack = TR1;    //if the track is overdubbing, play and select the next track
              else if (State[TR4] == "muted") State[TR4] = "playing";    //if the track is muted, play it
            }
          }else {
            //if another track is being recorded, stop recording and play it:
            if (selectedTrack == TR1){
              if (State[TR1] == "playing" || State[TR1] == "empty"){
                for (int i = 0; i < 4; i++){
                  if (State[i] == "recording" || State[i] == "overdubbing") State[i] = "playing"; //if another track is being recorded, stop and play it
                }
                State[TR1] = "overdubbing";   //if it's playing or empty, overdub
              } else if (State[TR1] == "recording") State[TR1] = "playing", selectedTrack = TR2;    //if the track is recording, play it and select the next track
              else if (State[TR1] == "overdubbing")  State[TR1] = "playing", selectedTrack = TR2;    //if the track is overdubbing, play and select the next track
              else if (State[TR1] == "muted") State[TR1] = "recording";    //if the track is muted, record
            }
            else if (selectedTrack == TR2){
              if (State[TR2] == "playing" || State[TR2] == "empty"){
                for (int i = 0; i < 4; i++){
                  if (State[i] == "recording" || State[i] == "overdubbing") State[i] = "playing"; //if another track is being recorded, stop and play it
                }
                State[TR2] = "overdubbing";   //if it's playing or empty, overdub
              } else if (State[TR2] == "recording") State[TR2] = "playing", selectedTrack = TR3;    //if the track is recording, play it and select the next track
              else if (State[TR2] == "overdubbing") State[TR2] = "playing", selectedTrack = TR3;    //if the track is overdubbing, play and select the next track
              else if (State[TR2] == "muted") State[TR2] = "recording";    //if the track is muted, record
            }
            else if (selectedTrack == TR3){
              if (State[TR3] == "playing" || State[TR3] == "empty"){
                for (int i = 0; i < 4; i++){
                  if (State[i] == "recording" || State[i] == "overdubbing") State[i] = "playing"; //if another track is being recorded, stop and play it
                }
                State[TR3] = "overdubbing";   //if it's playing or empty, overdub
              } else if (State[TR3] == "recording") State[TR3] = "playing", selectedTrack = TR4;    //if the track is recording, play it and select the next track
              else if (State[TR3] == "overdubbing") State[TR3] = "playing", selectedTrack = TR4;    //if the track is overdubbing, play and select the next track
              else if (State[TR3] == "muted") State[TR3] = "recording";    //if the track is muted, record
            }
            else if (selectedTrack == TR4){
              if (State[TR4] == "playing" || State[TR4] == "empty"){
                for (int i = 0; i < 4; i++){
                  if (State[i] == "recording" || State[i] == "overdubbing") State[i] = "playing"; //if another track is being recorded, stop and play it
                }
                State[TR4] = "overdubbing";   //if it's playing or empty, overdub
              }
              else if (State[TR4] == "recording") State[TR4] = "playing", selectedTrack = TR1;    //if the track is recording, play it and select the first track
              else if (State[TR4] == "overdubbing") State[TR4] = "playing", selectedTrack = TR1;    //if the track is overdubbing, play and select the first track
              else if (State[TR4] == "muted") State[TR4] = "recording";    //if the track is muted, record
            }
          }
          time = millis();
          doublePressClear = false;
        }
      }
      if (playMode == HIGH){    //if the current mode is Play Mode
        if (buttonpress == "Stop"){    //if the Stop button is pressed
          sendNote(0x2A);    //send the note
          for (int i = 0; i < 4; i++){
            if (State[i] != "empty") State[i] = "muted"; //if another track is being recorded, stop and play it
            PlayedWRecPlay[i] = false;
          }
          stopMode = true;    //make the stopMode variable and mode true
          previousPlay = false;
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "RecPlay"){    //if the Rec/Play button is pressed
          sendNote(0x31);    //send the note
          if (stopMode) ringPosition = 0;    //if we are in stop mode, make the ring spin from the position 0
          if (!stopModeUsed){    //if stop Mode hasn't been used, just play every track that isn't empty: 
            for (int i = 0; i < 4; i++){
              if (State[i] != "empty") State[i] = "playing";
              else if (State[i] == "empty") PlayedWRecPlay[i] = true;
            }
          } else if (!previousPlay && stopModeUsed){    //if stop mode has been used and the previous pressed button isn't RecPlay
            if (!PressedInStop[TR1] && !PressedInStop[TR2] && !PressedInStop[TR3] && !PressedInStop[TR4]){ //if none of the tracks has been pressed in stop, play every track that isn't empty:
              for (int i = 0; i < 4; i++){
                if (State[i] != "empty") State[i] = "playing";
                else if (State[i] == "empty") PlayedWRecPlay[i] = true;
              }
            } else {    //if a track has been pressed and it isn't empty, play it
              for (int i = 0; i < 4; i++){
                if (PressedInStop[i]) State[i] = "playing";
              }
            }
            previousPlay = true;
          } else if (previousPlay && stopModeUsed){    //if the previous pressed button is RecPlay, play every track that is not empty:
            for (int i = 0; i < 4; i++){
              if (State[i] != "empty") State[i] = "playing";
              else if (State[i] == "empty") PlayedWRecPlay[i] = true;
            }
            previousPlay = false;
          }
          stopMode = false;
          doublePressClear = false;
          time = millis();
        }
        if (stopMode){    //if we are in stop mode
          if (buttonpress == "Track1" && State[TR1] != "empty"){    //if the track 1 button is pressed and it isn't empty
            sendNote(0x2C);    //send the note
            if (PressedInStop[TR1]) PressedInStop[TR1] = false;    //toggle between the track being pressed in stop mode and not
            else PressedInStop[TR1] = true;
            selectedTrack = TR1;    //select the track
            doublePressClear = false;
            time = millis();
          }
          if (buttonpress == "Track2" && State[TR2] != "empty"){    //if the track 2 button is pressed and it isn't empty
            sendNote(0x2D);    //send the note
            if (PressedInStop[TR2]) PressedInStop[TR2] = false;    //toggle between the track being pressed in stop mode and not
            else PressedInStop[TR2] = true;
            selectedTrack = TR2;    //select the track
            doublePressClear = false;
            time = millis();
          }
          if (buttonpress == "Track3" && State[TR3] != "empty"){    //if the track 3 button is pressed and it isn't empty
            sendNote(0x2E);    //send the note
            if (PressedInStop[TR3]) PressedInStop[TR3] = false;    //toggle between the track being pressed in stop mode and not
            else PressedInStop[TR3] = true;
            selectedTrack = TR3;    //select the track
            doublePressClear = false;
            time = millis();
          }
          if (buttonpress == "Track4" && State[TR4] != "empty"){    //if the track 4 button is pressed and it isn't empty
            sendNote(0x2F);    //send the note
            if (PressedInStop[TR4]) PressedInStop[TR4] = false;    //toggle between the track being pressed in stop mode and not
            else PressedInStop[TR4] = true;
            selectedTrack = TR4;    //select the track
            doublePressClear = false;
            time = millis();
          }
        }else if (!stopMode){    //if we aren't in stop mode
          if (buttonpress == "Track1" && State[TR1] != "empty"){    //if the track 1 button is pressed and it isn't empty
            sendNote(0x2C);    //send the note
            //toggle between playing and muted when it was pressed in stop mode:
            if (PressedInStop[TR1] && State[TR1] == "playing") previousPlay = true, State[TR1] = "muted";
            else if (PressedInStop[TR1] && State[TR1] == "muted") previousPlay = true, State[TR1] = "playing";
            else if (!PressedInStop[TR1]){    //if it wasn't pressed in stop
              if (State[TR1] == "playing") State[TR1] = "muted";    //and it is playing, mute it
              else if (State[TR1] == "muted"){    //else if it is muted, play it
              //reset everything in stopMode:
                State[TR1] = "playing";
                stopModeUsed = false;
                for (int i = 0; i < 4; i++){
                  PressedInStop[i] = false;
                }
              }
            }
            selectedTrack = TR1;    //select the track
            doublePressClear = false;
            time = millis();
          }
          if (buttonpress == "Track2" && !firstRecording && State[TR2] != "empty"){    //if the track 2 button is pressed and it isn't empty
            sendNote(0x2D);    //send the note
            //toggle between playing and muted when it was pressed in stop mode:
            if (PressedInStop[TR2] && State[TR2] == "playing") previousPlay = true, State[TR2] = "muted";
            else if (PressedInStop[TR2] && State[TR2] == "muted") previousPlay = true, State[TR2] = "playing";
            else if (!PressedInStop[TR2]){    //if it wasn't pressed in stop
              if (State[TR2] == "playing") State[TR2] = "muted";    //and it is playing, mute it
              else if (State[TR2] == "muted"){    //else if it is muted, play it
              //reset everything in stopMode:
                State[TR2] = "playing";
                stopModeUsed = false;
                for (int i = 0; i < 4; i++){
                  PressedInStop[i] = false;
                }
              }
            }
            selectedTrack = TR2;    //select the track
            doublePressClear = false;
            time = millis();
          }
          if (buttonpress == "Track3" && !firstRecording && State[TR3] != "empty"){    //if the track 3 button is pressed and it isn't empty
            sendNote(0x2E);    //send the note
            //toggle between playing and muted when it was pressed in stop mode:
            if (PressedInStop[TR3] && State[TR3] == "playing") previousPlay = true, State[TR3] = "muted";
            else if (PressedInStop[TR3] && State[TR3] == "muted") previousPlay = true, State[TR3] = "playing";
            else if (!PressedInStop[TR3]){    //if it wasn't pressed in stop
              if (State[TR3] == "playing") State[TR3] = "muted";    //and it is playing, mute it
              else if (State[TR3] == "muted"){    //else if it is muted, play it
              //reset everything in stopMode:
                State[TR3] = "playing";
                stopModeUsed = false;
                for (int i = 0; i < 4; i++){
                  PressedInStop[i] = false;
                }
              }
            }
            selectedTrack = TR3;    //select the track
            doublePressClear = false;
            time = millis();
          }
          if (buttonpress == "Track4" && !firstRecording && State[TR4] != "empty"){    //if the track 4 button is pressed and it isn't empty
            sendNote(0x2F);    //send the note
            //toggle between playing and muted when it was pressed in stop mode:
            if (PressedInStop[TR4] && State[TR4] == "playing") previousPlay = true, State[TR4] = "muted";
            else if (PressedInStop[TR4] && State[TR4] == "muted") previousPlay = true, State[TR4] = "playing";
            else if (!PressedInStop[TR4]){    //if it wasn't pressed in stop
              if (State[TR4] == "playing") State[TR4] = "muted";    //and it is playing, mute it
              else if (State[TR4] == "muted"){    //else if it is muted, play it
              //reset everything in stopMode:
                State[TR4] = "playing";
                stopModeUsed = false;
                for (int i = 0; i < 4; i++){
                  PressedInStop[i] = false;
                }
              }
            }
            selectedTrack = TR4;    //select the track
            doublePressClear = false;
            time = millis();
          }
        }
      }
      //here end the button presses
    }
    
    lastbuttonpress = buttonpress;
    setLEDs();
  }
  if (playMode || !playMode){
    if (millis() - time2 > doublePressClearTime){    //debounce
      if (buttonpress == "Clear" && !doublePressClear){
        if (canClearTrack && !stopMode){    //if we can clear the selected track, do it
          sendNote(0x1E);
          if (State[selectedTrack] == "playing") State[selectedTrack] = "empty", PlayedWRecPlay[selectedTrack] = false, PressedInStop[selectedTrack] = false;
          doublePressClear = true;    //activate the function that resets everything if we press the Clear button again
          previousPlay = false;
        }else if (canClearTrack && stopMode) doublePressClear = true;
        else if (!canClearTrack) reset();
        time2 = millis();
      }else if (buttonpress == "Clear" && doublePressClear) reset();    //reset everything when Clear is pressed twice
      if (buttonpress == "Clear" && firstRecording){    //if nothing is recorded yet, just send the potentiometer (volume) value
        volumeChanging = true;
        sendVolume();
        volumeChanging = false;
        time2 = millis();
      }
    }
  }
  if (state != lastState){
    if (digitalRead(dataPin) != state){
      vol[selectedTrack]++;
      volumeChanging = true;
    }
    else {
      vol[selectedTrack]--;
      volumeChanging = true;
    }
  } else volumeChanging = false;

  lastState = state;
  if (volumeChanging) sendVolume();
}

void sendNote(int pitch){
  Serial.write(midichannel);    //sends the notes in the selected channel
  Serial.write(pitch);    //sends note
  Serial.write(0x45);    //medium velocity = Note ON
}

void setLEDs(){
  //mode LED:
  if (playMode == LOW) LEDs[modeLED] = CRGB(255,0,0);    //when we are in Record mode, the LED is red
  if (playMode == HIGH) LEDs[modeLED] = CRGB(0,255,0);    //when we are in Play mode, the LED is green

  if (digitalRead(clearButton) == LOW){    //turn ON the Clear LED when the button is pressed and the pedal has been used
    if (!firstRecording){
      if (State[selectedTrack] == "playing") LEDs[clearLED] = CRGB(0,0,255);
      else if (State[selectedTrack] != "playing" && State[selectedTrack] != "empty") LEDs[clearLED] = CRGB(255,0,0);
    } else LEDs[clearLED] = CRGB(255,0,0);
  }
  if (digitalRead(clearButton) == HIGH) LEDs[clearLED] = CRGB(0,0,0);    //turn OFF the Clear LED when the button is NOT pressed
  
  if (digitalRead(X2Button) == LOW && !firstRecording) LEDs[X2LED] = CRGB(0,0,255);    //turn ON the X2 LED when the button is pressed and the pedal has been used
  else if (digitalRead(X2Button) == LOW && firstRecording) LEDs[X2LED] = CRGB(255,0,0);
  if (digitalRead(X2Button) == HIGH) LEDs[X2LED] = CRGB(0,0,0);    //turn OFF the X2 LED when the button is NOT pressed

  if (State[TR1] == "recording" || State[TR1] == "overdubbing") LEDs[Tr1LED] = CRGB(255,0,0);    //when track 1 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR1] == "playing") LEDs[Tr1LED] = CRGB(0,255,0);    //when track 1 is playing, turn on it's LED, colour green
  else if (State[TR1] == "muted" || State[TR1] == "empty") LEDs[Tr1LED] = CRGB(0,0,0);    //when track 1 is muted or empty, turn off it's LED
  
  if (State[TR2] == "recording" || State[TR2] == "overdubbing") LEDs[Tr2LED] = CRGB(255,0,0);    //when track 2 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR2] == "playing") LEDs[Tr2LED] = CRGB(0,255,0);    //when track 2 is playing, turn on it's LED, colour green
  else if (State[TR2] == "muted" || State[TR2] == "empty") LEDs[Tr2LED] = CRGB(0,0,0);    //when track 2 is muted or empty, turn off it's LED
  
  if (State[TR3] == "recording" || State[TR3] == "overdubbing") LEDs[Tr3LED] = CRGB(255,0,0);    //when track 3 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR3] == "playing") LEDs[Tr3LED] = CRGB(0,255,0);    //when track 3 is playing, turn on it's LED, colour green
  else if (State[TR3] == "muted" || State[TR3] == "empty") LEDs[Tr3LED] = CRGB(0,0,0);    //when track 3 is muted or empty, turn off it's LED
  
  if (State[TR4] == "recording" || State[TR4] == "overdubbing") LEDs[Tr4LED] = CRGB(255,0,0);    //when track 4 is recording or overdubing, turn on it's LED, colour red
  else if (State[TR4] == "playing") LEDs[Tr4LED] = CRGB(0,255,0);    //when track 4 is playing, turn on it's LED, colour green
  else if (State[TR4] == "muted" || State[TR4] == "empty") LEDs[Tr4LED] = CRGB(0,0,0);    //when track 4 is muted or empty, turn off it's LED

  //the setHue variable sets the LED Ring's colour. 0 is red, 60 is yellow(ish), 96 is green.
  if ((State[TR1] == "recording" || State[TR2] == "recording" || State[TR3] == "recording" || State[TR4] == "recording") && firstRecording) setHue = 0;    //sets the led ring to red (recording)
  else if ((State[TR1] == "recording" || State[TR2] == "recording" || State[TR3] == "recording" || State[TR4] == "recording") && !firstRecording) setHue = 60;    //sets the led ring to yellow (overdubbing)
  else if (State[TR1] == "overdubbing" || State[TR2] == "overdubbing" || State[TR3] == "overdubbing" || State[TR4] == "overdubbing") setHue = 60;    //sets the led ring to yellow (overdubbing)
  else if (((State[TR1] == "muted" || State[TR1] == "empty") && (State[TR2] == "muted" || State[TR2] == "empty") && (State[TR3] == "muted" || State[TR3] == "empty") && (State[TR4] == "muted" || State[TR4] == "empty")) && !firstRecording && !PlayedWRecPlay[TR1] && !PlayedWRecPlay[TR2] && !PlayedWRecPlay[TR3] && !PlayedWRecPlay[TR4]) stopMode = true;    //stop the spinning ring animation when all tracks are muted
  else setHue = 96;    //sets the led ring to green (playing)
  
  FastLED.show();    //updates the led states
}

void reset(){    //function to reset the pedal
  sendNote(0x1F);    //sends note that resets Mobius
  //empties the tracks:
  for (int i = 0; i < 4; i++){
    State[i] = "empty";
    PressedInStop[i] = false;
    PlayedWRecPlay[i] = false;
  }
  stopModeUsed = false;
  stopMode = false;
  selectedTrack = TR1;    //selects the 1rst track
  firstRecording = true;    //sets the variable firstRecording to true
  previousPlay = false;   
  sendNote(0x2B);    //sends the Record Mode note
  playMode = LOW;    //into record mode
  //sends potentiometer value:
  ringPosition = 0;    //resets the ring position
  startLEDs();    //animation to show the pedal has been reset
  doublePressClear = false;
  time = millis();
}

void startLEDs(){    //animation to show the pedal has been reset
  for (int i = 0; i < 3; i++){    //turns on and off every led 3 times in red
    for (int j = 0; j < 18; i++){
      LEDs[j] = CRGB(255,0,0);
    }
    FastLED.show();
    delay(150);
    for (int y = 0; y < 18; y++){
      LEDs[y] = CRGB(0,0,0);
    }
    FastLED.show();
    delay(150);
  }
  setHue = 96;    //sets the led ring colour to green
}

void ringLEDs(){    //function that makes the spinning animation for the led ring
  FastLED.setBrightness(255);
  EVERY_N_MILLISECONDS(ringSpeed){    //this gets an entire spin in 3/4 of a second (0.75s) Half a second would be 31.25, a second would be 62.5
    fadeToBlackBy(LEDs, qtyLEDs - nonRingLEDs, 70);    //Dims the LEDs by 64/256 (1/4) and thus sets the trail's length. Also set limit to the ring LEDs quantity and exclude the mode and track ones (qtyLEDs-7)
    LEDs[ringPosition] = CHSV(setHue, 255, 255);    //Sets the LED's hue according to the potentiometer rotation    
    ringPosition++;    //Shifts all LEDs one step in the currently active direction    
    if (ringPosition == qtyLEDs - nonRingLEDs) ringPosition = 0;    //If one end is reached, reset the position to loop around
    FastLED.show();    //Finally, display all LED's data (illuminate the LED ring)
  }
}

void sendVolume(){
  Serial.write(176);    //176 = CC Command
  Serial.write(selectedTrack);    // Which value: if the selected Track is 1, the value sent will be 1; If it's 2, the value 2 will be sent and so on.
  Serial.write(vol[selectedTrack]);
}
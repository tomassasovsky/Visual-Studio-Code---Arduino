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

unsigned int volTr1 = 127;
unsigned int volTr2 = 127;
unsigned int volTr3 = 127;
unsigned int volTr4 = 127;

#define clockPin A0
#define dataPin A1
#define swPin A2

unsigned int lastState;
unsigned int state;

bool volumeChanging = false;

bool playMode = LOW;    //LOW = Rec mode, HIGH = Play Mode
long time = 0;    //the last time the output pin was toggled
long time2 = 0;
long debounceTime = 150;    //the debounce time, increase if the output flickers (was 150)
long doublePressClearTime = 750;

String Tr1State = "empty";    //Track state, to determine how and which LED(s) must be on or off.
String Tr2State = "empty";
String Tr3State = "empty";
String Tr4State = "empty";
/*
when Rec/Play is pressed in Play Mode, every track gets played, no matter if they are empty or not.
because of this, we need to keep an even tighter tracking of each of the track's states
so if Rec/Play is pressed in Play Mode and, for example, the track 3 is empty, the corresponding variable (Tr3PlayedWRecPlay) turns true
likewise, when the button Stop is pressed, every track gets muted, so every one of these variables turn false.
*/
bool Tr1PlayedWRecPlay = false;
bool Tr2PlayedWRecPlay = false;
bool Tr3PlayedWRecPlay = false;
bool Tr4PlayedWRecPlay = false;
bool Tr1PressedInStop = false;    //when a track is pressed-in-stop, it gets focuslocked
bool Tr2PressedInStop = false;
bool Tr3PressedInStop = false;
bool Tr4PressedInStop = false;
bool previousPlay = false;    //this variable is for when the stop mode has been used and RecPlay is pressed twice
bool canStopTrack;
/*
Selected track, to record on it without having to press the individual track.
Plus, if you record using the RecPlayButton, you will also overdub, unlike pressing the track button, 
which would only record and then play (no overdub).
*/
int selectedTrack = 1;

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
  //if (firstRecording && ) sendNote();
  if ((Tr1State != "empty" || Tr2State != "empty" || Tr3State != "empty" || Tr4State != "empty") && !stopMode) ringLEDs();    //the led ring spins when the pedal is recording, overdubbing or playing.
  if ((Tr1State == "recording" || Tr1State == "overdubbing") || (Tr2State == "recording" || Tr2State == "overdubbing") || (Tr3State == "recording" || Tr3State == "overdubbing") || (Tr4State == "recording" || Tr4State == "overdubbing") && firstRecording) canStopTrack = false; 
  else canStopTrack = true;    //you can only clear the selected track and change between modes when you are not recording for the first time.
  //if (firstRecording) canChangeMode = false;
  //else canChangeMode = true;
  if (firstTrack == 0) canClearTrack = true;
  else canClearTrack = false;
  if (Tr1State == "empty" && Tr2State == "empty" && Tr3State == "empty" && Tr4State == "empty" && !firstRecording) reset();   //reset the pedal when every track is empty but it has been used (for example, when the only track playing is Tr1 and you clear it, it resets the whole pedal)
  if (Tr1PressedInStop || Tr2PressedInStop || Tr3PressedInStop || Tr4PressedInStop) stopModeUsed = true;
  else stopModeUsed = false;
  if (digitalRead(clearButton) == HIGH) doublePressClear = false;

  if (Tr1State == "recording" && firstRecording) firstTrack = 1;
  if (Tr2State == "recording" && firstRecording) firstTrack = 2;
  if (Tr3State == "recording" && firstRecording) firstTrack = 3;
  if (Tr4State == "recording" && firstRecording) firstTrack = 4;
  if (!firstRecording || Tr1State == "overdubbing" || Tr2State == "overdubbing" || Tr3State == "overdubbing" || Tr4State == "overdubbing") firstTrack = 0;

  buttonpress = "released";   //release the variable buttonpress every time the void loops

  state = digitalRead(dataPin);
    
  constrain(volTr1, 0, 127);
  constrain(volTr2, 0, 127);
  constrain(volTr3, 0, 127);
  constrain(volTr4, 0, 127);

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
      if (buttonpress == "Mode" && firstTrack == 0){ //if modeButton is pressed and mode can be changed
        if (playMode == LOW){    //if the current mode is Record Mode
          sendNote(0x29);    //send a note so that the pedal knows we've change modes
          playMode = HIGH;    //enter play mode
        }else{
          sendNote(0x2B);
          playMode = LOW;    //entering rec mode
        }
        //if any of the tracks is recording when pressing the button, play them:
        if (Tr1State != "muted" && Tr1State != "empty") Tr1State = "playing";  
        if (Tr2State != "muted" && Tr2State != "empty") Tr2State = "playing";
        if (Tr3State != "muted" && Tr3State != "empty") Tr3State = "playing";
        if (Tr4State != "muted" && Tr4State != "empty") Tr4State = "playing";
        // reset the focuslock function:
        Tr1PressedInStop = false;
        Tr2PressedInStop = false;
        Tr3PressedInStop = false;
        Tr4PressedInStop = false;
        stopModeUsed = false;
        previousPlay = false;
        stopMode = false;
        doublePressClear = false;
        time = millis();
      }
      if (buttonpress == "Undo"){    //if the Undo button is pressed
        sendNote(0x22);    //send a note that undoes the last thing it did
        //if (readMIDI(29)){
          if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";    //play a track if it is being recorded
          if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
          if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
          if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
          doublePressClear = false;
          previousPlay = false;
        //}
        time = millis();
      }
      if(buttonpress == "NextTrack" && selectedTrack != 4) selectedTrack++;
      else if (buttonpress == "NextTrack" && selectedTrack == 4) selectedTrack = 1;
      if (buttonpress == "X2") {
        if((selectedTrack == 1) && ((Tr1State == "playing") || (Tr1State == "empty" && !firstRecording) || (Tr1State == "muted"))) {
          sendNote(0x20);
          doublePressClear = false;
          previousPlay = false;
          if (Tr1State == "empty") Tr1PlayedWRecPlay = true;
          else if (Tr1State == "muted") Tr1State = "playing";
          if (Tr2State == "empty") Tr2PlayedWRecPlay = true;
          else if (Tr2State == "muted") Tr2State = "playing";
          if (Tr3State == "empty") Tr3PlayedWRecPlay = true;
          else if (Tr3State == "muted") Tr3State = "playing";
          if (Tr4State == "empty") Tr4PlayedWRecPlay = true;
          else if (Tr4State == "muted") Tr4State = "playing";
        }
        else if((selectedTrack == 2) && ((Tr2State == "playing") || (Tr2State == "empty" && !firstRecording) || (Tr2State == "muted"))) {
          sendNote(0x20);
          doublePressClear = false;
          previousPlay = false;
          if (Tr1State == "empty") Tr1PlayedWRecPlay = true;
          else if (Tr1State == "muted") Tr1State = "playing";
          if (Tr2State == "empty") Tr2PlayedWRecPlay = true;
          else if (Tr2State == "muted") Tr2State = "playing";
          if (Tr3State == "empty") Tr3PlayedWRecPlay = true;
          else if (Tr3State == "muted") Tr3State = "playing";
          if (Tr4State == "empty") Tr4PlayedWRecPlay = true;
          else if (Tr4State == "muted") Tr4State = "playing";
        }
        else if((selectedTrack == 3) && ((Tr3State == "playing") || (Tr3State == "empty" && !firstRecording) || (Tr3State == "muted"))) {
          sendNote(0x20);
          doublePressClear = false;
          previousPlay = false;
          if (Tr1State == "empty") Tr1PlayedWRecPlay = true;
          else if (Tr1State == "muted") Tr1State = "playing";
          if (Tr2State == "empty") Tr2PlayedWRecPlay = true;
          else if (Tr2State == "muted") Tr2State = "playing";
          if (Tr3State == "empty") Tr3PlayedWRecPlay = true;
          else if (Tr3State == "muted") Tr3State = "playing";
          if (Tr4State == "empty") Tr4PlayedWRecPlay = true;
          else if (Tr4State == "muted") Tr4State = "playing";
        }
        else if((selectedTrack == 4) && ((Tr4State == "playing") || (Tr4State == "empty" && !firstRecording) || (Tr4State == "muted"))) {
          sendNote(0x20);
          doublePressClear = false;
          previousPlay = false;
          if (Tr1State == "empty") Tr1PlayedWRecPlay = true;
          else if (Tr1State == "muted") Tr1State = "playing";
          if (Tr2State == "empty") Tr2PlayedWRecPlay = true;
          else if (Tr2State == "muted") Tr2State = "playing";
          if (Tr3State == "empty") Tr3PlayedWRecPlay = true;
          else if (Tr3State == "muted") Tr3State = "playing";
          if (Tr4State == "empty") Tr4PlayedWRecPlay = true;
          else if (Tr4State == "muted") Tr4State = "playing";
        }
        time = millis();
      }
      
      if (playMode == LOW){    //if the current mode is Record Mode
        if (buttonpress == "Stop" && canStopTrack){    //if Stop is pressed
          if ((selectedTrack == 1 && Tr1State != "overdubbing" && Tr1State != "recording") || (selectedTrack == 2 && (Tr2State != "overdubbing" && Tr2State != "recording")) || (selectedTrack == 3 && Tr3State != "overdubbing" && Tr3State != "recording") || (selectedTrack == 4 && Tr4State != "overdubbing" && Tr4State != "recording")){
            sendNote(0x28);    //send the note
            //if(readMIDI(29)) {
            //mute the selected track:
              if (selectedTrack == 1  && Tr1State != "empty") Tr1State = "muted";
              if (selectedTrack == 2  && Tr2State != "empty") Tr2State = "muted";
              if (selectedTrack == 3  && Tr3State != "empty") Tr3State = "muted";
              if (selectedTrack == 4  && Tr4State != "empty") Tr4State = "muted";
              doublePressClear = false;
            //}
          }
          time = millis();
        }
        if (buttonpress == "Track1" && (firstTrack == 1 || firstTrack == 0)){    //if the track 1 button is pressed
          sendNote(0x23);    //send the note
          //if(readMIDI(29)) {
            if (Tr1State == "playing" || Tr1State == "empty" || Tr1State == "muted"){    //if it's playing, empty or muted, record it
              Tr1State = "recording";
              //if another track is being recorded, stop and play it:
              if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing"; 
              if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
              if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
            }else if (Tr1State == "overdubbing") Tr1State = "playing", firstRecording = false;    //if the track is overdubbing, play it
            else if (Tr1State == "recording") Tr1State = "playing", firstRecording = false;    //if the track is recording, play it
            selectedTrack = 1;    //select the track 1
            doublePressClear = false;
          //}
          time = millis();
        }
        if (buttonpress == "Track2" && (firstTrack == 2 || firstTrack == 0)){    //if the track 2 button is pressed
          sendNote(0x24);    //send the note
          //if(readMIDI(29)) {
            if (Tr2State == "playing" || Tr2State == "empty" || Tr2State == "muted"){    //if it's playing, empty or muted, record it
              Tr2State = "recording";
              //if another track is being recorded, stop and play it:
              if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
              if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
              if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
            }else if (Tr2State == "overdubbing") Tr2State = "playing", firstRecording = false;    //if the track is overdubbing, play it
            else if (Tr2State == "recording") Tr2State = "playing", firstRecording = false;    //if the track is recording, play it
            selectedTrack = 2;    //select the track 2
            doublePressClear = false;
          //}
          time = millis();
        }
        if (buttonpress == "Track3" && (firstTrack == 3 || firstTrack == 0)){    //if the track 3 button is pressed
          sendNote(0x25);    //send the note
         // if(readMIDI(29)) {
            if (Tr3State == "playing" || Tr3State == "empty" || Tr3State == "muted"){    //if it's playing, empty or muted, record it
              Tr3State = "recording";
              //if another track is being recorded, stop and play it:
              if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
              if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
              if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
            }else if (Tr3State == "overdubbing") Tr3State = "playing", firstRecording = false;    //if the track is overdubbing, play it
            else if (Tr3State == "recording") Tr3State = "playing", firstRecording = false;    //if the track is recording, play it
            selectedTrack = 3;    //select the track 3
            doublePressClear = false;
          //}
          time = millis();
        }
        if (buttonpress == "Track4" && (firstTrack == 4 || firstTrack == 0)){    //if the track 4 button is pressed
          sendNote(0x26);    //send the note
          //if(readMIDI(29)) {
            if (Tr4State == "playing" || Tr4State == "empty" || Tr4State == "muted"){    //if it's playing, empty or muted, record it
              Tr4State = "recording";
              //if another track is being recorded, stop and play it:
              if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
              if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
              if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
            }else if (Tr4State == "overdubbing") Tr4State = "playing", firstRecording = false;    //if the track is overdubbing, play it
            else if (Tr4State == "recording") Tr4State = "playing", firstRecording = false;    //if the track is recording, play it
            selectedTrack = 4;    //select the track 4
            doublePressClear = false;
          //}
          time = millis();
        }
        if (buttonpress == "RecPlay"){    //if the Rec/Play button is pressed
          sendNote(0x27);    //send the note
          //if(readMIDI(29)) {
            if (selectedTrack == 1 && firstRecording){    //if the 1st track is selected and it's the first time recording
              if (Tr1State == "playing" || Tr1State == "empty") Tr1State = "recording";    //if the track is either empty or playing, record
              else if (Tr1State == "recording") Tr1State = "overdubbing", firstRecording = false;    //if the track is recording, overdub
              else if (Tr1State == "overdubbing") Tr1State = "playing", selectedTrack = 2;    //if the track is overdubbing, play and select the next track
              else if (Tr1State == "muted") Tr1State = "playing";    //if the track is muted, play it
            }else if (selectedTrack == 1 && !firstRecording){    //if the 1st track is selected and it's NOT the first time recording
              if (Tr1State == "playing" || Tr1State == "empty"){    //if it's playing or empty, overdub
                Tr1State = "overdubbing";
                //if another track is being recorded, stop recording and play it:
                if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
                if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
                if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
              }else if (Tr1State == "recording") Tr1State = "playing", selectedTrack = 2;    //if the track is recording, play it and select the next track
              else if (Tr1State == "overdubbing")  Tr1State = "playing", selectedTrack = 2;    //if the track is overdubbing, play and select the next track
              else if (Tr1State == "muted") Tr1State = "recording";    //if the track is muted, record
            }
            else if (selectedTrack == 2 && firstRecording){    //if the 2nd track is selected and it's the first time recording
              if (Tr2State == "playing" || Tr2State == "empty") Tr2State = "recording";    //if the track is either empty or playing, record
              else if (Tr2State == "recording") Tr2State = "overdubbing", firstRecording = false;    //if the track is recording, overdub
              else if (Tr2State == "overdubbing") Tr2State = "playing", selectedTrack = 3;    //if the track is overdubbing, play and select the next track
              else if (Tr2State == "muted") Tr2State = "playing";    //if the track is muted, play it
            }else if (selectedTrack == 2 && !firstRecording){    //if the 1st track is selected and it's NOT the first time recording
              if (Tr2State == "playing" || Tr2State == "empty"){    //if it's playing or empty, overdub
                Tr2State = "overdubbing";
                //if another track is being recorded, stop recording and play it:
                if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
                if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
                if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
              }else if (Tr2State == "recording") Tr2State = "playing", selectedTrack = 3;    //if the track is recording, play it and select the next track
              else if (Tr2State == "overdubbing") Tr2State = "playing", selectedTrack = 3;    //if the track is overdubbing, play and select the next track
              else if (Tr2State == "muted") Tr2State = "recording";    //if the track is muted, record
            }
            else if (selectedTrack == 3 && firstRecording){    //if the 3rd track is selected and it's the first time recording
              if (Tr3State == "playing" || Tr3State == "empty") Tr3State = "recording";    //if the track is either empty or playing, record
              else if (Tr3State == "recording") Tr3State = "overdubbing", firstRecording = false;    //if the track is recording, overdub
              else if (Tr3State == "overdubbing") Tr3State = "playing", selectedTrack = 4;    //if the track is overdubbing, play and select the next track
              else if (Tr3State == "muted") Tr3State = "playing";    //if the track is muted, play it
            }else if (selectedTrack == 3 && !firstRecording){    //if the 1st track is selected and it's NOT the first time recording
              if (Tr3State == "playing" || Tr3State == "empty"){    //if it's playing or empty, overdub
                Tr3State = "overdubbing";
                //if another track is being recorded, stop recording and play it:
                if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
                if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
                if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
              }else if (Tr3State == "recording") Tr3State = "playing", selectedTrack = 4;    //if the track is recording, play it and select the next track
              else if (Tr3State == "overdubbing") Tr3State = "playing", selectedTrack = 4;    //if the track is overdubbing, play and select the next track
              else if (Tr3State == "muted") Tr3State = "recording";    //if the track is muted, record
            }
            else if (selectedTrack == 4 && firstRecording){    //if the 4th track is selected and it's the first time recording
              if (Tr4State == "playing" || Tr4State == "empty") Tr4State = "recording";    //if the track is either empty or playing, record
              else if (Tr4State == "recording") Tr4State = "overdubbing";    //if the track is recording, overdub
              else if (Tr4State == "overdubbing") Tr4State = "playing", selectedTrack = 1, firstRecording = false;    //if the track is overdubbing, play and select the next track
              else if (Tr4State == "muted") Tr4State = "playing";    //if the track is muted, play it
            }else if (selectedTrack == 4 && !firstRecording){    //if the 1st track is selected and it's NOT the first time recording
              if (Tr4State == "playing" || Tr4State == "empty"){    //if it's playing or empty, overdub
                Tr4State = "overdubbing";
                //if another track is being recorded, stop recording and play it:
                if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
                if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
                if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
              }else if (Tr4State == "recording") Tr4State = "playing", selectedTrack = 1;    //if the track is recording, play it and select the first track
              else if (Tr4State == "overdubbing") Tr4State = "playing", selectedTrack = 1;    //if the track is overdubbing, play and select the first track
              else if (Tr4State == "muted") Tr4State = "recording";    //if the track is muted, record
            }
          //}
          time = millis();
          doublePressClear = false;
        }
      }
      if (playMode == HIGH){    //if the current mode is Play Mode
        if (buttonpress == "Stop"){    //if the Stop button is pressed
          sendNote(0x2A);    //send the note
          //if(readMIDI(29)) {
            //mute the tracks if they aren't empty
            if (Tr1State != "empty") Tr1State = "muted";
            if (Tr2State != "empty") Tr2State = "muted";
            if (Tr3State != "empty") Tr3State = "muted";
            if (Tr4State != "empty") Tr4State = "muted";
            Tr1PlayedWRecPlay = false;
            Tr2PlayedWRecPlay = false;
            Tr3PlayedWRecPlay = false;
            Tr4PlayedWRecPlay = false;
            stopMode = true;    //make the stopMode variable and mode true
            previousPlay = false;
            doublePressClear = false;
         // }
          time = millis();
        }
        if (buttonpress == "RecPlay"){    //if the Rec/Play button is pressed
          sendNote(0x31);    //send the note
          //if(readMIDI(29)) {
            if (stopMode) ringPosition = 0;    //if we are in stop mode, make the ring spin from the position 0
            if (!stopModeUsed){    //if stop Mode hasn't been used, just play every track that isn't empty: 
              if (Tr1State != "empty") Tr1State = "playing";
              else if (Tr1State == "empty") Tr1PlayedWRecPlay = true;
              if (Tr2State != "empty") Tr2State = "playing";
              else if (Tr2State == "empty") Tr2PlayedWRecPlay = true;
              if (Tr3State != "empty") Tr3State = "playing";
              else if (Tr3State == "empty") Tr3PlayedWRecPlay = true;
              if (Tr4State != "empty") Tr4State = "playing";
              else if (Tr4State == "empty") Tr4PlayedWRecPlay = true;
            }else if (!previousPlay && stopModeUsed){    //if stop mode has been used and the previous pressed button isn't RecPlay
              if (!Tr1PressedInStop && !Tr2PressedInStop && !Tr3PressedInStop && !Tr4PressedInStop){    //if none of the tracks has been pressed in stop, play every track that isn't empty: 
                if (Tr1State != "empty") Tr1State = "playing";
                if (Tr2State != "empty") Tr2State = "playing";
                if (Tr3State != "empty") Tr3State = "playing";
                if (Tr4State != "empty") Tr4State = "playing";
              }else {    //if a track has been pressed and it isn't empty, play it
                if (Tr1PressedInStop) Tr1State = "playing";
                if (Tr2PressedInStop) Tr2State = "playing";
                if (Tr3PressedInStop) Tr3State = "playing";
                if (Tr4PressedInStop) Tr4State = "playing";
              }
              previousPlay = true;
            }else if (previousPlay && stopModeUsed){    //if the previous pressed button is RecPlay, play every track that is not empty:
              if (Tr1State != "empty") Tr1State = "playing";
              else if (Tr1State == "empty") Tr1PlayedWRecPlay = true;
              if (Tr2State != "empty") Tr2State = "playing";
              else if (Tr2State == "empty") Tr2PlayedWRecPlay = true;
              if (Tr3State != "empty") Tr3State = "playing";
              else if (Tr3State == "empty") Tr3PlayedWRecPlay = true;
              if (Tr4State != "empty") Tr4State = "playing";
              else if (Tr4State == "empty") Tr4PlayedWRecPlay = true;
              previousPlay = false;
            }
            stopMode = false;
            doublePressClear = false;
          //}
          time = millis();
        }
        if (stopMode){    //if we are in stop mode
          if (buttonpress == "Track1" && Tr1State != "empty"){    //if the track 1 button is pressed and it isn't empty
            sendNote(0x2C);    //send the note
            //if(readMIDI(29)) {
              Tr1PressedInStop = !Tr1PressedInStop;    //toggle between the track being pressed in stop mode and not
              selectedTrack = 1;    //select the track
              doublePressClear = false;
            //}
            time = millis();
          }
          if (buttonpress == "Track2" && Tr2State != "empty"){    //if the track 2 button is pressed and it isn't empty
            sendNote(0x2D);    //send the note
           //if(readMIDI(29)) {
              Tr2PressedInStop = !Tr2PressedInStop;    //toggle between the track being pressed in stop mode and not
              selectedTrack = 2;    //select the track
              doublePressClear = false;
            //}
            time = millis();
          }
          if (buttonpress == "Track3" && Tr3State != "empty"){    //if the track 3 button is pressed and it isn't empty
            sendNote(0x2E);    //send the note
           // if(readMIDI(29)) {
              Tr3PressedInStop = !Tr3PressedInStop;    //toggle between the track being pressed in stop mode and not
              selectedTrack = 3;    //select the track
              doublePressClear = false;
           // }
            time = millis();
          }
          if (buttonpress == "Track4" && Tr4State != "empty"){    //if the track 4 button is pressed and it isn't empty
            sendNote(0x2F);    //send the note
           // if(readMIDI(29)) {
              Tr4PressedInStop = !Tr4PressedInStop;    //toggle between the track being pressed in stop mode and not
              selectedTrack = 4;    //select the track
              doublePressClear = false;
           // }
            time = millis();
          }
        }else if (!stopMode){    //if we aren't in stop mode
          if (buttonpress == "Track1" && Tr1State != "empty"){    //if the track 1 button is pressed and it isn't empty
            sendNote(0x2C);    //send the note
            //if(readMIDI(29)) {
              //toggle between playing and muted when it was pressed in stop mode:
              if (Tr1PressedInStop && Tr1State == "playing") previousPlay = true, Tr1State = "muted";
              else if (Tr1PressedInStop && Tr1State == "muted") previousPlay = true, Tr1State = "playing";
              else if (!Tr1PressedInStop){    //if it wasn't pressed in stop
                if (Tr1State == "playing") Tr1State = "muted";    //and it is playing, mute it
                else if (Tr1State == "muted"){    //else if it is muted, play it
                //reset everything in stopMode:
                  Tr1State = "playing"; 
                  stopModeUsed = false;
                  Tr1PressedInStop = false;
                  Tr2PressedInStop = false;
                  Tr3PressedInStop = false;
                  Tr4PressedInStop = false;
                }
              }
              selectedTrack = 1;    //select the track
              doublePressClear = false;
            //}
            time = millis();
          }
          if (buttonpress == "Track2" && !firstRecording && Tr2State != "empty"){    //if the track 2 button is pressed and it isn't empty
            sendNote(0x2D);    //send the note
            //if(readMIDI(29)) {
              //toggle between playing and muted when it was pressed in stop mode:
              if (Tr2PressedInStop && Tr2State == "playing") previousPlay = true, Tr2State = "muted";
              else if (Tr2PressedInStop && Tr2State == "muted") previousPlay = true, Tr2State = "playing";
              else if (!Tr2PressedInStop){    //if it wasn't pressed in stop
                if (Tr2State == "playing") Tr2State = "muted";    //and it is playing, mute it
                else if (Tr2State == "muted"){    //else if it is muted, play it
                //reset everything in stopMode:
                  Tr2State = "playing"; 
                  stopModeUsed = false;
                  Tr1PressedInStop = false;
                  Tr2PressedInStop = false;
                  Tr3PressedInStop = false;
                  Tr4PressedInStop = false;
                }
              }
              selectedTrack = 2;    //select the track
              doublePressClear = false;
            //}
            time = millis();
          }
          if (buttonpress == "Track3" && !firstRecording && Tr3State != "empty"){    //if the track 3 button is pressed and it isn't empty
            sendNote(0x2E);    //send the note
            //if(readMIDI(29)) {
              //toggle between playing and muted when it was pressed in stop mode:
              if (Tr3PressedInStop && Tr3State == "playing") previousPlay = true, Tr3State = "muted";
              else if (Tr3PressedInStop && Tr3State == "muted") previousPlay = true,Tr3State = "playing" ;
              else if (!Tr3PressedInStop){    //if it wasn't pressed in stop
                if (Tr3State == "playing") Tr3State = "muted";    //and it is playing, mute it
                else if (Tr3State == "muted"){     //else if it is muted, play it
                //reset everything in stopMode:
                  Tr3State = "playing";
                  stopModeUsed = false;
                  Tr1PressedInStop = false;
                  Tr2PressedInStop = false;
                  Tr3PressedInStop = false;
                  Tr4PressedInStop = false;
                }
              }
              selectedTrack = 3;    //select the track
              doublePressClear = false;
            //}
            time = millis();
          }
          if (buttonpress == "Track4" && !firstRecording && Tr4State != "empty"){    //if the track 4 button is pressed and it isn't empty
            sendNote(0x2F);    //send the note
            //if(readMIDI(29)) {
              //toggle between playing and muted when it was pressed in stop mode:
              if (Tr4PressedInStop && Tr4State == "playing") previousPlay = true, Tr4State = "muted";
              else if (Tr4PressedInStop && Tr4State == "muted") previousPlay = true, Tr4State = "playing";
              else if (!Tr4PressedInStop){    //if it wasn't pressed in stop
                if (Tr4State == "playing") Tr4State = "muted";    //and it is playing, mute it
                else if (Tr4State == "muted"){    //else if it is muted, play it
                //reset everything in stopMode:
                  Tr4State = "playing";
                  stopModeUsed = false;
                  Tr1PressedInStop = false;
                  Tr2PressedInStop = false;
                  Tr3PressedInStop = false;
                  Tr4PressedInStop = false;
                }
              }
              selectedTrack = 4;    //select the track
              doublePressClear = false;
            //}
            time = millis();
          }
        }
      }
      //here ends the button presses
    }
    
    lastbuttonpress = buttonpress;
    setLEDs();
  }
  if (playMode || !playMode){
    if (millis() - time2 > doublePressClearTime){    //debounce
      if (buttonpress == "Clear" && !doublePressClear){
        if (canClearTrack){    //if we can clear the selected track, do it
          sendNote(0x1E);
          //if(readMIDI(29)) {
            if(selectedTrack == 1 && Tr1State == "playing") Tr1State = "empty", Tr1PlayedWRecPlay = false;
            if(selectedTrack == 2 && Tr2State == "playing") Tr2State = "empty", Tr2PlayedWRecPlay = false;
            if(selectedTrack == 3 && Tr3State == "playing") Tr3State = "empty", Tr3PlayedWRecPlay = false;
            if(selectedTrack == 4 && Tr4State == "playing") Tr4State = "empty", Tr4PlayedWRecPlay = false;
            doublePressClear = true;    //activate the function that resets everything if we press the Clear button again
            previousPlay = false;
          //}
        } else if (!canClearTrack) reset();
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
    if(!firstRecording){
      if(selectedTrack == 1 && Tr1State == "playing") LEDs[clearLED] = CRGB(0,0,255);
      else if(selectedTrack == 1 && Tr1State != "playing" && Tr1State != "empty") LEDs[clearLED] = CRGB(255,0,0);
      if(selectedTrack == 2 && Tr2State == "playing") LEDs[clearLED] = CRGB(0,0,255);
      else if(selectedTrack == 2 && Tr2State != "playing" && Tr2State != "empty") LEDs[clearLED] = CRGB(255,0,0);
      if(selectedTrack == 3 && Tr3State == "playing") LEDs[clearLED] = CRGB(0,0,255);
      else if(selectedTrack == 3 && Tr3State != "playing" && Tr3State != "empty") LEDs[clearLED] = CRGB(255,0,0);
      if(selectedTrack == 4 && Tr4State == "playing") LEDs[clearLED] = CRGB(0,0,255);
      else if(selectedTrack == 4 && Tr4State != "playing" && Tr4State != "empty") LEDs[clearLED] = CRGB(255,0,0);
    }else LEDs[clearLED] = CRGB(255,0,0);
  }
  if (digitalRead(clearButton) == HIGH) LEDs[clearLED] = CRGB(0,0,0);    //turn OFF the Clear LED when the button is NOT pressed
  
  if (digitalRead(X2Button) == LOW && !firstRecording) LEDs[X2LED] = CRGB(0,0,255);    //turn ON the X2 LED when the button is pressed and the pedal has been used
  else if (digitalRead(X2Button) == LOW && firstRecording) LEDs[X2LED] = CRGB(255,0,0);
  if (digitalRead(X2Button) == HIGH) LEDs[X2LED] = CRGB(0,0,0);    //turn OFF the X2 LED when the button is NOT pressed

  if (Tr1State == "recording" || Tr1State == "overdubbing") LEDs[Tr1LED] = CRGB(255,0,0);    //when track 1 is recording or overdubing, turn on it's LED, colour red
  else if (Tr1State == "playing") LEDs[Tr1LED] = CRGB(0,255,0);    //when track 1 is playing, turn on it's LED, colour green
  else if (Tr1State == "muted" || Tr1State == "empty") LEDs[Tr1LED] = CRGB(0,0,0);    //when track 1 is muted or empty, turn off it's LED
  
  if (Tr2State == "recording" || Tr2State == "overdubbing") LEDs[Tr2LED] = CRGB(255,0,0);    //when track 2 is recording or overdubing, turn on it's LED, colour red
  else if (Tr2State == "playing") LEDs[Tr2LED] = CRGB(0,255,0);    //when track 2 is playing, turn on it's LED, colour green
  else if (Tr2State == "muted" || Tr2State == "empty") LEDs[Tr2LED] = CRGB(0,0,0);    //when track 2 is muted or empty, turn off it's LED
  
  if (Tr3State == "recording" || Tr3State == "overdubbing") LEDs[Tr3LED] = CRGB(255,0,0);    //when track 3 is recording or overdubing, turn on it's LED, colour red
  else if (Tr3State == "playing") LEDs[Tr3LED] = CRGB(0,255,0);    //when track 3 is playing, turn on it's LED, colour green
  else if (Tr3State == "muted" || Tr3State == "empty") LEDs[Tr3LED] = CRGB(0,0,0);    //when track 3 is muted or empty, turn off it's LED
  
  if (Tr4State == "recording" || Tr4State == "overdubbing") LEDs[Tr4LED] = CRGB(255,0,0);    //when track 4 is recording or overdubing, turn on it's LED, colour red
  else if (Tr4State == "playing") LEDs[Tr4LED] = CRGB(0,255,0);    //when track 4 is playing, turn on it's LED, colour green
  else if (Tr4State == "muted" || Tr4State == "empty") LEDs[Tr4LED] = CRGB(0,0,0);    //when track 4 is muted or empty, turn off it's LED

  //the setHue variable sets the LED Ring's colour. 0 is red, 60 is yellow(ish), 96 is green.
  if ((Tr1State == "recording" || Tr2State == "recording" || Tr3State == "recording" || Tr4State == "recording") && firstRecording) setHue = 0;    //sets the led ring to red (recording)
  else if ((Tr1State == "recording" || Tr2State == "recording" || Tr3State == "recording" || Tr4State == "recording") && !firstRecording) setHue = 60;    //sets the led ring to yellow (overdubbing)
  else if (Tr1State == "overdubbing" || Tr2State == "overdubbing" || Tr3State == "overdubbing" || Tr4State == "overdubbing") setHue = 60;    //sets the led ring to yellow (overdubbing)
  else if ((Tr1State == "muted" || Tr1State == "empty") && (Tr2State == "muted" || Tr2State == "empty") && (Tr3State == "muted" || Tr3State == "empty") && (Tr4State == "muted" || Tr4State == "empty") && !firstRecording && !Tr1PlayedWRecPlay && !Tr2PlayedWRecPlay && !Tr3PlayedWRecPlay && !Tr4PlayedWRecPlay) stopMode = true;    //stop the spinning ring animation when all tracks are muted
  else setHue = 96;    //sets the led ring to green (playing)
  
  FastLED.show();    //updates the led states
}

void reset(){    //function to reset the pedal
  sendNote(0x1F);    //sends note that resets Mobius
  //empties the tracks:
  Tr1State = "empty";
  Tr2State = "empty";
  Tr3State = "empty";
  Tr4State = "empty";
  //resets the stopMode:
  Tr1PressedInStop = false, Tr1PlayedWRecPlay = false;
  Tr2PressedInStop = false, Tr2PlayedWRecPlay = false;
  Tr3PressedInStop = false, Tr3PlayedWRecPlay = false;
  Tr4PressedInStop = false, Tr4PlayedWRecPlay = false;
  stopModeUsed = false;
  stopMode = false;
  selectedTrack = 1;    //selects the 1rst track
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
  for (int i = 0; i < 3; i++){
    fill_solid(LEDs, qtyLEDs, CRGB::Red);
    FastLED.show();
    delay(150);
    FastLED.clear();
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
  if (selectedTrack == 1) Serial.write(volTr1);    //This sends the velocity corresponding to the selected track, which goes from 0 to 127.
  if (selectedTrack == 2) Serial.write(volTr2);
  if (selectedTrack == 3) Serial.write(volTr3);
  if (selectedTrack == 4) Serial.write(volTr4);
}
/*
bool readMIDI(byte value){
  do {
    if (Serial.available()){
      byte commandByte = Serial.read();
      byte valueByte = Serial.read();
      byte velocityByte = Serial.read();
      if (commandByte == 144 && valueByte == value && velocityByte > 0) readMIDIreturn = true;
      else readMIDIreturn = false;
    } else readMIDIreturn = false;
  }
  while (Serial.available() >= 2);
  if (readMIDIreturn == true) return true;
  else return false;
}
*/
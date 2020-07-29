#include <FastLED.h>  //library for controlling the LEDs

#define pinLEDs 2
#define qtyLEDs 19
#define typeOfLEDs WS2812B  //type of LEDs i'm using
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
#define bankButton 12
#define potPin A0
#define nonRingLEDs 7

unsigned int potVal = 0;         //current pot value
unsigned int lastPotVal = 0;     //previous pot value

boolean playMode = LOW;   // LOW = Rec mode, HIGH = Play Mode
long time = 0;         // the last time the output pin was toggled
long debounceTime = 300;   // the debounce time, increase if the output flickers (was 150)
#define doublePressClearTime 500

String Tr1State = "empty";    // Track state, to determine how and which LED(s) must be on or off.
String Tr2State = "empty";
String Tr3State = "empty";
String Tr4State = "empty";
boolean Tr1PressedInStop = false; //when a track is pressed-in-stop, it gets focuslocked
boolean Tr2PressedInStop = false;
boolean Tr3PressedInStop = false;
boolean Tr4PressedInStop = false;
boolean previousPlay = false;

int selectedTrack = 1;        /* Selected track, to record on it without having to press the individual track.
Plus, if you record using the RecPlayButton, you will also overdub, unlike pressing the track button, 
which would only record and then play (no overdub). */
int firstTrack;
boolean firstRecording = true;
boolean stopMode = false;
boolean stopModeUsed = false;
boolean canClearTrack = false;
boolean canChangeMode = false;
boolean doublePressClear = false;

String buttonpress = "";      // To store the button pressed at the time
String lastbuttonpress = "";  // To store the previous button pressed
#define midichannel 0x90

#define modeLED 12
#define Tr1LED 13
#define Tr2LED 14
#define Tr3LED 15
#define Tr4LED 16
#define clearLED 17
#define bankLED 18

byte ringPosition = 0; // Array position of LED that will be lit (0 to 15)
int setHue = 96; //yellow is 35/50 = 45, red is 0, green is 96
#define ringSpeed  54.6875//46.875 62.5

void setup() {
  //Serial.begin(38400);      //to use LoopMIDI and Hairless MIDI Serial
  Serial.begin(31250);      //to use only the Arduino UNO (Atmega16u2). You must upload another bootloader to use it as a native MIDI-USB device.
  FastLED.addLeds<typeOfLEDs, pinLEDs, GRB>(LEDs, qtyLEDs); //declare LEDs
  pinMode(recPlayButton, INPUT_PULLUP); //declare buttons as Input INPUT_PULLUP
  pinMode(stopButton, INPUT_PULLUP);
  pinMode(undoButton, INPUT_PULLUP);
  pinMode(modeButton, INPUT_PULLUP);
  pinMode(track1Button, INPUT_PULLUP);
  pinMode(track2Button, INPUT_PULLUP);
  pinMode(track3Button, INPUT_PULLUP);
  pinMode(track4Button, INPUT_PULLUP);
  pinMode(clearButton, INPUT_PULLUP);
  pinMode(bankButton, INPUT_PULLUP);
  pinMode(potPin, INPUT); //declaring potentiometer as INPUT
  sendNote(0x1F); //resets the pedal
  sendNote(0x2B); //gets into Record Mode
  startLEDs(); //animation for the LEDs when the pedal is reset
  setLEDs(); //matches the LEDs states with the tracks states
}

void loop() {
  if ((Tr1State != "empty" || Tr2State != "empty" || Tr3State != "empty" || Tr4State != "empty") && !stopMode) ringLEDs(); //the led ring spins when the pedal is recording, overdubbing or playing.
  if ((Tr1State == "recording" || Tr1State == "overdubbing") || (Tr2State == "recording" || Tr2State == "overdubbing") || (Tr3State == "recording" || Tr3State == "overdubbing") || (Tr4State == "recording" || Tr4State == "overdubbing") && firstRecording) canClearTrack = false, canChangeMode = false; 
  else canClearTrack = true, canChangeMode = true; //you can only clear the selected track and change between modes when you are not recording for the first time.
  if (Tr1State == "empty" && Tr2State == "empty" && Tr3State == "empty" && Tr4State == "empty" && !firstRecording) reset(); //reset the pedal when every track is empty but it has been used (for example, when the only track playing is Tr1 and you clear it, it resets the whole pedal)
  
  if (Tr1State == "recording" && firstRecording) firstTrack = 1;
  if (Tr2State == "recording" && firstRecording) firstTrack = 2;
  if (Tr3State == "recording" && firstRecording) firstTrack = 3;
  if (Tr4State == "recording" && firstRecording) firstTrack = 4;
  if (!firstRecording) firstTrack = 0;

  buttonpress = "released"; //release the variable buttonpress every time the void loops

  // set the variable buttonpress to a recognizable name when a button is pressed:
  if (digitalRead(recPlayButton) == LOW) buttonpress = "RecPlay";
  if (digitalRead(stopButton) == LOW) buttonpress = "Stop";
  if (digitalRead(undoButton) == LOW) buttonpress = "Undo";
  if (digitalRead(track1Button) == LOW) buttonpress = "Track1";
  if (digitalRead(track2Button) == LOW) buttonpress = "Track2";
  if (digitalRead(track3Button) == LOW) buttonpress = "Track3";
  if (digitalRead(track4Button) == LOW) buttonpress = "Track4";
  if (digitalRead(clearButton) == LOW) buttonpress = "Clear";
  if (digitalRead(bankButton) == LOW) buttonpress = "Bank";
  if (digitalRead(modeButton) == LOW) buttonpress	= "Mode";

  if (millis() - time > debounceTime){ //debounce
    if (buttonpress != lastbuttonpress && buttonpress != "released"){  //in any mode
      if (buttonpress == "Mode" && canChangeMode){ //if modeButton is pressed and mode can be changed
        if (playMode == LOW){ //if the current mode is Record Mode
          playMode = HIGH;    //enter play mode
          sendNote(0x29);     //send a note so that the pedal knows we've change modes
        }else{
          playMode = LOW;            //entering rec mode
          sendNote(0x2B);
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
      }
      if (buttonpress == "Clear" && !doublePressClear){
        if (canClearTrack){ //if we can clear the selected track, do it
          sendNote(0x1E);
          if(selectedTrack == 1) Tr1State = "empty";
          if(selectedTrack == 2) Tr2State = "empty";
          if(selectedTrack == 3) Tr3State = "empty";
          if(selectedTrack == 4) Tr4State = "empty";
          doublePressClear = true;  //activate the function that resets everything if we press the Clear button again
          previousPlay = false;
        }
        time = millis();
      }else if (buttonpress == "Clear" && doublePressClear) reset(); //reset everything when Clear is pressed twice
      if (buttonpress == "Clear" && firstRecording){  //if nothing is recorded yet, just send the potentiometer (volume) value
        lastPotVal = potVal;
        if (potVal/8 > 122) potVal = 1023; //clear the signal a bit
        if (potVal/8 < 6) potVal = 0;
        //send the value:
        Serial.write(176);  //176 = CC Command
        Serial.write(1); //1 = Which Control
        Serial.write(potVal/8); // Value read from potentiometer
        time = millis();
      }
      if (buttonpress == "Undo"){ //if the Undo button is pressed
        sendNote(0x22); //send a note that undoes the last thing it did
        if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing"; //play a track if it is being recorded
        if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
        if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
        if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
        doublePressClear = false;
        previousPlay = false;
      }
      if (buttonpress == "Bank") sendNote(0x20), doublePressClear = false, previousPlay = false; //not ready yet, only doubles the record time
      time = millis();
    }
    if (playMode == LOW){           //if the current mode is Record Mode
      if (buttonpress == "Stop" && canClearTrack){  //if Stop is pressed
        sendNote(0x28); //send the note
        //mute the selected track:
        if (selectedTrack == 1) Tr1State = "muted";
        if (selectedTrack == 2) Tr2State = "muted";
        if (selectedTrack == 3) Tr3State = "muted";
        if (selectedTrack == 4) Tr4State = "muted";
        doublePressClear = false;
      } 
      if (buttonpress == "Track1" && (firstTrack == 1 || firstTrack == 0)){ //if the track 1 button is pressed
        sendNote(0x23); //send the note
        if (Tr1State == "playing" || Tr1State == "empty" || Tr1State == "muted"){ //if it's playing, empty or muted, record it
          Tr1State = "recording";
          //if another track is being recorded, stop and play it:
          if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing"; 
          if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
          if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
        }else if (Tr1State == "overdubbing") Tr1State = "playing", firstRecording = false;  //if the track is overdubbing, play it
        else if (Tr1State == "recording") Tr1State = "playing", firstRecording = false; //if the track is recording, play it
        selectedTrack = 1;  //select the track 1
        doublePressClear = false;
        time = millis();
      }
      if (buttonpress == "Track2" && (firstTrack == 2 || firstTrack == 0)){ //if the track 2 button is pressed
        sendNote(0x24); //send the note
        if (Tr2State == "playing" || Tr2State == "empty" || Tr2State == "muted"){ //if it's playing, empty or muted, record it
          Tr2State = "recording";
          //if another track is being recorded, stop and play it:
          if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
          if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
          if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
        }else if (Tr2State == "overdubbing") Tr2State = "playing", firstRecording = false;  //if the track is overdubbing, play it
        else if (Tr2State == "recording") Tr2State = "playing", firstRecording = false; //if the track is recording, play it
        selectedTrack = 2;  //select the track 2
        doublePressClear = false;
        time = millis();
      }
      if (buttonpress == "Track3" && (firstTrack == 3 || firstTrack == 0)){ //if the track 3 button is pressed
        sendNote(0x25); //send the note
        if (Tr3State == "playing" || Tr3State == "empty" || Tr3State == "muted"){ //if it's playing, empty or muted, record it
          Tr3State = "recording";
          //if another track is being recorded, stop and play it:
          if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
          if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
          if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
        }else if (Tr3State == "overdubbing") Tr3State = "playing", firstRecording = false;  //if the track is overdubbing, play it
        else if (Tr3State == "recording") Tr3State = "playing", firstRecording = false; //if the track is recording, play it
        selectedTrack = 3;  //select the track 3
        doublePressClear = false;
        time = millis();
      }
      if (buttonpress == "Track4" && (firstTrack == 4 || firstTrack == 0)){ //if the track 4 button is pressed
        sendNote(0x26); //send the note
        if (Tr4State == "playing" || Tr4State == "empty" || Tr4State == "muted"){ //if it's playing, empty or muted, record it
          Tr4State = "recording";
          //if another track is being recorded, stop and play it:
          if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
          if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
          if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
        }else if (Tr4State == "overdubbing") Tr4State = "playing", firstRecording = false;  //if the track is overdubbing, play it
        else if (Tr4State == "recording") Tr4State = "playing", firstRecording = false; //if the track is recording, play it
        selectedTrack = 4;  //select the track 4
        doublePressClear = false;
        time = millis();
      }
      if (buttonpress == "RecPlay"){  //if the Rec/Play button is pressed
        sendNote(0x27);   //send the note
        if (selectedTrack == 1 && firstRecording){    //if the 1st track is selected and it's the first time recording
          if (Tr1State == "playing" || Tr1State == "empty") Tr1State = "recording";   //if the track is either empty or playing, record
          else if (Tr1State == "recording") Tr1State = "overdubbing";   //if the track is recording, overdub
          else if (Tr1State == "overdubbing") Tr1State = "playing", firstRecording = false, selectedTrack = 2;  //if the track is overdubbing, play and select the next track
          else if (Tr1State == "muted") Tr1State = "playing";   //if the track is muted, play it
        }else if (selectedTrack == 1 && !firstRecording){   //if the 1st track is selected and it's NOT the first time recording
          if (Tr1State == "playing" || Tr1State == "empty"){    //if it's playing or empty, overdub
            Tr1State = "overdubbing";
            //if another track is being recorded, stop recording and play it:
            if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
            if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
            if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
          }else if (Tr1State == "recording") Tr1State = "playing", selectedTrack = 2;  //if the track is recording, play it and select the next track
          else if (Tr1State == "overdubbing")  Tr1State = "playing", selectedTrack = 2;   //if the track is overdubbing, play and select the next track
          else if (Tr1State == "muted") Tr1State = "recording";   //if the track is muted, record
        }
        if (selectedTrack == 2 && firstRecording){    //if the 2nd track is selected and it's the first time recording
          if (Tr2State == "playing" || Tr2State == "empty") Tr2State = "recording";   //if the track is either empty or playing, record
          else if (Tr2State == "recording") Tr2State = "overdubbing";   //if the track is recording, overdub
          else if (Tr2State == "overdubbing") Tr2State = "playing", firstRecording = false, selectedTrack = 3;  //if the track is overdubbing, play and select the next track
          else if (Tr2State == "muted") Tr2State = "playing";   //if the track is muted, play it
        }else if (selectedTrack == 2 && !firstRecording){   //if the 1st track is selected and it's NOT the first time recording
          if (Tr2State == "playing" || Tr2State == "empty"){    //if it's playing or empty, overdub
            Tr2State = "overdubbing";
            //if another track is being recorded, stop recording and play it:
            if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
            if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
            if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
          }else if (Tr2State == "recording") Tr2State = "playing", selectedTrack = 3;  //if the track is recording, play it and select the next track
          else if (Tr2State == "overdubbing") Tr2State = "playing", selectedTrack = 3;   //if the track is overdubbing, play and select the next track
          else if (Tr2State == "muted") Tr2State = "recording";   //if the track is muted, record
        }
        if (selectedTrack == 3 && firstRecording){    //if the 3rd track is selected and it's the first time recording
          if (Tr3State == "playing" || Tr3State == "empty") Tr3State = "recording";   //if the track is either empty or playing, record
          else if (Tr3State == "recording") Tr3State = "overdubbing";   //if the track is recording, overdub
          else if (Tr3State == "overdubbing") Tr3State = "playing", firstRecording = false, selectedTrack = 4;  //if the track is overdubbing, play and select the next track
          else if (Tr3State == "muted") Tr3State = "playing";   //if the track is muted, play it
        }else if (selectedTrack == 3 && !firstRecording){   //if the 1st track is selected and it's NOT the first time recording
          if (Tr3State == "playing" || Tr3State == "empty"){    //if it's playing or empty, overdub
            Tr3State = "overdubbing";
            //if another track is being recorded, stop recording and play it:
            if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
            if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
            if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
          }else if (Tr3State == "recording") Tr3State = "playing", selectedTrack = 4;  //if the track is recording, play it and select the next track
          else if (Tr3State == "overdubbing") Tr3State = "playing", selectedTrack = 4;   //if the track is overdubbing, play and select the next track
          else if (Tr3State == "muted") Tr3State = "recording";   //if the track is muted, record
        }
        if (selectedTrack == 4 && firstRecording){  //if the 4th track is selected and it's the first time recording
          if (Tr4State == "playing" || Tr4State == "empty") Tr4State = "recording";   //if the track is either empty or playing, record
          else if (Tr4State == "recording") Tr4State = "overdubbing";   //if the track is recording, overdub
          else if (Tr4State == "overdubbing") Tr4State = "playing", selectedTrack = 1, firstRecording = false;  //if the track is overdubbing, play and select the next track
          else if (Tr4State == "muted") Tr4State = "playing";   //if the track is muted, play it
        }else if (selectedTrack == 4 && !firstRecording){   //if the 1st track is selected and it's NOT the first time recording
          if (Tr4State == "playing" || Tr4State == "empty"){    //if it's playing or empty, overdub
            Tr4State = "overdubbing";
            //if another track is being recorded, stop recording and play it:
            if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
            if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
            if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
          }else if (Tr4State == "recording") Tr4State = "playing", selectedTrack = 1;  //if the track is recording, play it and select the first track
          else if (Tr4State == "overdubbing") Tr4State = "playing", selectedTrack = 1;   //if the track is overdubbing, play and select the first track
          else if (Tr4State == "muted") Tr4State = "recording";   //if the track is muted, record
        }
        time = millis();
        doublePressClear = false;
      }
    }
    if (playMode == HIGH){      //if the current mode is Play Mode
      if (buttonpress == "Stop"){   //if the Stop button is pressed
        sendNote(0x2A);   //send the note
        //mute the tracks if they aren't empty
        if (Tr1State != "empty") Tr1State = "muted";
        if (Tr2State != "empty") Tr2State = "muted";
        if (Tr3State != "empty") Tr3State = "muted";
        if (Tr4State != "empty") Tr4State = "muted";
        stopMode = true;    //make the stopMode variable and mode true
        previousPlay = false;
        doublePressClear = false;
        time = millis();
      }
      if (buttonpress == "RecPlay"){    //if the Rec/Play button is pressed
        sendNote(0x31);   //send the note
        if (stopMode) ringPosition = 0;   //if we are in stop mode, make the ring spin from the position 0
        if (!stopModeUsed){   //if stop Mode hasn't been used, just play every track that isn't empty: 
          if (Tr1State != "empty") Tr1State = "playing";
          if (Tr2State != "empty") Tr2State = "playing";
          if (Tr3State != "empty") Tr3State = "playing";
          if (Tr4State != "empty") Tr4State = "playing";
        }else if (!previousPlay && stopModeUsed){   //if stop mode has been used and the previous pressed button isn't RecPlay
          if (!Tr1PressedInStop && !Tr2PressedInStop && !Tr3PressedInStop && !Tr4PressedInStop){   //if none of the tracks has been pressed in stop, play every track that isn't empty: 
            if (Tr1State != "empty") Tr1State = "playing";
            if (Tr2State != "empty") Tr2State = "playing";
            if (Tr3State != "empty") Tr3State = "playing";
            if (Tr4State != "empty") Tr4State = "playing";
          }else {   //if a track has been pressed and it isn't empty, play it
            if (Tr1PressedInStop) Tr1State = "playing";
            if (Tr2PressedInStop) Tr2State = "playing";
            if (Tr3PressedInStop) Tr3State = "playing";
            if (Tr4PressedInStop) Tr4State = "playing";
          }
          previousPlay = true;
        }else if (previousPlay && stopModeUsed){    //if the previous pressed button is RecPlay, play every track that is not empty:
          if (Tr1State != "empty") Tr1State = "playing";
          if (Tr2State != "empty") Tr2State = "playing";
          if (Tr3State != "empty") Tr3State = "playing";
          if (Tr4State != "empty") Tr4State = "playing";
          previousPlay = false;
        }
        stopMode = false;
        doublePressClear = false;
        time = millis();
      }
      if (stopMode){    //if we are in stop mode
        if (buttonpress == "Track1" && Tr1State != "empty"){   //if the track 1 button is pressed and it isn't empty
          sendNote(0x2C);   //send the note
          Tr1PressedInStop = !Tr1PressedInStop;   //toggle between the track being pressed in stop mode and not
          stopModeUsed = true;    //set the stopModeUsed variable to true
          selectedTrack = 1;    //select the track
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "Track2" && Tr2State != "empty"){   //if the track 2 button is pressed and it isn't empty
          sendNote(0x2D);   //send the note
          Tr2PressedInStop = !Tr2PressedInStop;   //toggle between the track being pressed in stop mode and not
          stopModeUsed = true;    //set the stopModeUsed variable to true
          selectedTrack = 2;    //select the track
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "Track3" && Tr3State != "empty"){   //if the track 3 button is pressed and it isn't empty
          sendNote(0x2E);   //send the note
          Tr3PressedInStop = !Tr3PressedInStop;   //toggle between the track being pressed in stop mode and not
          stopModeUsed = true;    //set the stopModeUsed variable to true
          selectedTrack = 3;    //select the track
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "Track4" && Tr4State != "empty"){   //if the track 4 button is pressed and it isn't empty
          sendNote(0x2F);   //send the note
          Tr4PressedInStop = !Tr4PressedInStop;   //toggle between the track being pressed in stop mode and not
          stopModeUsed = true;    //set the stopModeUsed variable to true
          selectedTrack = 4;    //select the track
          doublePressClear = false;
          time = millis();
        }
      }else if (!stopMode){   //if we aren't in stop mode
        if (buttonpress == "Track1" && Tr1State != "empty"){    //if the track 1 button is pressed and it isn't empty
          sendNote(0x2C);   //send the note
          //toggle between playing and muted when it was pressed in stop mode:
          if (Tr1PressedInStop && Tr1State == "playing") previousPlay = true, Tr1State = "muted";
          else if (Tr1PressedInStop && Tr1State == "muted") previousPlay = true, Tr1State = "playing";
          else if (!Tr1PressedInStop){   //if it wasn't pressed in stop
            if (Tr1State == "playing") Tr1State = "muted";    //and it is playing, mute it
            else if (Tr1State == "muted") Tr1State = "playing";   //else if it is muted, play it
            //reset everything in stopMode:
            stopModeUsed = false;
            Tr1PressedInStop = false;
            Tr2PressedInStop = false;
            Tr3PressedInStop = false;
            Tr4PressedInStop = false;
          }
          selectedTrack = 1;    //select the track
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "Track2" && !firstRecording && Tr2State != "empty"){    //if the track 2 button is pressed and it isn't empty
          sendNote(0x2D);   //send the note
          //toggle between playing and muted when it was pressed in stop mode:
          if (Tr2PressedInStop && Tr2State == "playing") previousPlay = true, Tr2State = "muted";
          else if (Tr2PressedInStop && Tr2State == "muted") previousPlay = true, Tr2State = "playing";
          else if (!Tr2PressedInStop){   //if it wasn't pressed in stop
            if (Tr2State == "playing") Tr2State = "muted";    //and it is playing, mute it
            else if (Tr2State == "muted") Tr2State = "playing";   //else if it is muted, play it
            //reset everything in stopMode:
            stopModeUsed = false;
            Tr1PressedInStop = false;
            Tr2PressedInStop = false;
            Tr3PressedInStop = false;
            Tr4PressedInStop = false;
          }
          selectedTrack = 2;    //select the track
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "Track3" && !firstRecording && Tr3State != "empty"){    //if the track 3 button is pressed and it isn't empty
          sendNote(0x2E);   //send the note
          //toggle between playing and muted when it was pressed in stop mode:
          if (Tr3PressedInStop && Tr3State == "playing") previousPlay = true, Tr3State = "muted";
          else if (Tr3PressedInStop && Tr3State == "muted") previousPlay = true,Tr3State = "playing" ;
          else if (!Tr3PressedInStop){   //if it wasn't pressed in stop
            if (Tr3State == "playing") Tr3State = "muted";    //and it is playing, mute it
            else if (Tr3State == "muted") Tr3State = "playing";   //else if it is muted, play it
            //reset everything in stopMode:
            stopModeUsed = false;
            Tr1PressedInStop = false;
            Tr2PressedInStop = false;
            Tr3PressedInStop = false;
            Tr4PressedInStop = false;
          }
          selectedTrack = 3;    //select the track
          doublePressClear = false;
          time = millis();
        }
        if (buttonpress == "Track4" && !firstRecording && Tr4State != "empty"){    //if the track 4 button is pressed and it isn't empty
          sendNote(0x2F);   //send the note
          //toggle between playing and muted when it was pressed in stop mode:
          if (Tr4PressedInStop && Tr4State == "playing") previousPlay = true, Tr4State = "muted";
          else if (Tr4PressedInStop && Tr4State == "muted") previousPlay = true, Tr4State = "playing";
          else if (!Tr4PressedInStop){   //if it wasn't pressed in stop
            if (Tr4State == "playing") Tr4State = "muted";    //and it is playing, mute it
            else if (Tr4State == "muted") Tr4State = "playing";   //else if it is muted, play it
            //reset everything in stopMode:
            stopModeUsed = false;
            Tr1PressedInStop = false;
            Tr2PressedInStop = false;
            Tr3PressedInStop = false;
            Tr4PressedInStop = false;
          }
          selectedTrack = 4;    //select the track
          doublePressClear = false;
          time = millis();
        }
      }
    }
    //here ends the button presses
    lastbuttonpress = buttonpress;
    setLEDs();
  }
  potVal = analogRead(potPin); // Divide by 8 to get range of 0-127 for midi
  unsigned int diffPot = abs(lastPotVal - potVal);
  
  if (diffPot > 30){// If the value does not = the last value the following command is made. This is because the pot has been turned. Otherwise the pot remains the same and no midi message is output.
    lastPotVal = potVal;    //so that the arduino doesn't send the potentiometer value unless it changes
    //cleanup of the potentiometer value:
    if (potVal/8 > 123) potVal = 1023;
    if (potVal < 40) potVal = 0;
    Serial.write(176);  //176 = CC Command
    Serial.write(1); //2 = Which Control
    Serial.write(potVal/8); // Value read from potentiometer
  }
}
void sendNote(int pitch){
  Serial.write(midichannel);    //sends the notes in the selected channel
  Serial.write(pitch);    //sends note
  Serial.write(0x45);  //medium velocity = Note ON
}
void setLEDs(){
  //mode LED:
  if (playMode == LOW) LEDs[modeLED] = CRGB(255,0,0);   //when we are in Record mode, the LED is red
  if (playMode == HIGH) LEDs[modeLED] = CRGB(0,255,0);   //when we are in Play mode, the LED is green

  if (digitalRead(clearButton) == LOW && !firstRecording) LEDs[clearLED] = CRGB(0,0,255);   //turn ON the Clear LED when the button is pressed and the pedal has been used
  if (digitalRead(clearButton) == HIGH) LEDs[clearLED] = CRGB(0,0,0);   //turn OFF the Clear LED when the button is NOT pressed
  
  if (Tr1State == "recording" || Tr1State == "overdubbing") LEDs[Tr1LED] = CRGB(255,0,0);   //when track 1 is recording or overdubing, turn on it's LED, colour red
  else if (Tr1State == "playing") LEDs[Tr1LED] = CRGB(0,255,0);   //when track 1 is playing, turn on it's LED, colour green
  else if (Tr1State == "muted" || Tr1State == "empty") LEDs[Tr1LED] = CRGB(0,0,0);   //when track 1 is muted or empty, turn off it's LED
  
  if (Tr2State == "recording" || Tr2State == "overdubbing") LEDs[Tr2LED] = CRGB(255,0,0);   //when track 2 is recording or overdubing, turn on it's LED, colour red
  else if (Tr2State == "playing") LEDs[Tr2LED] = CRGB(0,255,0);   //when track 2 is playing, turn on it's LED, colour green
  else if (Tr2State == "muted" || Tr2State == "empty") LEDs[Tr2LED] = CRGB(0,0,0);  //when track 2 is muted or empty, turn off it's LED
  
  if (Tr3State == "recording" || Tr3State == "overdubbing") LEDs[Tr3LED] = CRGB(255,0,0);   //when track 3 is recording or overdubing, turn on it's LED, colour red
  else if (Tr3State == "playing") LEDs[Tr3LED] = CRGB(0,255,0);   //when track 3 is playing, turn on it's LED, colour green
  else if (Tr3State == "muted" || Tr3State == "empty") LEDs[Tr3LED] = CRGB(0,0,0);  //when track 3 is muted or empty, turn off it's LED
  
  if (Tr4State == "recording" || Tr4State == "overdubbing") LEDs[Tr4LED] = CRGB(255,0,0);   //when track 4 is recording or overdubing, turn on it's LED, colour red
  else if (Tr4State == "playing") LEDs[Tr4LED] = CRGB(0,255,0);   //when track 4 is playing, turn on it's LED, colour green
  else if (Tr4State == "muted" || Tr4State == "empty") LEDs[Tr4LED] = CRGB(0,0,0);  //when track 4 is muted or empty, turn off it's LED

  //the setHue variable sets the LED Ring's colour. 0 is red, 60 is yellow(ish), 96 is green.
  if ((Tr1State == "recording" || Tr2State == "recording" || Tr3State == "recording" || Tr4State == "recording") && firstRecording) setHue = 0;   //sets the led ring to red (recording)
  else if ((Tr1State == "recording" || Tr2State == "recording" || Tr3State == "recording" || Tr4State == "recording") && !firstRecording) setHue = 60;    //sets the led ring to yellow (overdubbing)
  else if (Tr1State == "overdubbing" || Tr2State == "overdubbing" || Tr3State == "overdubbing" || Tr4State == "overdubbing") setHue = 60;   //sets the led ring to yellow (overdubbing)
  else if ((Tr1State == "muted" || Tr1State == "empty") && (Tr2State == "muted" || Tr2State == "empty") && (Tr3State == "muted" || Tr3State == "empty") && (Tr4State == "muted" || Tr4State == "empty") && !firstRecording) stopMode = true;  //stop the spinning ring animation when all tracks are muted
  else setHue = 96;   //sets the led ring to green (playing)
  
  FastLED.show();   //updates the led states
}
void reset(){   //function to reset the pedal
  sendNote(0x1F);   //sends note that resets Mobius
  //empties the tracks:
  Tr1State = "empty";
  Tr2State = "empty";
  Tr3State = "empty";
  Tr4State = "empty";
  //resets the stopMode:
  Tr1PressedInStop = false;
  Tr2PressedInStop = false;
  Tr3PressedInStop = false;
  Tr4PressedInStop = false;
  stopModeUsed = false;
  stopMode = false;
  selectedTrack = 1;    //selects the 1rst track
  firstRecording = true;    //sets the variable firstRecording to true
  previousPlay = false;   
  sendNote(0x2B);   //sends the Record Mode note
  playMode = LOW;           //into record mode
  delay(50);
  //sends potentiometer value:
  Serial.write(176);  //176 = CC Command
  Serial.write(1); //2 = Which Control
  Serial.write(potVal/8); // Value read from potentiometer
  ringPosition = 0;   //resets the ring position
  startLEDs();    //animation to show the pedal has been reset
  doublePressClear = false;
  time = millis();
}
void startLEDs(){   //animation to show the pedal has been reset
  for (int i = 0; i < 3; i++){    //turns on and off every led 3 times in red
    LEDs[0] = CRGB(255,0,0);
    LEDs[1] = CRGB(255,0,0);
    LEDs[2] = CRGB(255,0,0);
    LEDs[3] = CRGB(255,0,0);
    LEDs[4] = CRGB(255,0,0);
    LEDs[5] = CRGB(255,0,0);
    LEDs[6] = CRGB(255,0,0);
    LEDs[7] = CRGB(255,0,0);
    LEDs[8] = CRGB(255,0,0);
    LEDs[9] = CRGB(255,0,0);
    LEDs[10] = CRGB(255,0,0);
    LEDs[11] = CRGB(255,0,0);
    LEDs[12] = CRGB(255,0,0);
    LEDs[13] = CRGB(255,0,0);
    LEDs[14] = CRGB(255,0,0);
    LEDs[15] = CRGB(255,0,0);
    LEDs[16] = CRGB(255,0,0);
    LEDs[17] = CRGB(255,0,0);
    LEDs[18] = CRGB(255,0,0);
    FastLED.show();
    delay(150);
    LEDs[0] = CRGB(0,0,0);
    LEDs[1] = CRGB(0,0,0);
    LEDs[2] = CRGB(0,0,0);
    LEDs[3] = CRGB(0,0,0);
    LEDs[4] = CRGB(0,0,0);
    LEDs[5] = CRGB(0,0,0);
    LEDs[6] = CRGB(0,0,0);
    LEDs[7] = CRGB(0,0,0);
    LEDs[8] = CRGB(0,0,0);
    LEDs[9] = CRGB(0,0,0);
    LEDs[10] = CRGB(0,0,0);
    LEDs[11] = CRGB(0,0,0);
    LEDs[12] = CRGB(0,0,0);
    LEDs[13] = CRGB(0,0,0);
    LEDs[14] = CRGB(0,0,0);
    LEDs[15] = CRGB(0,0,0);
    LEDs[16] = CRGB(0,0,0);
    LEDs[17] = CRGB(0,0,0);
    LEDs[18] = CRGB(0,0,0);
    FastLED.show();
    delay(150);
  }
  setHue = 96;    //sets the led ring colour to green
}
void ringLEDs(){    //function that makes the spinning animation for the led ring
  FastLED.setBrightness(255);
  EVERY_N_MILLISECONDS(ringSpeed){ //this gets an entire spin in 3/4 of a second (0.75s) Half a second would be 31.25, a second would be 62.5
    fadeToBlackBy(LEDs, qtyLEDs - nonRingLEDs, 70);  // Dims the LEDs by 64/256 (1/4) and thus sets the trail's length. Also set limit to the ring LEDs quantity and exclude the mode and track ones (qtyLEDs-7)
    LEDs[ringPosition] = CHSV(setHue, 255, 255); // Sets the LED's hue according to the potentiometer rotation    
    ringPosition++; // Shifts all LEDs one step in the currently active direction    
    if (ringPosition == qtyLEDs - nonRingLEDs) ringPosition = 0; // If one end is reached, reset the position to loop around
    FastLED.show(); // Finally, display all LED's data (illuminate the LED ring)
  }
}
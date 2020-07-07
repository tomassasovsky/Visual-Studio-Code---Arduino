#include <FastLED.h>

#define pinLEDs 2
#define qtyLEDs 21
#define typeOfLEDs WS2812B
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
#define nonRingLEDs 5

unsigned int potVal = 0;         //current pot value
unsigned int lastPotVal = 0;     //previous pot value

boolean playMode = LOW;   // LOW = Rec mode, HIGH = Play Mode
long time = 0;         // the last time the output pin was toggled
long debounceTime = 300;   // the debounce time, increase if the output flickers (was 150)
#define longPressClearTime 500

String Tr1State = "empty";    // Track state, to determine how and which LED(s) must be on or off.
String Tr2State = "empty";
String Tr3State = "empty";
String Tr4State = "empty";
boolean Tr1PressedInStop = false;
boolean Tr2PressedInStop = false;
boolean Tr3PressedInStop = false;
boolean Tr4PressedInStop = false;

int selectedTrack = 1;        /* Selected track, to record on it without having to press the individual track.
Plus, if you record using the RecPlayButton, you will also overdub, unlike pressing the track button, 
which would only record and then play (no overdubb). */
boolean firstRecording = true;
boolean stopMode = false;
boolean canClearTrack = false;
boolean canChangeMode = false;
boolean longPressClear = false;
boolean previousPlay = false;


String buttonpress = "";      // To store the button pressed at the time
String lastbuttonpress = "";  // To store the previous button pressed
#define midichannel 0x90

#define modeLED 16
#define Tr1LED 17
#define Tr2LED 18
#define Tr3LED 19
#define Tr4LED 20

byte ringPosition = 0; // Array position of LED that will be lit (0 to 15)
int setHue = 96; //yellow is 35/50 = 45, red is 0, green is 96
#define ringSpeed 46.875

void setup() {
  Serial.begin(38400);      //to use LoopMIDI and Hairless MIDI Serial
  //Serial.begin(31250);      //to use only the Arduino UNO (Atmega16u2). You must upload another bootloader to use it as a native MIDI-USB device.
  FastLED.addLeds<typeOfLEDs, pinLEDs, GRB>(LEDs, qtyLEDs);
  pinMode(recPlayButton, INPUT_PULLUP);
  pinMode(stopButton, INPUT_PULLUP);
  pinMode(undoButton, INPUT_PULLUP);
  pinMode(modeButton, INPUT_PULLUP);
  pinMode(track1Button, INPUT_PULLUP);
  pinMode(track2Button, INPUT_PULLUP);
  pinMode(track3Button, INPUT_PULLUP);
  pinMode(track4Button, INPUT_PULLUP);
  pinMode(clearButton, INPUT_PULLUP);
  pinMode(bankButton, INPUT_PULLUP);
  pinMode(potPin, INPUT);
  sendNote(0x1F);
  Serial.write(midichannel);
  Serial.write(0x2B);
  Serial.write(0x45);       //medium velocity = Note ON
  startLEDs();
  setLEDs();
}

void loop() {
  if (Tr1State != "empty" || Tr2State != "empty" || Tr3State != "empty" || Tr4State != "empty") ringLEDs();
  if ((Tr1State == "recording" || Tr1State == "overdubbing") || (Tr2State == "recording" || Tr2State == "overdubbing") || (Tr3State == "recording" || Tr3State == "overdubbing") || (Tr4State == "recording" || Tr4State == "overdubbing") || firstRecording) canClearTrack = false, canChangeMode = false;
  else canClearTrack = true, canChangeMode = true;
  if (Tr1State == "empty" && Tr2State == "empty" && Tr3State == "empty" && Tr4State == "empty" && !firstRecording){
    
    sendNote(0x1F);
    selectedTrack = 1;
    firstRecording = true;
    Serial.write(midichannel);
    Serial.write(0x2B);
    Serial.write(0x45);       //medium velocity = Note ON
    delay(50);
    lastPotVal = potVal;
    if (potVal/8 > 123) potVal = 1023;
    if (potVal < 20) potVal = 0;
    Serial.write(176);  //176 = CC Command
    Serial.write(1); //2 = Which Control
    Serial.write(potVal/8); // Value read from potentiometer
    playMode = LOW;           //into record mode
    startLEDs();
  }
  
  buttonpress = "released";
  if (digitalRead(recPlayButton) == LOW) buttonpress = "RecPlay", longPressClear = false;
  if (digitalRead(stopButton) == LOW) buttonpress = "Stop", longPressClear = false;
  if (digitalRead(undoButton) == LOW) buttonpress = "Undo", longPressClear = false;
  if (digitalRead(track1Button) == LOW) buttonpress = "Track1", longPressClear = false;
  if (digitalRead(track2Button) == LOW) buttonpress = "Track2", longPressClear = false;
  if (digitalRead(track3Button) == LOW) buttonpress = "Track3", longPressClear = false;
  if (digitalRead(track4Button) == LOW) buttonpress = "Track4", longPressClear = false;
  if (digitalRead(clearButton) == LOW) buttonpress = "Clear";
  if (digitalRead(bankButton) == LOW) buttonpress = "Bank", longPressClear = false;
  if (digitalRead(modeButton) == LOW) buttonpress	= "Mode", longPressClear = false;

  if (millis() - time > debounceTime){
    if (buttonpress != lastbuttonpress && buttonpress != "released"){  //any mode
      if (buttonpress == "Mode" && canChangeMode){
        if (playMode == LOW){
          playMode = HIGH;           //entering play mode
          Serial.write(midichannel);
          Serial.write(0x29);
          Serial.write(0x45);        //medium velocity = Note ON
          if (Tr1State != "muted" && Tr1State != "empty") Tr1State = "playing";
          if (Tr2State != "muted" && Tr2State != "empty") Tr2State = "playing";
          if (Tr3State != "muted" && Tr3State != "empty") Tr3State = "playing";
          if (Tr4State != "muted" && Tr4State != "empty") Tr4State = "playing";
        }else{
          playMode = LOW;            //entering rec mode
          Serial.write(midichannel);
          Serial.write(0x2B);
          Serial.write(0x45);        //medium velocity = Note ON
          if (Tr1State != "muted" && Tr1State != "empty") Tr1State = "playing";
          if (Tr2State != "muted" && Tr2State != "empty") Tr2State = "playing";
          if (Tr3State != "muted" && Tr3State != "empty") Tr3State = "playing";
          if (Tr4State != "muted" && Tr4State != "empty") Tr4State = "playing";
        }
      }
      if (buttonpress == "Clear" && !longPressClear){
        if (canClearTrack){
          sendNote(0x1E);
          if(selectedTrack == 1 && Tr1State != "empty") Tr1State = "empty";
          if(selectedTrack == 2 && Tr2State != "empty") Tr2State = "empty";
          if(selectedTrack == 3 && Tr3State != "empty") Tr3State = "empty";
          if(selectedTrack == 4 && Tr4State != "empty") Tr4State = "empty";
          longPressClear = true;
        }
        time = millis();
      }else if (buttonpress == "Clear" && longPressClear){
        sendNote(0x1F);
        Tr1State = "empty";
        Tr2State = "empty";
        Tr3State = "empty";
        Tr4State = "empty";
        selectedTrack = 1;
        firstRecording = true;
        Serial.write(midichannel);
        Serial.write(0x2B);
        Serial.write(0x45);       //medium velocity = Note ON
        delay(50);
        Serial.write(176);  //176 = CC Command
        Serial.write(1); //2 = Which Control
        Serial.write(potVal/8); // Value read from potentiometer
        playMode = LOW;           //into record mode
        startLEDs();
        longPressClear = false;
        time = millis();
      }
      
      if (buttonpress == "Undo"){
        if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
        if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
        if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
        if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
        sendNote(0x22);
      }
      time = millis();
    }
    if (playMode == LOW){           //if the current mode is "Record Mode"
      if (buttonpress == "Track1"){
        if (Tr1State == "playing" || Tr1State == "empty" || Tr1State == "muted"){
          Tr1State = "recording";
          if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
          if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
          if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
        }else if (Tr1State == "overdubbing") Tr1State = "playing", firstRecording = false;
        else if (Tr1State == "recording") Tr1State = "playing", firstRecording = false;
        sendNote(0x23);
        selectedTrack = 1;
        time = millis();
      }
      if (buttonpress == "Track2"){
        if (Tr2State == "playing" || Tr2State == "empty" || Tr2State == "muted"){
          Tr2State = "recording";
          if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
          if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
          if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
        }else if (Tr2State == "overdubbing") Tr2State = "playing", firstRecording = false;
        else if (Tr2State == "recording") Tr2State = "playing", firstRecording = false;
        sendNote(0x24);
        selectedTrack = 2;
        time = millis();
      }
      if (buttonpress == "Track3"){
        if (Tr3State == "playing" || Tr3State == "empty" || Tr3State == "muted"){
          Tr3State = "recording";
          if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
          if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
          if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
        }else if (Tr3State == "overdubbing") Tr3State = "playing", firstRecording = false;
        else if (Tr3State == "recording") Tr3State = "playing", firstRecording = false;
        sendNote(0x25);
        selectedTrack = 3;
        time = millis();
      }
      if (buttonpress == "Track4"){
        if (Tr4State == "playing" || Tr4State == "empty" || Tr4State == "muted"){
          Tr4State = "recording";
          if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
          if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
          if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
        }else if (Tr4State == "overdubbing") Tr4State = "playing", firstRecording = false;
        else if (Tr4State == "recording") Tr4State = "playing", firstRecording = false;
        sendNote(0x26);
        selectedTrack = 4;
        time = millis();
      }
      if (buttonpress == "RecPlay"){
        if (selectedTrack == 1 && firstRecording){
          if (Tr1State == "playing" || Tr1State == "empty") Tr1State = "recording";
          else if (Tr1State == "recording") Tr1State = "overdubbing";
          else if (Tr1State == "overdubbing") Tr1State = "playing", buttonpress = "released", firstRecording = false, selectedTrack = 2;
          else if (Tr1State == "muted") Tr1State = "playing";
          sendNote(0x27);
        }else if (selectedTrack == 1 && !firstRecording){
          if (Tr1State == "playing" || Tr1State == "empty"){
            Tr1State = "overdubbing", buttonpress = "released";
            if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
            if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
            if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
          }else if (Tr1State == "recording") Tr1State = "playing", selectedTrack = 2;
          else if (Tr1State == "overdubbing") buttonpress = "released", Tr1State = "playing", selectedTrack = 2;
          else if (Tr1State == "muted") Tr1State = "playing";
          sendNote(0x27);
        }else if (selectedTrack == 2 && firstRecording){
          if (Tr2State == "playing" || Tr2State == "empty") Tr2State = "recording", buttonpress = "released";
          else if (Tr2State == "recording") Tr2State = "overdubbing";
          else if (Tr2State == "overdubbing") buttonpress = "released", Tr2State = "playing", firstRecording = false, selectedTrack = 3;
          else if (Tr2State == "muted") Tr2State = "playing";
          sendNote(0x27);
        }else if (selectedTrack == 2 && !firstRecording){
          if (Tr2State == "playing" || Tr2State == "empty"){
            Tr2State = "overdubbing", buttonpress = "released";
            if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
            if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
            if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
          }else if (Tr2State == "recording") Tr2State = "playing", selectedTrack = 3;
          else if (Tr2State == "overdubbing") buttonpress = "released", Tr2State = "playing", selectedTrack = 3;
          else if (Tr2State == "muted") Tr2State = "playing";
          sendNote(0x27);
        }else if (selectedTrack == 3 && firstRecording){
          if (Tr3State == "playing" || Tr3State == "empty") Tr3State = "recording", buttonpress = "released";
          else if (Tr3State == "recording") Tr3State = "overdubbing";
          else if (Tr3State == "overdubbing") buttonpress = "released", Tr3State = "playing", firstRecording = false, selectedTrack = 4;
          else if (Tr3State == "muted") Tr3State = "playing";
          sendNote(0x27);
        }else if (selectedTrack == 3 && !firstRecording){
          if (Tr3State == "playing" || Tr3State == "empty"){
            Tr3State = "overdubbing", buttonpress = "released";
            if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
            if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
            if (Tr4State == "recording" || Tr4State == "overdubbing") Tr4State = "playing";
          }else if (Tr3State == "recording") Tr3State = "playing", selectedTrack = 4;
          else if (Tr3State == "overdubbing") buttonpress = "released", Tr3State = "playing", selectedTrack = 4;
          else if (Tr3State == "muted") Tr3State = "playing";
          sendNote(0x27);
        }else if (selectedTrack == 4 && firstRecording){
          if (Tr4State == "playing" || Tr4State == "empty") Tr4State = "recording", buttonpress = "released";
          else if (Tr4State == "recording") Tr4State = "overdubbing";
          else if (Tr4State == "overdubbing") buttonpress = "released", Tr4State = "playing", selectedTrack = 1, firstRecording = false;
          else if (Tr4State == "muted") Tr4State = "playing";
          sendNote(0x27);
        }else if (selectedTrack == 4 && !firstRecording){
          if (Tr4State == "playing" || Tr4State == "empty"){
            Tr4State = "overdubbing", buttonpress = "released";
            if (Tr1State == "recording" || Tr1State == "overdubbing") Tr1State = "playing";
            if (Tr2State == "recording" || Tr2State == "overdubbing") Tr2State = "playing";
            if (Tr3State == "recording" || Tr3State == "overdubbing") Tr3State = "playing";
          }else if (Tr4State == "recording") Tr4State = "playing", selectedTrack = 1;
          else if (Tr4State == "overdubbing") buttonpress = "released", Tr4State = "playing", selectedTrack = 1;
          else if (Tr4State == "muted") Tr4State = "playing";
          sendNote(0x27);
        }
        time = millis();
      }
    }
    if (playMode == HIGH){      //if the current mode is Play Mode
      if (buttonpress == "Stop"){
        sendNote(0x2A);
        if (Tr1State != "empty") Tr1State = "muted";
        if (Tr2State != "empty") Tr2State = "muted";
        if (Tr3State != "empty") Tr3State = "muted";
        if (Tr4State != "empty") Tr4State = "muted";
        if (stopMode == true) stopMode = false;
        else if (stopMode == false) stopMode = true;
        previousPlay = false;
        time = millis();
      }
      if (stopMode){
        if (buttonpress == "Track1" && !firstRecording && Tr1State != "empty"){
          if (Tr1State == "muted") Tr1State = "playing", Tr1PressedInStop = true;
          else Tr1State = "muted", Tr1PressedInStop = false;
          selectedTrack = 1;
          sendNote(0x2C);
          time = millis();
        }
        if (buttonpress == "Track2" && !firstRecording && Tr2State != "empty"){
          if (Tr2State == "muted") Tr2State = "playing", Tr2PressedInStop = true;
          else Tr2State = "muted", Tr2PressedInStop = false;
          selectedTrack = 2;
          sendNote(0x2D);
          time = millis();
        }
        if (buttonpress == "Track3" && !firstRecording && Tr3State != "empty"){
          if (Tr3State == "muted") Tr3State = "playing", Tr3PressedInStop = true;
          else Tr3State = "muted", Tr3PressedInStop = false;
          selectedTrack = 3;
          sendNote(0x2E);
          time = millis();
        }
        if (buttonpress == "Track4" && !firstRecording && Tr4State != "empty"){
          if (Tr4State == "muted") Tr4State = "playing", Tr4PressedInStop = true;
          else Tr4State = "muted", Tr4PressedInStop = false;
          selectedTrack = 4;
          sendNote(0x2F);
          time = millis();
        }
      }else if (!stopMode){
        if (buttonpress == "Track1" && !firstRecording && Tr1State != "empty"){
          if (Tr1State == "muted") Tr1State = "playing";
          else Tr1State = "muted";
          selectedTrack = 1;
          sendNote(0x2C);
          time = millis();
        }
        if (buttonpress == "Track2" && !firstRecording && Tr2State != "empty"){
          if (Tr2State == "muted") Tr2State = "playing";
          else Tr2State = "muted";
          selectedTrack = 2;
          sendNote(0x2D);
          time = millis();
        }
        if (buttonpress == "Track3" && !firstRecording && Tr3State != "empty"){
          if (Tr3State == "muted") Tr3State = "playing";
          else Tr3State = "muted";
          selectedTrack = 3;
          sendNote(0x2E);
          time = millis();
        }
        if (buttonpress == "Track4" && !firstRecording && Tr4State != "empty"){
          if (Tr4State == "muted") Tr4State = "playing";
          else Tr4State = "muted";
          selectedTrack = 4;
          sendNote(0x2F);
          time = millis();
        }
      }
      if (buttonpress == "RecPlay"){
        if (!previousPlay){
          if (Tr1PressedInStop == true && Tr2PressedInStop == true && Tr3PressedInStop == true && Tr4PressedInStop == true) { //1111
            Tr1State = "playing";
            Tr2State = "playing";
            Tr3State = "playing";
            Tr4State = "playing";
          }else if (Tr1PressedInStop == false && Tr2PressedInStop == false && Tr3PressedInStop == false && Tr4PressedInStop == true) { //0001
            Tr4State = "playing";
          }else if (Tr1PressedInStop == false && Tr2PressedInStop == false && Tr3PressedInStop == true && Tr4PressedInStop == false) { //0010
            Tr3State = "playing";
          }else if (Tr1PressedInStop == false && Tr2PressedInStop == false && Tr3PressedInStop == true && Tr4PressedInStop == true) { //0011
            Tr3State = "playing";
            Tr4State = "playing";
          }else if (Tr1PressedInStop == false && Tr2PressedInStop == true && Tr3PressedInStop == false && Tr4PressedInStop == false) { //0100
            Tr2State = "playing";
          }else if (Tr1PressedInStop == false && Tr2PressedInStop == true && Tr3PressedInStop == false && Tr4PressedInStop == true) { //0101
            Tr2State = "playing";
            Tr4State = "playing";
          }else if (Tr1PressedInStop == false && Tr2PressedInStop == true && Tr3PressedInStop == true && Tr4PressedInStop == false) { //0110
            Tr2State = "playing";
            Tr3State = "playing";
          }else if (Tr1PressedInStop == false && Tr2PressedInStop == true && Tr3PressedInStop == true && Tr4PressedInStop == true) { //0111
            Tr2State = "playing";
            Tr3State = "playing";
            Tr4State = "playing";
          }else if (Tr1PressedInStop == true && Tr2PressedInStop == false && Tr3PressedInStop == false && Tr4PressedInStop == false) { //1000
            Tr1State = "playing";          
          }else if (Tr1PressedInStop == true && Tr2PressedInStop == false && Tr3PressedInStop == false && Tr4PressedInStop == true) { //1001
            Tr1State = "playing";
            Tr4State = "playing";
          }else if (Tr1PressedInStop == true && Tr2PressedInStop == false && Tr3PressedInStop == true && Tr4PressedInStop == false) { //1010
            Tr1State = "playing";
            Tr3State = "playing";
          }else if (Tr1PressedInStop == true && Tr2PressedInStop == false && Tr3PressedInStop == true && Tr4PressedInStop == true) { //1011
            Tr1State = "playing";
            Tr3State = "playing";
            Tr4State = "playing";
          }else if (Tr1PressedInStop == true && Tr2PressedInStop == true && Tr3PressedInStop == false && Tr4PressedInStop == false) { //1100
            Tr1State = "playing";
            Tr2State = "playing";
          }else if (Tr1PressedInStop == true && Tr2PressedInStop == true && Tr3PressedInStop == false && Tr4PressedInStop == true) { //1101
            Tr1State = "playing";
            Tr2State = "playing";
            Tr4State = "playing";
          }else if (Tr1PressedInStop == true && Tr2PressedInStop == true && Tr3PressedInStop == true && Tr4PressedInStop == false) { //1110
            Tr1State = "playing";
            Tr2State = "playing";
            Tr3State = "playing";
          }
          previousPlay = true;
        }else if (previousPlay){
          if (Tr1State != "empty") Tr1State = "playing";
          if (Tr2State != "empty") Tr2State = "playing";
          if (Tr3State != "empty") Tr3State = "playing";
          if (Tr4State != "empty") Tr4State = "playing";
          previousPlay = false;
        }
        sendNote(0x31);
        time = millis();
      }
      if (buttonpress == "Track1" && !firstRecording && Tr1State != "empty"){
        if (Tr1State == "muted") Tr1State = "playing";
        else Tr1State = "muted";
        selectedTrack = 1;
        sendNote(0x2C);
        time = millis();
      }
      if (buttonpress == "Track2" && !firstRecording && Tr2State != "empty"){
        if (Tr2State == "muted") Tr2State = "playing";
        else Tr2State = "muted";
        selectedTrack = 2;
        sendNote(0x2D);
        time = millis();
      }
      if (buttonpress == "Track3" && !firstRecording && Tr3State != "empty"){
        if (Tr3State == "muted") Tr3State = "playing";
        else Tr3State = "muted";
        selectedTrack = 3;
        sendNote(0x2E);
        time = millis();
      }
      if (buttonpress == "Track4" && !firstRecording && Tr4State != "empty"){
        if (Tr4State == "muted") Tr4State = "playing";
        else Tr4State = "muted";
        selectedTrack = 4;
        sendNote(0x2F);
        time = millis();
      }
    }
    //here ends the button presses
    lastbuttonpress = buttonpress;
    setLEDs();
  }
  potVal = analogRead(potPin); // Divide by 8 to get range of 0-127 for midi
  unsigned int diffPot = abs(lastPotVal - potVal);
  
  if (diffPot > 30){// If the value does not = the last value the following command is made. This is because the pot has been turned. Otherwise the pot remains the same and no midi message is output.
    lastPotVal = potVal;
    if (potVal/8 > 123) potVal = 1023;
    if (potVal < 20) potVal = 0;
    Serial.write(176);  //176 = CC Command
    Serial.write(1); //2 = Which Control
    Serial.write(potVal/8); // Value read from potentiometer
  }
}
void sendNote(int pitch){
  Serial.write(midichannel);
  Serial.write(pitch);
  Serial.write(0x45);  //medium velocity = Note ON
}
void setLEDs(){
  if (playMode == LOW) LEDs[modeLED] = CRGB(255,0,0);
  if (playMode == HIGH) LEDs[modeLED] = CRGB(0,255,0);
  
  if (Tr1State == "recording" || Tr1State == "overdubbing") LEDs[Tr1LED] = CRGB(255,0,0);
  else if (Tr1State == "playing") LEDs[Tr1LED] = CRGB(0,255,0);
  else if (Tr1State == "muted" || Tr1State == "empty") LEDs[Tr1LED] = CRGB(0,0,0);
  
  if (Tr2State == "playing") LEDs[Tr2LED] = CRGB(0,255,0);
  else if (Tr2State == "recording" || Tr2State == "overdubbing") LEDs[Tr2LED] = CRGB(255,0,0);
  else if (Tr2State == "muted" || Tr2State == "empty") LEDs[Tr2LED] = CRGB(0,0,0);
  
  if (Tr3State == "playing") LEDs[Tr3LED] = CRGB(0,255,0);
  else if (Tr3State == "recording" || Tr3State == "overdubbing") LEDs[Tr3LED] = CRGB(255,0,0);
  else if (Tr3State == "muted" || Tr3State == "empty") LEDs[Tr3LED] = CRGB(0,0,0);
  
  if (Tr4State == "playing") LEDs[Tr4LED] = CRGB(0,255,0);
  else if (Tr4State == "recording" || Tr4State == "overdubbing") LEDs[Tr4LED] = CRGB(255,0,0);
  else if (Tr4State == "muted" || Tr4State == "empty") LEDs[Tr4LED] = CRGB(0,0,0);

  if ((Tr1State == "recording" || Tr2State == "recording" || Tr3State == "recording" || Tr4State == "recording") && firstRecording) setHue = 0;
  else if ((Tr1State == "recording" || Tr2State == "recording" || Tr3State == "recording" || Tr4State == "recording") && !firstRecording) setHue = 45;
  else if (Tr1State == "overdubbing" || Tr2State == "overdubbing" || Tr3State == "overdubbing" || Tr4State == "overdubbing") setHue = 45;
  else if (Tr1State == "muted" && Tr2State == "muted" && Tr3State == "muted" && Tr4State == "muted" || ((Tr1State == "muted" || Tr1State == "empty") && (Tr2State == "muted" || Tr2State == "empty") && (Tr3State == "muted" || Tr3State == "empty") && (Tr4State == "muted" || Tr4State == "empty") && !firstRecording)) setHue = 160;  //set the led ring to blue when all tracks are muted
  else setHue = 96;
  
  FastLED.show();
}
void startLEDs(){
  for(int i = 0; i < 3; i++){
    for(int j = 0; j < qtyLEDs; j++){
      LEDs[j] = CRGB(255, 0, 0);
      FastLED.show();
    }
    delay(150);
    for(int j = 0; j < qtyLEDs; j++){
      LEDs[j] = CRGB(0, 0, 0);
      FastLED.show();
    }
    delay(150);
  }
  setHue = 96;
}
void ringLEDs(){
  FastLED.setBrightness(255);
  EVERY_N_MILLISECONDS(ringSpeed){ //this gets an entire spin in 3/4 of a second (0.75s) Half a second would be 31.25, a second would be 62.5
    fadeToBlackBy(LEDs, qtyLEDs - nonRingLEDs, 50);  // Dims the LEDs by 64/256 (1/4) and thus sets the trail's length. Also set limit to the ring LEDs quantity and exclude the mode and track ones (qtyLEDs-5)
    LEDs[ringPosition] = CHSV(setHue, 255, 255); // Sets the LED's hue according to the potentiometer rotation    
    ringPosition++; // Shifts all LEDs one step in the currently active direction    
    if (ringPosition == qtyLEDs - nonRingLEDs) ringPosition = 0; // If one end is reached, reset the position to loop around
    FastLED.show(); // Finally, display all LED's data (= illuminate the LED ring)
  }
}
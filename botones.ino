#define boton1 3

String buttonpress = "released";    //To store the button pressed at the time
String lastbuttonpress = "";    //To store the previous button pressed

long time = 0;    //the last time the output pin was toggled
long debounceTime = 150;    //the debounce time, increase if the output flickers (was 150)

bool stateLED = false;    //to turn on or off the led

void setup(){
  pinMode(boton1, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop(){
  buttonpress = "released";   //release the variable buttonpress every time the void loops
  if (digitalRead(boton1) == LOW) buttonpress = "Boton1";   

  if (millis() - time > debounceTime){    //debounce
    if (buttonpress != lastbuttonpress && buttonpress != "released"){
      if (buttonpress == "Boton1"){
        stateLED = !stateLED;
        time = millis();
      }
    }
  }
  digitalWrite(LED_BUILTIN, stateLED);
}
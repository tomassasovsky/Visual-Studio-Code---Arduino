#define relayPin 0
#define ledPin 1
#define buttonPin 2

int buttonRead;
boolean pedalState = false;

void setup(){
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(ledPin, OUTPUT);
    pinMode(relayPin, OUTPUT);
}
void loop(){
    buttonRead = digitalRead(buttonPin);
    if(buttonRead == 0){  //if button is pressed
        pedalState = !pedalState;  //if pedal is on, turn it off. Else, turn it on.
        changeModes();
        delay(150);
    }
}
void changeModes(){
    switch(pedalState){
        case true:      //when pedal is on, turn relay and led on.
            digitalWrite (ledPin, HIGH);
            digitalWrite (relayPin, HIGH);
        break;
        default:        //when pedal is off, turn relay and led off.
            digitalWrite (ledPin, LOW);
            digitalWrite (relayPin, LOW);
        break;
    }
}
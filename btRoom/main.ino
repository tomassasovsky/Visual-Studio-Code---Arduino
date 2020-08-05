#include <RBDdimmer.h> //dimmer library

#define zerocross  2
#define dimmer_1_Out  4 //FAN OUT
#define dimmer_2_Out 3 //LIGHTS OUT
#define lights_button A0
#define fan_button A1
int tube = 6;

String buttonPress = " ";
String lastbuttonpress = "";    //To store the previous button pressed

dimmerLamp dimmer1(dimmer_1_Out);// FAN OUT
dimmerLamp dimmer2(dimmer_2_Out);// LIGHTS OUT

long time = 0;    //the last time the output pin was toggled
long debounceTime = 150;    //the debounce time, increase if the output flickers (was 150)

int fan_state = 0;
int fan_speed;

int lights_state = 0;

int tube_state = 0;

char Received = ' ';
bool canprint = true;

void setup() {
  Serial.begin(9600);
  dimmer1.begin(NORMAL_MODE, OFF);
  dimmer2.begin(NORMAL_MODE, OFF);
  pinMode(fan_button, INPUT_PULLUP);
  pinMode(lights_button, INPUT_PULLUP);
  pinMode(tube, OUTPUT);
}


void loop() {
    if(Serial.available()>0) Received = Serial.read();

    buttonPress = "released";   //release the variable buttonpress every time the void loops

    if(digitalRead(fan_button) == LOW) buttonPress = "Fan";
    if(digitalRead(lights_button) == LOW) buttonPress = "Lights";
    

    if (millis() - time > debounceTime){    //debounce
        if (buttonPress != lastbuttonpress && buttonPress != "released"){    //in any mode
            if (buttonPress == "Lights") lights_state++, time = millis();
            if (buttonPress == "Fan") fan_state++, time = millis();
        }
        lastbuttonpress = buttonPress;
    }
    
    if (Received == 'F') fan_state++, Received = ' ', canprint = true;
 
    switch(fan_state){
        case 1:
            dimmer1.setState(ON);
            dimmer1.setPower(55);
            fan_speed = 20;
        break;
        case 2:
            dimmer1.setPower(60);
            fan_speed = 40;
        break;
        case 3:
            dimmer1.setPower(67);
            fan_speed = 60;
        break;
        case 4:
            dimmer1.setPower(75);
            fan_speed = 80;
        break;
        case 5:
            dimmer1.setPower(94);
            fan_speed = 100;
        break;
        default:
            dimmer1.setPower(0);
            dimmer1.setState(OFF);
            if (canprint == true){
                Serial.println("Fan is OFF");
                canprint = false;
            }
            fan_speed = 0;
            fan_state = 0;
        break;        
    }

    if (fan_state != 0 && canprint == true){
        Serial.print("Fan Speed:");
        printSpace(dimmer1.getPower()); 
        Serial.print(fan_speed);
        Serial.println("%");
        canprint = false;    
    }
 
    
    if(Received == 'L') lights_state++, Received = ' ';
 
    switch(lights_state){
        case 1:
            dimmer2.setState(ON);
            dimmer2.setPower(96);
        break;
        default:
            dimmer2.setPower(0);
            dimmer2.setState(OFF);
            lights_state = 0;
        break;        
    }
        
    if(Received == 'T'){
        tube_state++;
        Received = ' '; 
    }
 
    switch(tube_state){
        case 1:
            digitalWrite(tube, HIGH);
        break;
        default:
            digitalWrite(tube, LOW);
            tube_state = 0;
        break;        
    }

}

void printSpace(int val){
  if ((val / 100) == 0) Serial.print(" ");
  if ((val / 10) == 0) Serial.print(" ");
}
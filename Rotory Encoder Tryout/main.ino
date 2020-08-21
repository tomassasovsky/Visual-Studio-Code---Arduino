 #define outputA A1
 #define outputB A0
 int counter = 0; 
 int State;
 int LastState;  
 void setup() { 
   pinMode (outputA, INPUT_PULLUP);
   pinMode (outputB, INPUT_PULLUP);
   
   Serial.begin (9600);
   // Reads the initial state of the outputA
   LastState = digitalRead(outputA);   
 } 
 void loop() { 
   State = digitalRead(outputA); // Reads the "current" state of the outputA
   // If the previous and the current state of the outputA are different, that means a Pulse has occured
   if (State != LastState){     
     // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
     if (digitalRead(outputB) != aState) { 
       counter ++;
     } else {
       counter --;
     }
     Serial.print("Position: ");
     Serial.println(counter);
   } 
   LastState = State; // Updates the previous state of the outputA with the current state
 }
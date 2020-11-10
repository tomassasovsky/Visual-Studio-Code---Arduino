int LED1 = 12;
int LED2 = 13;
int button = 3;

bool LED1State = false;
bool LED2State = false;

bool buttonActive = false;
bool longPressActive = false;

long buttonTimer = 0;
long longPressTime = 250;

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(button, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(button) == LOW) {
    if (!buttonActive) {
      buttonActive = true;
      buttonTimer = millis();
    }
    if ((millis() - buttonTimer > longPressTime) && (!longPressActive)) {
      longPressActive = true;
      LED1State = !LED1State;
      digitalWrite(LED1, LED1State);
    }
  } else {
    if (buttonActive) {
      if (longPressActive) {
        longPressActive = false;
      } else {
        LED2State = !LED2State;
        digitalWrite(LED2, LED2State);
      }
      buttonActive = false;
    }
  }
}
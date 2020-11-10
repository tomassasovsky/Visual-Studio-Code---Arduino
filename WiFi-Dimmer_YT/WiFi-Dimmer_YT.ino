#include "UbidotsESPMQTT.h"
#include "hw_timer.h" 

#define TOKEN "BBFF-Y5yh3G3nM6UaVVBg00iAIwvneC13y1" // Your Ubidots TOKEN

#define WIFINAME "Tomato24" //Your SSID
#define WIFIPASS "" // Your Wifi Pass
#define DEVICE_LABEL "myDimmer"

#define VARIABLE_LABEL1  "bombillo"  // Nombre de variable en Ubidots
#define VARIABLE_LABEL2  "ventilador"  // Nombre de variable en Ubidots

const int ERROR_VALUE = 65535;  // Valor de error cualquiera

const uint8_t NUMBER_OF_VARIABLES = 2; // Cantidad de variables a las que el programa se va a suscribir
char * variable_labels[NUMBER_OF_VARIABLES] = {"bombillo","ventilador"}; // Nombres de las variables

const byte zcPin = 0;
const byte pwmPin = 2;
int luz = 1;  
int boton = 3;

byte fade = 1;
byte state = 1;
byte tarBrightness = 255;
byte curBrightness = 0;
byte zcState = 0; // 0 = ready; 1 = processing;

int ledState = LOW;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50; 

float estadoluz; // Nombre de la variable que se va a usar en el codigo
byte estadoventilador; // Nombre de la variable que se va a usar en el codigo

float value; // Variable para almacenar el dato de entrada.
uint8_t variable; // Para usar en el switch case

Ubidots ubiClient(TOKEN);

WiFiClient client;

void callback(char* topic, byte* payload, unsigned int length) {
  char* variable_label = (char *) malloc(sizeof(char) * 30);
  get_variable_label_topic(topic, variable_label);
  value = btof(payload, length);
  set_state(variable_label);
  execute_cases();
  free(variable_label);
  digitalWrite(luz, estadoluz);

  if (estadoventilador > 0) {
    tarBrightness = estadoventilador;
    Serial.println(tarBrightness);
  }
}

// Parse topic to extract the variable label which changed value
void get_variable_label_topic(char * topic, char * variable_label) {
  Serial.print("topic:");
  Serial.println(topic);
  sprintf(variable_label, "");
  for (int i = 0; i < NUMBER_OF_VARIABLES; i++) {
    char * result_lv = strstr(topic, variable_labels[i]);
    if (result_lv != NULL) {
      uint8_t len = strlen(result_lv);      
      char result[100];
      uint8_t i = 0;
      for (i = 0; i < len - 3; i++) { 
        result[i] = result_lv[i];
      }
      result[i] = '\0';
      Serial.print("Label is: ");
      Serial.println(result);
      sprintf(variable_label, "%s", result);
      break;
    }
  }
}

// cast from an array of chars to float value.
float btof(byte * payload, unsigned int length) {
  char * demo = (char *) malloc(sizeof(char) * 10);
  for (int i = 0; i < length; i++) {
    demo[i] = payload[i];
  }
  float value = atof(demo);
  free(demo);
  return value;
}

// State machine to use switch case
void set_state(char* variable_label) {
  variable = 0;
  for (uint8_t i = 0; i < NUMBER_OF_VARIABLES; i++) {
    if (strcmp(variable_label, variable_labels[i]) == 0) {
      break;
    }
    variable++;
  }
  if (variable >= NUMBER_OF_VARIABLES) variable = ERROR_VALUE; // Not valid
}

// Function with switch case to determine which variable changed and assigned the value accordingly to the code variable
void execute_cases() {  
  switch (variable) {
    case 0:
      estadoluz = map(value,0,3,1,255);
      Serial.print("Luz: ");
      Serial.println(estadoluz);
      Serial.println();
      break;
    case 1:
      estadoventilador = map(value,0,5,1,255);
      Serial.print("Ventilador: ");
      Serial.println(estadoventilador);
      Serial.println();
      break;
    case ERROR_VALUE:
      Serial.println("error");
      Serial.println();
      break;
    default:
      Serial.println("default");
      Serial.println();
  }

}

void setup() {
  ubiClient.ubidotsSetBroker("industrial.api.ubidots.com"); // Sets the broker properly for the business account
  ubiClient.setDebug(false); // Pass a true or false bool value to activate debug messages
  Serial.begin(115200);
  ubiClient.wifiConnection(WIFINAME, WIFIPASS);
  ubiClient.begin(callback);
  if(!ubiClient.connected()){
  ubiClient.reconnect();
  }
  pinMode(zcPin, INPUT);
  pinMode(pwmPin, OUTPUT);
  digitalWrite(pwmPin,LOW);
  pinMode(luz, OUTPUT);
  digitalWrite(luz,LOW);
  pinMode(boton, INPUT);
  // attachInterrupt(zcPin, zcDetectISR, RISING);    // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  hw_timer_init(NMI_SOURCE, 0);
  hw_timer_set_func(dimTimerISR);
  
  char* deviceStatus = getUbidotsDevice(DEVICE_LABEL);

  if (strcmp(deviceStatus, "404") == 0) {
    ubiClient.add("bombillo", 0); //Insert your variable Labels and the value to be sent
    ubiClient.ubidotsPublish(DEVICE_LABEL);
    ubiClient.add("ventilador", 0); //Insert your variable Labels and the value to be sent
    ubiClient.ubidotsPublish(DEVICE_LABEL);
    
    //ubiClient.loop();
  }
  ubiClient.ubidotsSubscribe(DEVICE_LABEL,VARIABLE_LABEL1); //Insert the Device and Variable's Labels
  ubiClient.ubidotsSubscribe(DEVICE_LABEL,VARIABLE_LABEL2); //Insert the Device and Variable's Labels
}

void loop() {
  if(!ubiClient.connected()){
    ubiClient.reconnect();
    ubiClient.ubidotsSubscribe(DEVICE_LABEL,VARIABLE_LABEL1); //Insert the Device and Variable's Labels
    ubiClient.ubidotsSubscribe(DEVICE_LABEL,VARIABLE_LABEL2); //Insert the Device and Variable's Labels
  }

  ubiClient.loop();
    
  // read the state of the switch into a local variable:
  int reading = digitalRead(boton);

  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is LOW
      if (buttonState == LOW) {
        ledState = !ledState;
        // set the LED:
      digitalWrite(luz, ledState);
      ubiClient.add("bombillo", ledState); //Insert your variable Labels and the value to be sent
      ubiClient.ubidotsPublish(DEVICE_LABEL);
      delay(100);
      }
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
}



char* getUbidotsDevice(char* deviceLabel) {
  char* data = (char *) malloc(sizeof(char) * 700);
  char* response = (char *) malloc(sizeof(char) * 400);
  sprintf(data, "GET /api/v1.6/devices/%s/", deviceLabel);
  sprintf(data, "%s HTTP/1.1\r\n", data);
  sprintf(data, "%sHost: industrial.api.ubidots.com\r\nUser-Agent:dimmer/1.0\r\n", data);
  sprintf(data, "%sX-Auth-Token: %s\r\nConnection: close\r\n\r\n", data, TOKEN);
  free(data);
 
  if (client.connect("industrial.api.ubidots.com", 80)) {
    client.println(data);
  } 
  else {
    free(data);
    free(response);
    return "e";
  }
  int timeout = 0;
  while(!client.available() && timeout < 5000) {
    timeout++;
    if (timeout >= 4999){
      free(data);
      free(response);
      return "e";
    }
    delay(1);
    }

  int i = 0;
  while (client.available()) {
    response[i++] = (char)client.read();
    if (i >= 399){
      break;
    }
  }
  char * pch;
  char * statusCode;
  int j = 0;
  pch = strtok (response, " ");
  while (pch != NULL) {
    if (j == 1 ) {
      statusCode = pch;
    }

    pch = strtok (NULL, " ");
    j++;
  }
  free(response);
  return statusCode;

}

void dimTimerISR() {
  if (fade == 1) {
    if (curBrightness > tarBrightness || (state == 0 && curBrightness > 0)) {
      --curBrightness;
    }
    else if (curBrightness < tarBrightness && state == 1 && curBrightness < 255) {
      ++curBrightness;
    }
  }
  else {
    if (state == 1) {
      curBrightness = tarBrightness;
    }
    else {
      curBrightness = 0;
    }
  }
  
  if (curBrightness == 0) {
    state = 0;
    digitalWrite(pwmPin, 0);
  }
  else if (curBrightness == 255) {
    state = 1;
    digitalWrite(pwmPin, 1);
  }
  else {
    digitalWrite(pwmPin, 1);
  }    
  zcState = 0;
}

void zcDetectISR() {
  if (zcState == 0) {
    zcState = 1;
    if (curBrightness < 255 && curBrightness > 0) {
      digitalWrite(pwmPin, 0);
      int dimDelay = 30 * (255 - curBrightness) + 400;//400
      hw_timer_arm(dimDelay);
    }
  }
}
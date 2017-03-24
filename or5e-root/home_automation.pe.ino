#include <DHT.h>

/*
 * This is programmed to do a home automation which can connect to touch switch and computer to communitate the status of pins.
 * 
 * We are using Arduino Mega for this project
 * Touch Switch - 8 Pins (D2 - D9)
 * Relay Driver - 8 Pins (D22 - D29)
 * 
 * Date: 12/Feb/2917
 * Author: Ponraj Suthanthiramani
 */
DHT dht;

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

//Configure Command Array
/**
 * GSS => GET System Status
 * GCS => Get Current Status
 * SST => Set Switch To SST 1 0
 * 
 */
const String commands[] = {"GSS", "GCS", "SST", "TMP"};

//Configuring Digital Touch input pins (Make Sure it is a Ditital Pin)
const int digitalTouchPins[] = {2, 3, 4, 5, 6, 7, 8, 9};

//We have this to hold the status of output which drives the relay
int digitalPinStatus[] = {0, 0, 0, 0, 0, 0, 0, 0};

//We have this to preserve last status of the button
int digitalPinLastStatus[] = {0, 0, 0, 0, 0, 0, 0, 0};

//Adressing for loop iteration max pins
int pin_count;  

//Configure delay in main loop NOTE: Smaller you have better the touch works
const int timerDelay = 100;

//Configure Digital out to control Relay circuit (Make Sure it is a Ditital Pin)
int relayControlPins[] = {46, 47, 48, 49, 50, 51, 52, 53};

//Configure DTH 11 Pin (Make Sure it is a Ditital Pin)
int DTH_11_PIN = 10;
/**
 * Setup the project
 */
void setup() {
  
  //Step 0: Calculate the max count in the digital input which will be used to iterate over it
  pin_count = sizeof(digitalTouchPins) / sizeof(int);

  // Step 1: Configure modes for all digital input pin to INPUT
  for ( int index = 0 ; index < pin_count ; index ++ ) {
    pinMode(digitalTouchPins[index], INPUT);
  }
  // Step 2: Configure modes for all digital outout pin to OUPUT and set all pins to HIGH
  for ( int index = 0 ; index < pin_count ; index ++ ) {
    pinMode(relayControlPins[index], OUTPUT);
    digitalWrite(relayControlPins[index], HIGH);
  }

  // Step 3: Setup Serial Port so that it can communicate the status to the computer for further enhancement
  Serial.begin(9600);

  //Step 4: Setup DTH Sensor Pin
  
  dht.setup(DTH_11_PIN);
}

/**
 * Main loop
 */
void loop() {

  // Step 1: Read all the digital pin status an set the status approriatly
  for ( int index = 0 ; index < pin_count ; index ++ ) {
    int pinStatus = digitalRead( digitalTouchPins[index] );

    /**
     * pinStatus => Current
     * digitalPinLastStatus[index] => Previous
     * Logic used to Toggle Switch
     * | Previous | Current | Action               |
     * |    0     |    0    | No Action            |
     * |    0     |    1    | Toggle + Update Prev |
     * |    1     |    1    | No Action            |
     * |    1     |    0    | Update Prev          |
     */
    if( !digitalPinLastStatus[index] && pinStatus ) {
      toggleStatusAndSetSwitch ( index );
      digitalPinLastStatus[index] = pinStatus;
    }
    if( digitalPinLastStatus[index] && !pinStatus ) {
      digitalPinLastStatus[index] = 0;
    }
  }

  // Step 2: Send to serial communicator
  if (stringComplete) {
    processCommands(inputString);
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
  delay(timerDelay);
}

/**
 * Toggle the switch status and set the relay as the status
 */
void toggleStatusAndSetSwitch(int index) {
  //Step 1: Toggling the current status
  digitalPinStatus[index] = ! digitalPinStatus[index];

  //Step 2: Set the relay driver to the current status
  setRelay(index);

  //Step 3: Communicate to the computer or further tracking
  /*
   * Computer Format
   * [PinNo | Status]
   * eg: 
   * If the pin 1 toggles to on
   * [1|ON]
   * If the pin 1 toggles to off
   * [1|OFF]
   * 
   */
  Serial.print( "[" );
  Serial.print( index + 1 );
  Serial.print( "|");
  Serial.println( ( digitalPinStatus[index] ) ? "ON]" : "OFF]" );
}

void setRelay(int index) {
  if(digitalPinStatus[index] == 0) {
    digitalWrite(relayControlPins[index], HIGH);
  }
  else {
    digitalWrite(relayControlPins[index], LOW);
  }
}
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void processCommands( String query ) {
  String cmd = query.substring(0,3);
  //GSS (Get System Status)
  if(commands[0] == cmd) {
    Serial.println("STATUS OK");
  }
  //GCS (Get Current Status)
  else if(commands[1] == cmd) {
    String response = "{";
    for ( int index = 0 ; index < pin_count ; index ++ ) {
      if( digitalPinStatus[index] ) {
        response += "[";
        response += (index+1);
        response += "|ON]";
      }
    }
    response += "}";
    Serial.print("STATUS:");
    Serial.println(response);
  }
  //SST Set Switch To
  else if(commands[2] == cmd) {
    int pinToSet = query.substring(4, 5).toInt();
    int setValue = query.substring(6, 7).toInt();
    pinToSet = pinToSet - 1;
    digitalPinStatus[pinToSet] = setValue;
    setRelay(pinToSet);
    Serial.println("Set Successful.");
  }
  //TMP (Get Temprature
  else if(commands[3] == cmd) {
      delay(dht.getMinimumSamplingPeriod());
      float humidity = dht.getHumidity();
      float temperature = dht.getTemperature();
      Serial.print("[");
      Serial.print(dht.getStatusString());
      Serial.print("|");
      Serial.print(dht.getHumidity(), 1);
      Serial.print("|");
      Serial.print(dht.getTemperature(), 1);
      Serial.print("|");
      Serial.print(dht.toFahrenheit(temperature), 1);
      Serial.println("]");
  }
  else {
    Serial.print("Command not Recogonized.");
    Serial.println(cmd);
  }
}


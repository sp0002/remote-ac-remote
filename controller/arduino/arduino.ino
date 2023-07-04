#define IR_SEND_PIN 3

#include <IRremote.hpp> 

const byte numChars = 32;
char receivedChars[numChars];
boolean newData = false;

void setup() { 
  Serial.begin(9600);
  IrSender.begin(ENABLE_LED_FEEDBACK);
}

void loop() {
  recvWithEndMarker();
  processData();
}

void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
    
  if (Serial.available() > 0) {
    rc = Serial.read();

    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    } else {
      receivedChars[ndx] = '\0';  // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void processData() {
  if (newData == true) {
    Serial.print("Received: ");
    Serial.println(receivedChars);
    if (strncmp(receivedChars, "0x", 2) == 0){
      IrSender.sendLG(0x88, strtol(receivedChars, NULL, 16), 2);
    }
    newData = false;
  }
}

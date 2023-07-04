#include <IRremote.hpp> // include the IRremote library

#define RECEIVER_PIN 2
// RECEIVER: - to GND, mid pin to 5V, S to digital pin

void setup() {
  Serial.begin(9600); // begin serial communication with a baud rate of 9600
  delay(6000);

  IrReceiver.begin(RECEIVER_PIN);
  Serial.print(F("Ready to receive IR signals at pin "));
  Serial.println(RECEIVER_PIN);
  printActiveIRProtocols(&Serial);
}

void loop() {
  if (IrReceiver.decode()) {
    IrReceiver.printIRResultShort(&Serial);
    IrReceiver.printIRSendUsage(&Serial);
    IrReceiver.resume();
  }
}
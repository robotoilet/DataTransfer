#include <Wire.h>

#include "YunClient.h"
#include "SPI.h"
#include <PubSubClient.h>

#include "yunTimestamp.h"
#include "yunTransmitter.h"

#define COLLECTOR 0x1
#define STORAGE 0x2
#define TRANSMITTER 0x3


#define SEND_RESOLUTION 11 // when to check for sensors to report [s]


void setup() {
  Wire.begin(TRANSMITTER);
  Serial.begin(9600);
  Bridge.begin();
}

// we need a global `previousValue` in order not to run collectData
// a million billion times while loop has found the right second
unsigned long previousValue = 0;
void loop() {
  unsigned long seconds = millis() / 1000;
  if (seconds != previousValue) {
    Serial.println("second " + String(seconds));
    if (seconds % (SEND_RESOLUTION) == 0) {
      Serial.println("send resolution triggered.");
      sendData();
    }
  }
  previousValue = seconds;
}


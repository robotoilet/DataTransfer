#include "Arduino.h"

#include "YunClient.h"
#include "SPI.h"
#include <PubSubClient.h>

#define BYTESUM_CHARLENGTH 6
#define CHECKSUM_LENGTH TIMESTAMP_LENGTH + BYTESUM_CHARLENGTH

#define BUFFER_LENGTH 23 * 5

#define CHECK_FOR_UPDATES_DURATION 5000

#define COLLECTOR 0x1
#define STORAGE 0x2

#define TIMESTAMP_LENGTH 10
#define TS_INDEX_DP 3

#define NO_DATA_CHAR '!'

byte server[] = {10, 10, 63, 175};  // WIFI-specifics
//byte server[] = {192, 168, 0, 7};  // WIFI-specifics


void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "verifiedData") == 0) {
    removeTransferredFile(payload, length);
  } else {
    Serial.println("T: CALLBACK with unknown topic!!!: " + String(topic));
  }
}

int freERam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

YunClient eClient;
PubSubClient client(server, 1883, callback, eClient);


void buildChecksum(char* checksum, char* buffer) {
  unsigned long checksumByteSum = 0; // sum of all byte-values of the file

  char ts[TIMESTAMP_LENGTH + 1];
  int i = 0;
  for (i;i<strlen(buffer);i++) {
    if (i>=TS_INDEX_DP && i<TS_INDEX_DP+TIMESTAMP_LENGTH) {
      ts[i-TS_INDEX_DP] = buffer[i];
    } else if (i == TS_INDEX_DP+TIMESTAMP_LENGTH) {
      ts[i-TS_INDEX_DP] = '\0';
    }
    checksumByteSum += (byte)buffer[i];
  }
  //Serial.println("checksumByteSum: " + String(checksumByteSum));
  sprintf(checksum, "%s:%ld", ts, checksumByteSum);
}

bool requestDataForSending(char* sendBuffer, char* checksum) {
  bool gotData = getDataForSending(sendBuffer);
  if (gotData) buildChecksum(checksum, sendBuffer);
  Serial.println("T: checksum: " + String(checksum));
  return gotData;
}

void checkForUpdates() {
  unsigned long start = millis();
  // keep listening for a while
  while (millis() - start <= CHECK_FOR_UPDATES_DURATION) {
    if (client.loop()) break;
  }
  client.disconnect();
}

// 3. Data Transfer to server
void sendData() {
  if (client.connected() || client.connect("siteX", "punterX", "punterX")) {
    Serial.println("T: connected!");
    char sendBuffer[BUFFER_LENGTH + 1];
    char checksum[CHECKSUM_LENGTH + 1];
    if (requestDataForSending(sendBuffer, checksum)) {
      Serial.println("[T:] trying to send ..");
      if (client.publish(checksum, sendBuffer)) {
        Serial.println("[T:] success?..");
      }
    } else {
      Serial.println("[T:] No data for sending available!");
    }
    checkForUpdates();
  } else {
    Serial.println("[T:] no connection!");
  }
}

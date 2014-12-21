#define YUN
//#define UNO

#include "Timer.h"
#include "TestSensor.h"
#include <PubSubClient.h>

#define SECOND 1000L
#define MINUTE 60 * SECOND
#define HOUR 60 * MINUTE

#ifdef YUN
  #include <SPI.h>
  #include "YunBoard.h"
  #include <YunClient.h>

  #define LOGDIR  "/mnt/sda1"
  #define LOGPATH "/mnt/sda1/"
  #define BOARD_TYPE YunBoard

  YunClient eClient;
#endif

#ifdef UNO
  #include <SdFat.h>
  #include "UnoBoard.h"
  #include <Wire.h>
  #include "RTClib.h"
  #include "Ethernet.h"

  #define LOGDIR ""
  #define LOGPATH ""
  #define BOARD_TYPE UnoBoard

  EthernetClient eClient;
#endif

#define TIMESTAMP_LENGTH 10                    // number of chars of a unix ts
#define LOGPATH_TIMESTAMP_INDEX sizeof(LOGDIR) - 1 // where to write the ts in the path
#define LOGPATH_LABEL_INDEX LOGPATH_TIMESTAMP_INDEX + TIMESTAMP_LENGTH // where to write '.c'
                                                                       // or '.s'
#define CLOSED_SUFFIX "C"
#define SENT_SUFFIX "S"
#define LOG_SUFFIX "L"

#define DOT "."
#define DOT_LENGTH sizeof(DOT) -1
#define LABEL_LENGTH sizeof(CLOSED_SUFFIX) - 1

#define FILEPATH_LENGTH LOGPATH_LABEL_INDEX + DOT_LENGTH + LABEL_LENGTH

#define BYTESUM_CHARLENGTH 6
#define CHECKSUM_LENGTH TIMESTAMP_LENGTH + BYTESUM_CHARLENGTH

#define DATAPOINT_MAX 2

//byte server[] = { 10, 10, 63, 221 };  // WIFI-specifics
byte server[] = { 192, 168, 0, 7 };  // WIFI-specifics

BOARD_TYPE board(DATAPOINT_MAX);

// the actural sensors this sketch knows about
TestSensor<BOARD_TYPE> testSensorOne('1', &board); // create TestSensor with it's UID
TestSensor<BOARD_TYPE> testSensorTwo('2', &board); // create TestSensor with it's UID
TestSensor<BOARD_TYPE> testSensorThree('3', &board); // create TestSensor with it's UID

PubSubClient client(server, 1883, callback, eClient);

byte dataPointCounter = 0; // the number of dataPoints in one dataFile

Timer t;                   // the timer starts processes at configured time
                           //   (process: e.g. 'get sensor reading')

// TODO: remove if unused!
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("CALLBACK!!! topic: " + String(topic));
  // handle message arrived
}

void setup() {
  board.begin();
  Serial.begin(9600);

  while(!Serial);

  board.createNewDataFile();

  // task scheduling..
  // (1) sensor data collection and logging
  t.every(5 * SECOND, writeDataForSensorOne);
  t.every(10 * SECOND, writeDataForSensorTwo);
  t.every(11 * SECOND, writeDataForSensorThree);

  // (2) data transfer to server
//  t.every(10 * SECOND, sendData);

}


void writeDataForSensorOne() {
  testSensorOne.collectData();
}
void writeDataForSensorTwo() {
  testSensorTwo.collectData();
}
void writeDataForSensorThree() {
  testSensorThree.collectData();
}

void loop () {
  t.update();
}

// 3. Data Transfer to server
void sendData() {
  char sendFilePath[FILEPATH_LENGTH + 1] = LOGDIR;
  Serial.println("Preparing to send data");
  while (board.nextPathInDir(sendFilePath, CLOSED_SUFFIX)) {
    Serial.println("..for file " + String(sendFilePath));
    unsigned int bufferLength = board.fileSize(sendFilePath);
    char sendBuffer[bufferLength + 1];

    char checksum[CHECKSUM_LENGTH + 1] = {'\0'};
    char startEnd[10];
    // checksumBytes: sum of all byte-values of the file
    unsigned long checksumBytes = board.readFile(sendFilePath, sendBuffer);

    buildChecksum(checksum, sendBuffer, bufferLength, checksumBytes);

    Serial.println("trying to send it..");
    if (client.connect("siteX", "punterX", "punterX")) {
      Serial.println("Got a connection!");
      if (client.publish(checksum, sendBuffer)) {
        Serial.println("should have sent the stuff by now..");
        board.relabelFile(sendFilePath, SENT_SUFFIX);
      }
    } else {
      Serial.println("Didn't get a connection!");
    }
  }
}

void buildChecksum(char* checksum, char* buffer, unsigned int bufferLength, unsigned long bytes) {
  sprintf(checksum, "%ld", bytes);
  for (byte i=strlen(checksum); i<CHECKSUM_LENGTH; i++) {
    if (i < BYTESUM_CHARLENGTH) {
      checksum[i] = ':';
    } else {
      checksum[i] = buffer[bufferLength - (CHECKSUM_LENGTH - i)];
    }
  }
  checksum[CHECKSUM_LENGTH] = '\0';
}

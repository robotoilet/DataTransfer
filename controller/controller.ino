//#define YUN
#define UNO

#include "Timer.h"
#include "TestSensor.h"
#include <PubSubClient.h>

#define SECOND 1000L
#define MINUTE 60 * SECOND
#define HOUR 60 * MINUTE

#ifdef YUN
  #include "YunClient.h"
  #include "SPI.h"
  #include "YunBoard.h"
  #define BOARD_TYPE YunBoard
#endif

#ifdef UNO
  #include <SdFat.h>
  #include "UnoBoard.h"
  #include <Wire.h>
  #include "RTClib.h"
  #define BOARD_TYPE UnoBoard
#endif

#define DATAPOINT_MAX 5

BOARD_TYPE board(DATAPOINT_MAX);

// the actural sensors this sketch knows about
TestSensor<BOARD_TYPE> testSensorOne('a', &board); // create TestSensor with it's UID
TestSensor<BOARD_TYPE> testSensorTwo('c', &board); // create TestSensor with it's UID
TestSensor<BOARD_TYPE> testSensorThree('a', &board); // create TestSensor with it's UID


byte dataPointCounter = 0; // the number of dataPoints in one dataFile

Timer t;                   // the timer starts processes at configured time
                           //   (process: e.g. 'get sensor reading')

// uncomment to inspect for memory leaks (call this function where leaks suspected):
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// TODO: if we change the time library to accept member functions, we can
//       get rid of those useless functions:
void writeDataForSensorOne() { Serial.println("free ram: " + String(freeRam())); testSensorOne.collectData(); }
void writeDataForSensorTwo() { testSensorTwo.collectData(); }
void writeDataForSensorThree() { testSensorThree.collectData(); }

// 3. Data Transfer to server; specific to boards
void sendData() {
  board.sendData();
}

void setup() {
  board.begin();
  Serial.begin(9600);

  while(!Serial);

  board.createNewDataFile();

  // task scheduling..
  // (1) sensor data collection and logging
  t.every(1 * SECOND, writeDataForSensorOne);
  t.every(2 * SECOND, writeDataForSensorTwo);
  t.every(11 * SECOND, writeDataForSensorThree);

  // (2) data transfer to server
  t.every(10 * SECOND, sendData);
}

void loop () {
  t.update();
}

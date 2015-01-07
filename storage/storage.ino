//#define YUN
#define UNO

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
#define MAX_DATAPOINT_SIZE 23

#define COLLECTOR 1
#define STORAGE 2
#define TRANSMITTER 3

BOARD_TYPE board(DATAPOINT_MAX);

byte dataPointCounter = 0; // the number of dataPoints in one dataFile

// uncomment to inspect for memory leaks (call this function where leaks suspected):
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void collectData() {

// TODO: byte dataPointCounter = 0; // the number of dataPoints in one dataFile
  // TODO: if (b->checkCounter()) {

  // TODO: b->write(dataPoint, MAX_DATAPOINT_SIZE);

  Wire.requestFrom(COLLECTOR, MAX_DATAPOINT_SIZE);
  char dataPoint[MAX_DATAPOINT_SIZE];
  byte i = 0;
  while (Wire.available() > 0) {
    char c = Wire.read(); // receive a byte as character
    if (c == '!') break; // nada to retrieve; end this round of data collection
    if ((byte)c == 255) {// end of string, finish datapoint & try to get more
      dataPoint[i] = '\0';
      board.write(dataPoint); 
      Serial.print("just wrote datpoint " + String(dataPoint));
      collectData(); 
    }
    dataPoint[i] = c;
    i++;
  }
}

void sendData() {
  // TODO: 1. ask transmitter for a filepath that got sent off successfully
  //       2. give data for one *.C file to transmitter to send it off
  //       3. if transmitter has accepted this file, rename it to *.S
}

void setup() {
  board.begin();
  Serial.begin(9600);
  Wire.begin(STORAGE);

  board.createNewDataFile();
}

void loop () {
  t.update();
}

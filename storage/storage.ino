#include <SdFat.h>
#include <Wire.h>
#include "RTClib.h"

#define DATAPOINT_MAX 5

#define CLOSED_SUFFIX 'C'
#define SENT_SUFFIX 'S'
#define LOG_SUFFIX 'L'

#define LABEL_LENGTH 1

#define DOT '.'
#define DOT_LENGTH 1

#define TIMESTAMP_LENGTH 10                    // number of chars of a unix ts
#define FILEPATH_LENGTH TIMESTAMP_LENGTH + DOT_LENGTH + LABEL_LENGTH

#define COLLECTOR 0x1
#define STORAGE 0x2
#define TRANSMITTER 0x3

#define CHIP_SELECT 10
SdFat sd;

RTC_DS1307 rtc;

byte dataPointCounter = 0; // the number of dataPoints in one dataFile

char filePath[FILEPATH_LENGTH + 1];

// uncomment to inspect for memory leaks (call this function where leaks suspected):
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
void reportFreeRam() {
  Serial.println("Free Ram : " + String(freeRam()));
}

void setup() {
  Serial.begin(9600);
  Wire.begin(STORAGE);
  sd.begin(CHIP_SELECT, SPI_HALF_SPEED);
  createNewDataFile();
  Wire.onReceive(writeDataPoint);
}

void createNewDataFile(){
  resetFilePath();
  SdFile f(filePath, O_CREAT | O_WRITE | O_EXCL);
  f.close();
  Serial.println("created new datafile" + String(filePath));
}

unsigned long fileId = 9999999; // GLOBAL: starting number will be this + 1
void resetFilePath() {
  if (fileId == 99999999) fileId = 9999999;
  fileId++;
  filePath[0] = '\0';
  sprintf(filePath, "%ld", fileId);
  addFilePathSuffix(LOG_SUFFIX);
  while (sd.exists(filePath)) {
    resetFilePath();
  }
}

void addFilePathSuffix(char suffix) {
  strcat(filePath, ".00");
  filePath[FILEPATH_LENGTH -1] = suffix;
}

void writeDataPoint(int size) {
  checkCounter();
  SdFile f;
  if(!f.open(filePath, O_RDWR | O_CREAT | O_AT_END)) {
    Serial.println(F("Error writing file"));
  }
  while (Wire.available() > 0) {
    char c = Wire.read();
    f.print(c);
    Serial.print(c);
  }
  f.close();
  Serial.println(F("written datapoint"));
}

bool checkCounter() {
  reportFreeRam();
  //Serial.print(dataPointCounter);
  if (dataPointCounter < DATAPOINT_MAX) {
    dataPointCounter ++;
    return true;
  } else {
    // 1. relabel file to say it's closed
    SdFile f(filePath, O_CREAT | O_WRITE );   //init file
    filePath[FILEPATH_LENGTH - 1] = CLOSED_SUFFIX;
    if (!f.rename(sd.vwd(), filePath)) Serial.println(F("error renaming"));
    f.close();
    // 2. create a new file with timestamped name
    createNewDataFile();
    dataPointCounter = 0;
    return false;
  }
}

void sendData() {
  // TODO: 1. ask transmitter for a filepath that got sent off successfully
  //       2. give data for one *.C file to transmitter to send it off
  //       3. if transmitter has accepted this file, rename it to *.S
}

void loop () {
}

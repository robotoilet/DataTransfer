#include <FileIO.h>
#include "Timer.h"
#include "TestSensor.h"

// for MQTT stuff
#include <SPI.h>
#include <YunClient.h>
#include <PubSubClient.h>

#define SECOND 1000L
#define MINUTE 60 * SECOND
#define HOUR 60 * MINUTE

#define LOG_DIR "/mnt/sda1"
#define LOG_PATH "/mnt/sda1/datalog_"
#define LOG_PATH_TIMESTAMP 18 // where to write the ts

#define HR_TIMESTAMP_SIZE 15
#define LOG_PATH_LABEL LOG_PATH_TIMESTAMP + HR_TIMESTAMP_SIZE // where to write '.closed'
                                                              // or '.sent'
#define CLOSED_SUFFIX ".clsd"
#define SENT_SUFFIX ".sent"
#define LABEL_SIZE 5

#define UNIX_TIMESTAMP_SIZE 10

#define BYTESUM_CHARLENGTH 6
#define CHECKSUM_LENGTH HR_TIMESTAMP_SIZE + BYTESUM_CHARLENGTH

#define DATAPOINT_MAX 20

// global vars
File logDir = FileSystem.open(LOG_DIR);

char dataFilePath[42] = LOG_PATH;
byte dataPointCounter = 0; // the number of dataPoints in one dataFile
char unixTimestamp[10];    // a unix timestamp
char hrTimestamp[16];      // a human readable timestamp

Timer t;                   // the timer starts processes at configured time
                           //   (process: e.g. 'get sensor reading')
unsigned long checksumByteSum;
char checksum[CHECKSUM_LENGTH];  // for our own 'checksum' calculation

byte server[] = { 127, 0, 0, 1 };  // WIFI-specifics

YunClient yunClient;
PubSubClient client(server, 1883, callback, yunClient);

// the actural sensors this sketch knows about
TestSensor testSensorOne('1'); // create TestSensor with it's UID
TestSensor testSensorTwo('2'); // create TestSensor with it's UID


// TODO: remove if unused!
void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}


void setup() {
  Bridge.begin();
  Serial.begin(9600);
  FileSystem.begin();

  while(!Serial);
  Serial.println("data transfer of fake data\n");

  createNewDataFile();

  // task scheduling..
  // (1) sensor data collection and logging
  t.every(5 * SECOND, writeDataForSensorOne);
  t.every(10 * SECOND, writeDataForSensorTwo);

  // (3) data transfer to server
  t.every(5 * MINUTE, sendData);

}

void createNewDataFile(){
  getTimestamp(hrTimestamp);
  for (byte i=0; i<HR_TIMESTAMP_SIZE; i++) {
    dataFilePath[LOG_PATH_TIMESTAMP + i] = char(hrTimestamp[i]);
  }
  Serial.println("dataFilePath: " + String(dataFilePath));
  // Only create the file here with the right filename;
  // due to a bug I can't use dataFile as a global var with Yun:
  // https://github.com/arduino/Arduino/issues/1810
  File dataFile = FileSystem.open(dataFilePath, FILE_APPEND);
  dataFile.close();
}

void relabelDataFile(char* fileName, char* label){
  Process rename;
  rename.begin("mv");
  rename.addParameter(fileName);
  Serial.println("filename before: " + String(fileName));
  Serial.println("LOG_PATH_LABEL - 1 : " + String(LOG_PATH_LABEL - 1));
  Serial.println("filename at this pos: " + String(fileName[LOG_PATH_LABEL - 1]));
  for (byte i=0; i<LABEL_SIZE; i++) {
    Serial.println("LOG_PATH_LABEL + i: " + String(i + LOG_PATH_LABEL));
    Serial.println("filename at this pos before: " + String(fileName[i + LOG_PATH_LABEL]));
    fileName[LOG_PATH_LABEL + i] = label[i];
    Serial.println("filename at this pos after: " + String(fileName[i + LOG_PATH_LABEL]));
    Serial.println("filename total: " + String(fileName));
  }
  rename.addParameter(fileName);
  Serial.println("renamed filename to: " + String(fileName));
  rename.run();
  fileName[LOG_PATH_LABEL] = '\0';
}

// Write the current time into a provided char array;
// If the length of the array equals 10 a unix timestamp
// is created (10 characters), otherwise a human-readable format will be
// written (16 characters)
void getTimestamp(char* tsArray) {
  Process time;
  time.begin("date");
  if (tsArray == unixTimestamp) {
    time.addParameter("+%s");
  } else {
    time.addParameter("+%Y%m%d-%H%M%S");
  }
  time.run();

  int i = 0;
  while(time.available()>0) {
    char c = time.read();
    if(c != '\n')
      tsArray[i] = c;
      i ++;
  }
}

void writeDataForSensorOne() {
  if (checkCounter()) {
    // due to a bug I can't use dataFile as a global var with Yun:
    // https://github.com/arduino/Arduino/issues/1810
    File dataFile = FileSystem.open(dataFilePath, FILE_APPEND);
    testSensorOne.collectData(dataFile, unixTimestamp);
    dataFile.close();
  }
}
void writeDataForSensorTwo() {
  if (checkCounter()) {
    // due to a bug I can't use dataFile as a global var with Yun:
    // https://github.com/arduino/Arduino/issues/1810
    File dataFile = FileSystem.open(dataFilePath, FILE_APPEND);
    testSensorTwo.collectData(dataFile, unixTimestamp);
    dataFile.close();
  }
}

bool checkCounter() {
  Serial.println("dataPointCounter: " + String(dataPointCounter));
  boolean bol;
  if (dataPointCounter < DATAPOINT_MAX) {
    dataPointCounter ++;
    bol = true;
  } else {
    relabelDataFile(dataFilePath, CLOSED_SUFFIX);
    // 2. create a new file with timestamped name
    createNewDataFile();
    bol = false;
    dataPointCounter = 0;
  }
  // in any case get a current timestamp to pass on to sensor(s)
  getTimestamp(unixTimestamp);
  Serial.println("created unixTimestamp: " + String(unixTimestamp));
  return bol;
}

void loop () {
  t.update();
}


bool isClosed(const char* filename) {
  byte sizeName = sizeof(filename);
  byte sizeSuffix = sizeof(CLOSED_SUFFIX);
  for (byte i=0;i<sizeSuffix;i++) {
    if (filename[(sizeName - 1 - i)] != CLOSED_SUFFIX[sizeSuffix - 1]) {
      return false;
    }
  }
  return true;
}


// 3. Data Transfer to server
void sendData() {
  Serial.println("Preparing to send data");
  while (true) {
    File logFile = logDir.openNextFile();
    const char* fName = logFile.name();
    if (isClosed(fName)) {
      char sendBuffer[logFile.size()]; // we can do this as all in one line?
      readFile(logFile, sendBuffer);
      buildChecksum(fName);
      Serial.println("Created checksum: " + String(checksum));
      // try to send it..
      if (client.connect("siteX", "punterX", "punterX")) {
        client.publish(checksum, sendBuffer);
        logFile.close();
        dataFilePath[0] = '\0';
        strcpy(dataFilePath, fName);
        relabelDataFile(dataFilePath, SENT_SUFFIX);
      } else {
        logFile.close();
      }
      // close and rename it..
    } else {
      logFile.close();
    }
  }
}

void buildChecksum(const char* fName) {
  checksum[0] = '\0';
  String(checksumByteSum).toCharArray(checksum, BYTESUM_CHARLENGTH);
  for (byte i=0;i<HR_TIMESTAMP_SIZE;i++) {
    checksum[BYTESUM_CHARLENGTH + i] = fName[LOG_PATH_TIMESTAMP + i];
  }
}

void readFile(File f, char* buffer) {
  checksumByteSum = 0;
  unsigned int i = 0;
  int b = f.read();
  while (b != -1) {
    buffer[i] = (char)b;
    checksumByteSum += b;
    i++;
  }
}

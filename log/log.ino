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
#define FILENAME_SIZE LOG_PATH_LABEL + LABEL_SIZE

#define UNIX_TIMESTAMP_SIZE 10

#define BYTESUM_CHARLENGTH 6
#define CHECKSUM_LENGTH HR_TIMESTAMP_SIZE + BYTESUM_CHARLENGTH

#define DATAPOINT_MAX 10

// global vars
File logDir = FileSystem.open(LOG_DIR);

char dataFilePath[FILENAME_SIZE + 1] = LOG_PATH;
byte dataPointCounter = 0; // the number of dataPoints in one dataFile
char unixTimestamp[10];    // a unix timestamp
char hrTimestamp[16];      // a human readable timestamp

Timer t;                   // the timer starts processes at configured time
                           //   (process: e.g. 'get sensor reading')
unsigned long checksumByteSum;
char checksum[CHECKSUM_LENGTH];  // for our own 'checksum' calculation

byte server[] = { 192, 168, 0, 7 };  // WIFI-specifics

YunClient yunClient;
PubSubClient client(server, 1883, callback, yunClient);

// the actural sensors this sketch knows about
TestSensor testSensorOne('1'); // create TestSensor with it's UID
TestSensor testSensorTwo('2'); // create TestSensor with it's UID
TestSensor testSensorThree('3'); // create TestSensor with it's UID
TestSensor testSensorFour('4'); // create TestSensor with it's UID
TestSensor testSensorFive('5'); // create TestSensor with it's UID
TestSensor testSensorSix('6'); // create TestSensor with it's UID
TestSensor testSensorSeven('7'); // create TestSensor with it's UID
TestSensor testSensorEight('8'); // create TestSensor with it's UID
TestSensor testSensorNine('9'); // create TestSensor with it's UID


// TODO: remove if unused!
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("CALLBACK!!! topic: " + String(topic));
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
  t.every(11 * SECOND, writeDataForSensorThree);
  t.every(12 * SECOND, writeDataForSensorFour);
  t.every(13 * SECOND, writeDataForSensorFive);
  t.every(14 * SECOND, writeDataForSensorSix);
  t.every(15 * SECOND, writeDataForSensorSeven);
  t.every(16 * SECOND, writeDataForSensorEight);
  t.every(17 * SECOND, writeDataForSensorNine);

  // (3) data transfer to server
  t.every(20 * SECOND, sendData);

}

void createNewDataFile(){
  getTimestamp(hrTimestamp);
  for (byte i=0; i<HR_TIMESTAMP_SIZE; i++) {
    dataFilePath[LOG_PATH_TIMESTAMP + i] = char(hrTimestamp[i]);
  }
  Serial.println("\ndataFilePath: " + String(dataFilePath));
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
  for (byte i=0; i<LABEL_SIZE; i++) {
    fileName[LOG_PATH_LABEL + i] = label[i];
  }
  rename.addParameter(fileName);
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
void writeDataForSensorThree() {
  if (checkCounter()) {
    // due to a bug I can't use dataFile as a global var with Yun:
    // https://github.com/arduino/Arduino/issues/1810
    File dataFile = FileSystem.open(dataFilePath, FILE_APPEND);
    testSensorThree.collectData(dataFile, unixTimestamp);
    dataFile.close();
  }
}
void writeDataForSensorFour() {
  if (checkCounter()) {
    // due to a bug I can't use dataFile as a global var with Yun:
    // https://github.com/arduino/Arduino/issues/1810
    File dataFile = FileSystem.open(dataFilePath, FILE_APPEND);
    testSensorFour.collectData(dataFile, unixTimestamp);
    dataFile.close();
  }
}
void writeDataForSensorFive() {
  if (checkCounter()) {
    // due to a bug I can't use dataFile as a global var with Yun:
    // https://github.com/arduino/Arduino/issues/1810
    File dataFile = FileSystem.open(dataFilePath, FILE_APPEND);
    testSensorFive.collectData(dataFile, unixTimestamp);
    dataFile.close();
  }
}
void writeDataForSensorSix() {
  if (checkCounter()) {
    // due to a bug I can't use dataFile as a global var with Yun:
    // https://github.com/arduino/Arduino/issues/1810
    File dataFile = FileSystem.open(dataFilePath, FILE_APPEND);
    testSensorSix.collectData(dataFile, unixTimestamp);
    dataFile.close();
  }
}
void writeDataForSensorSeven() {
  if (checkCounter()) {
    // due to a bug I can't use dataFile as a global var with Yun:
    // https://github.com/arduino/Arduino/issues/1810
    File dataFile = FileSystem.open(dataFilePath, FILE_APPEND);
    testSensorSeven.collectData(dataFile, unixTimestamp);
    dataFile.close();
  }
}
void writeDataForSensorEight() {
  if (checkCounter()) {
    // due to a bug I can't use dataFile as a global var with Yun:
    // https://github.com/arduino/Arduino/issues/1810
    File dataFile = FileSystem.open(dataFilePath, FILE_APPEND);
    testSensorEight.collectData(dataFile, unixTimestamp);
    dataFile.close();
  }
}
void writeDataForSensorNine() {
  if (checkCounter()) {
    // due to a bug I can't use dataFile as a global var with Yun:
    // https://github.com/arduino/Arduino/issues/1810
    File dataFile = FileSystem.open(dataFilePath, FILE_APPEND);
    testSensorNine.collectData(dataFile, unixTimestamp);
    dataFile.close();
  }
}

bool checkCounter() {
  Serial.print("dp" + String(dataPointCounter) + "-");
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
  return bol;
}

void loop () {
  t.update();
}


bool isClosed(const char* filename) {
  for (byte i=0;i<LABEL_SIZE;i++) {
    if (filename[(FILENAME_SIZE - 1 - i)] != CLOSED_SUFFIX[LABEL_SIZE - 1 - i]) {
      return false;
    }
  }
  return true;
}


// 3. Data Transfer to server
void sendData() {
  char sendFilePath[FILENAME_SIZE + 1] = LOG_PATH;
  Serial.println("Preparing to send data");
  while (File logFile = logDir.openNextFile()) {
    const char* fName = logFile.name();
    Serial.println(fName);
    if (isClosed(fName)) {
      char sendBuffer[logFile.size()]; // we can do this as all in one line?
      readFile(logFile, sendBuffer);
      buildChecksum(fName);
      Serial.println("Created checksum: " + String(checksum));
      // try to send it..
      if (client.connect("siteX", "punterX", "punterX")) {
        Serial.println("Got a connection!");
        client.publish(checksum, sendBuffer);
        logFile.close();
        sendFilePath[0] = '\0';
        strcpy(sendFilePath, fName);
        relabelDataFile(sendFilePath, SENT_SUFFIX);
      } else {
        logFile.close();
      }
      // close and rename it..
    } else {
      logFile.close();
    }
  }
}

// build a 'checksum' of format <checksumByteSum>:<timestamp of filename>
void buildChecksum(const char* fName) {
  checksum[0] = '\0';
  String(checksumByteSum).toCharArray(checksum, BYTESUM_CHARLENGTH);

  // in case the bytesum is a digit less long then expected, we fill the
  // gap because otherwise the checksum wouldn't read as a whole
  if (checksum[BYTESUM_CHARLENGTH - 2] == '\0') {
    checksum[BYTESUM_CHARLENGTH - 2] = ':';
  }
  checksum[BYTESUM_CHARLENGTH - 1] = ':';

  for (byte i=0;i<HR_TIMESTAMP_SIZE;i++) {
    checksum[BYTESUM_CHARLENGTH + i] = fName[LOG_PATH_TIMESTAMP + i];
  }
}

void readFile(File f, char* buffer) {
  Serial.println("starting to read " + String(f.name()));
  checksumByteSum = 0;
  unsigned int i = 0;
  int b = f.read();
  while (b != -1) {
    buffer[i] = (char)b;
    checksumByteSum += b;
    i++;
    b = f.read();
  }
}

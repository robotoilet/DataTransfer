#define YUN

#include "Timer.h"
#include "TestSensor.h"

// for MQTT stuff
#include <SPI.h>
#include <PubSubClient.h>

#define SECOND 1000L
#define MINUTE 60 * SECOND
#define HOUR 60 * MINUTE

#define LOG_DIR "/mnt/sda1"
#define LOG_PATH "/mnt/sda1/"
#define LOG_PATH_TIMESTAMP sizeof(LOG_PATH) - 1 // where to write the ts

#define UNIX_TIMESTAMP_SIZE 10

#define LOG_PATH_LABEL_INDEX LOG_PATH_TIMESTAMP + UNIX_TIMESTAMP_SIZE // where to write '.c'
                                                                // or '.s'
#define CLOSED_SUFFIX ".c"
#define SENT_SUFFIX ".s"
#define LABEL_SIZE 5
#define FILENAME_SIZE LOG_PATH_LABEL_INDEX + LABEL_SIZE

#define BYTESUM_CHARLENGTH 6
#define CHECKSUM_LENGTH UNIX_TIMESTAMP_SIZE + BYTESUM_CHARLENGTH

#define DATAPOINT_MAX 10

byte server[] = { 10, 10, 63, 221 };  // WIFI-specifics

#ifdef YUN
  #include "YunBoard.h"
  #include <YunClient.h>
  YunBoard board; // change this to your board

  YunClient yunClient;
  PubSubClient client(server, 1883, callback, yunClient);

  // the actural sensors this sketch knows about
  TestSensor<YunBoard> testSensorOne('1'); // create TestSensor with it's UID
  TestSensor<YunBoard> testSensorTwo('2'); // create TestSensor with it's UID
  TestSensor<YunBoard> testSensorThree('3'); // create TestSensor with it's UID
  TestSensor<YunBoard> testSensorFour('4'); // create TestSensor with it's UID
  TestSensor<YunBoard> testSensorFive('5'); // create TestSensor with it's UID
  TestSensor<YunBoard> testSensorSix('6'); // create TestSensor with it's UID
  TestSensor<YunBoard> testSensorSeven('7'); // create TestSensor with it's UID
  TestSensor<YunBoard> testSensorEight('8'); // create TestSensor with it's UID
  TestSensor<YunBoard> testSensorNine('9'); // create TestSensor with it's UID
#endif

// global vars

char dataFilePath[FILENAME_SIZE + 1] = LOG_PATH;
byte dataPointCounter = 0; // the number of dataPoints in one dataFile
char unixTimestamp[UNIX_TIMESTAMP_SIZE + 1];    // a unix timestamp

Timer t;                   // the timer starts processes at configured time
                           //   (process: e.g. 'get sensor reading')
unsigned long checksumByteSum;
char checksum[CHECKSUM_LENGTH];  // for our own 'checksum' calculation


// TODO: remove if unused!
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("CALLBACK!!! topic: " + String(topic));
  // handle message arrived
}


void setup() {
  board.begin();
  Serial.begin(9600);

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
  t.every(30 * SECOND, sendData);

}

void createNewDataFile(){
  board.getTimestamp(unixTimestamp);
  for (byte i=0; i<UNIX_TIMESTAMP_SIZE; i++) {
    dataFilePath[LOG_PATH_TIMESTAMP + i] = char(unixTimestamp[i]);
  }
  Serial.println("\ndataFilePath: " + String(dataFilePath));
  board.createFile(dataFilePath);
}

void writeDataForSensorOne() {
  if (checkCounter()) {
    testSensorOne.collectData(dataFilePath, FILENAME_SIZE + 1, unixTimestamp, board);
  }
}
void writeDataForSensorTwo() {
  if (checkCounter()) {
    testSensorTwo.collectData(dataFilePath, FILENAME_SIZE + 1, unixTimestamp, board);
  }
}
void writeDataForSensorThree() {
  if (checkCounter()) {
    testSensorThree.collectData(dataFilePath, FILENAME_SIZE + 1, unixTimestamp, board);
  }
}
void writeDataForSensorFour() {
  if (checkCounter()) {
    testSensorFour.collectData(dataFilePath, FILENAME_SIZE + 1, unixTimestamp, board);
  }
}
void writeDataForSensorFive() {
  if (checkCounter()) {
    testSensorFive.collectData(dataFilePath, FILENAME_SIZE + 1, unixTimestamp, board);
  }
}
void writeDataForSensorSix() {
  if (checkCounter()) {
    testSensorSix.collectData(dataFilePath, FILENAME_SIZE + 1, unixTimestamp, board);
  }
}
void writeDataForSensorSeven() {
  if (checkCounter()) {
    testSensorSeven.collectData(dataFilePath, FILENAME_SIZE + 1, unixTimestamp, board);
  }
}
void writeDataForSensorEight() {
  if (checkCounter()) {
    testSensorEight.collectData(dataFilePath, FILENAME_SIZE + 1, unixTimestamp, board);
  }
}
void writeDataForSensorNine() {
  if (checkCounter()) {
    testSensorNine.collectData(dataFilePath, FILENAME_SIZE + 1, unixTimestamp, board);
  }
}

bool checkCounter() {
  Serial.print("dp" + String(dataPointCounter) + "-");
  boolean bol;
  if (dataPointCounter < DATAPOINT_MAX) {
    dataPointCounter ++;
    bol = true;
  } else {
    relabelFile(dataFilePath, CLOSED_SUFFIX);
    // 2. create a new file with timestamped name
    createNewDataFile();
    bol = false;
    dataPointCounter = 0;
  }
  // in any case get a current timestamp to pass on to sensor(s)
  board.getTimestamp(unixTimestamp);
  return bol;
}

void loop () {
  t.update();
}

// 3. Data Transfer to server
void sendData() {
  char sendFilePath[FILENAME_SIZE + 1] = LOG_PATH;
  Serial.println("Preparing to send data");
  while (board.nextPathInDir(LOG_DIR, sendFilePath, CLOSED_SUFFIX)) {
    Serial.println("Checking file " + String(sendFilePath));
    Serial.println("file " + String(sendFilePath) + " is closed and ready to send!");
    char sendBuffer[board.fileSize(sendFilePath)]; // we can do this as all in one line?
    board.readFile(sendFilePath, sendBuffer, checksumByteSum);
    buildChecksum(sendFilePath);
    //Serial.println("Created checksum: " + String(checksum));
    // try to send it..
    if (client.connect("siteX", "punterX", "punterX")) {
      Serial.println("Got a connection!");
      client.publish(checksum, sendBuffer);
      relabelFile(sendFilePath, SENT_SUFFIX);
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

  for (byte i=0;i<UNIX_TIMESTAMP_SIZE;i++) {
    checksum[BYTESUM_CHARLENGTH + i] = fName[LOG_PATH_TIMESTAMP + i];
  }
}

void relabelFile(char* oldName, char* label){
  char newName[FILENAME_SIZE];
  strcpy(newName, oldName);
  for (byte i=0; i<LABEL_SIZE; i++) {
    newName[LOG_PATH_LABEL_INDEX + i] = label[i];
  }
  board.renameFile(oldName, newName);
}

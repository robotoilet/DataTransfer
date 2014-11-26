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

#define LOG_PATH "/mnt/sda1/datalog_"
#define LOG_PATH_TIMESTAMP 18 // where to write the ts
#define LOG_PATH_SUFFIX LOG_PATH_TIMESTAMP + 16 // where to write '.sent'

#define HR_TIMESTAMP_SIZE 16
#define UNIX_TIMESTAMP_SIZE 10

// global vars
char dataFilePath[42] = LOG_PATH;
byte dataPointCounter = 0; // the number of dataPoints in one dataFile
char unixTimestamp[10];    // a unix timestamp
char hrTimestamp[16];      // a human readable timestamp

Timer t;                   // the timer starts processes at configured time
                           //   (process: e.g. 'get sensor reading')
unsigned long timeChecksum;// for our own 'checksum' calculation
unsigned int valChecksum;  // for our own 'checksum' calculation

byte server[] = { 10, 10, 63, 129 };  // WIFI-specifics

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
//  t.every(MINUTE, sendData);

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

void renameDataFile(char* label, byte pos, byte size){
  Serial.println("renameDataFile: label=" + String(label) + ", pos=" + pos);

  Process rename;
  rename.begin("mv");
  rename.addParameter(dataFilePath);
  Serial.println("dataFilePath: " + String(dataFilePath));
  for (byte i=0; i<size; i++) {
    dataFilePath[pos + i] = label[i];
  }
  rename.addParameter(dataFilePath);
  Serial.println("dataFilePath: " + String(dataFilePath));
  rename.run();
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
  Serial.println("Starting to collect and write data for sensor one");
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
  if (dataPointCounter < 50) {
    dataPointCounter ++;
    bol = true;
  } else {
    renameDataFile(".closed", LOG_PATH_SUFFIX, 7);
    // 2. create a new file with timestamped name
    createNewDataFile();
    bol = false;
    dataPointCounter = 0;
  }
  // in any case get a current timestamp to pass on to sensor(s)
  Serial.println("unixTimestamp: " + String(unixTimestamp));
  getTimestamp(unixTimestamp);
  Serial.println("unixTimestamp: " + String(unixTimestamp));
  return bol;
}

void loop () {
  t.update();
}

//// 3. Data Transfer to server
//void sendData() {
//  Serial.println("Preparing to send data");
//  readData();
//  dataToSend = xDataGrouped + yDataGrouped + zDataGrouped;
//  Serial.println("Data to send: " + dataToSend);
//  dataToSend.toCharArray(sendBuffer, 300);
//  // generate a hex encoded
//
//  String stringsum = String(timeChecksum) + "_" + String(valChecksum);
//  char checksum[stringsum.length()];
//  stringsum.toCharArray(checksum, sizeof(checksum));
//
//  // now send it: try to connect, if there's a connection, publish
//  // the stuff
//  if (client.connect("siteX", "punterX", "punterX")) {
//    client.publish(checksum, sendBuffer);
//  }
//
//  // tidy up the checksums
//  timeChecksum = 0;
//  valChecksum = 0;
//}
//
//void readData() {
//  Serial.println("reading data");
//  File f = FileSystem.open(LOGFILE, FILE_APPEND);
//  String sensorName;
//  if (f) {
//    Serial.println("readData: file object there");
//    String line = readLine(f);
//    Serial.println("readData: have a line: " + line);
//    while (line != "EOF") {
//      sensorName = getSensorName(line);
//      if (sensorName == "sensorX")
//        xDataGrouped += getSensorData(line);
//      else if (sensorName == "sensorY")
//        yDataGrouped += getSensorData(line);
//      else if (sensorName == "sensorZ")
//        zDataGrouped += getSensorData(line);
//      line = readLine(f);
//    }
//    f.close();
//  }
//}
//
//String readLine(File f) {
//  int b = f.read();
//  while (b != -1) {
//    char c = (char)b;
//    String line = "";
//    if (c == '\n') {
//      return line;
//      line += c;
//      char c = (char)f.read();
//    }
//    return line; // breaks out of the outer while loop delivering a line
//  }
//  return "EOF";
//}
//
//String getSensorName(String line) {
//  int firstComma = line.indexOf(',');
//  return line.substring(0, firstComma);
//}
//String getSensorData(String line) {
//  int firstComma = line.indexOf(',');
//  return line.substring(firstComma + 1, line.length());
//}
//
//

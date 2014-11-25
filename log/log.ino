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

byte dataPointCounter = 0;

char* LOGFILE = "/mnt/sda1/datalog.txt";
File dataFile;

Timer t;

// for our own 'checksum' calculation
unsigned long timeChecksum;
unsigned int valChecksum;


// WIFI-specifics: Update these with values suitable for your network.
byte server[] = { 10, 10, 63, 129 };

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

YunClient yunClient;
PubSubClient client(server, 1883, callback, yunClient);

TestSensor testSensorOne(1); // create TestSensor with it's UID
TestSensor testSensorTwo(2); // create TestSensor with it's UID

void setup() {
  Bridge.begin();
  Serial.begin(9600);
  FileSystem.begin();

  while(!Serial);  
  Serial.println("data transfer of fake data\n");
  
  dataFile = FileSystem.open(LOGFILE + getTimestamp(, FILE_APPEND);
    
  // task scheduling..
  // (1) sensor data collection
  t.every(5 * SECOND, writeDataForSensorOne);
  Serial.println("just saved some data for sensorOne..");
  t.every(10 * SECOND, writeDataSensorTwo);
  Serial.println("just saved some data for sensorTwo..");
  
  // (3) data transfer to server
  t.every(MINUTE, sendData);

}

bool checkCounter() {
  if (dataPointCounter < 50) {
    dataPointCounter ++;
    return true;
  } else {
    dataFile.close();
    // 1. send the dataFile
    // 2. rename this particular dataFile to *.sent
    // 3. create again a new file with timestamped name
    dataFile = FileSystem.open(LOGFILE, FILE_APPEND);
    return false;
  }
}

writeDataForSensorOne() {
  // call checkPointer function
  if checkCounter() {
    testSensorOne.collectData(dataFile);
  }
}

writeDataForSensorTwo() {
  // call checkPointer function
  if checkCounter() {
    testSensorOne.collectData(dataFile);
  }
}

void loop () {
  t.update();
}

// 3. Data Transfer to server
void sendData() {
  Serial.println("Preparing to send data");
  xDataGrouped = "SensorX";
  yDataGrouped = "SensorY";
  zDataGrouped = "SensorZ";
  readData();
  dataToSend = xDataGrouped + yDataGrouped + zDataGrouped;
  Serial.println("Data to send: " + dataToSend);
  dataToSend.toCharArray(sendBuffer, 300);
  // generate a hex encoded
  
  String stringsum = String(timeChecksum) + "_" + String(valChecksum);
  char checksum[stringsum.length()];
  stringsum.toCharArray(checksum, sizeof(checksum));
  
  // now send it: try to connect, if there's a connection, publish
  // the stuff
  if (client.connect("siteX", "punterX", "punterX")) {
    client.publish(checksum, sendBuffer);
  }
  
  // tidy up the checksums
  timeChecksum = 0;
  valChecksum = 0;
}

void readData() {
  Serial.println("reading data");
  File f = FileSystem.open(LOGFILE, FILE_APPEND);
  String sensorName;
  if (f) {
    Serial.println("readData: file object there");
    String line = readLine(f);
    Serial.println("readData: have a line: " + line);
    while (line != "EOF") {
      sensorName = getSensorName(line);
      if (sensorName == "sensorX") 
        xDataGrouped += getSensorData(line);
      else if (sensorName == "sensorY")
        yDataGrouped += getSensorData(line);
      else if (sensorName == "sensorZ")
        zDataGrouped += getSensorData(line);
      line = readLine(f);
    }
    f.close();
  }
}

String readLine(File f) {
  int b = f.read();
  while (b != -1) {
    char c = (char)b;
    String line = "";
    if (c == '\n') {
      return line;
      line += c;
      char c = (char)f.read();
    }
    return line; // breaks out of the outer while loop delivering a line
  }
  return "EOF";
}

String getSensorName(String line) {
  int firstComma = line.indexOf(',');
  return line.substring(0, firstComma);
}
String getSensorData(String line) {
  int firstComma = line.indexOf(',');
  return line.substring(firstComma + 1, line.length());
}



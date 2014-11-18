#include <FileIO.h>
#include "Timer.h"

int SECOND = 1000;
int MINUTE = 60 * SECOND;
int HOUR = 60 * MINUTE;
char* LOGFILE = "/mnt/sda1/datalog.txt";

Timer t;
String xData;
String yData;
String zData;
String xDataGrouped;
String yDataGrouped;
String zDataGrouped;

void setup() {
  Bridge.begin();
  Serial.begin(9600);
  FileSystem.begin();

  Serial.println("Filesystem datalogger\n");

  // initialise Strings for each sensor, that will build up timestamp-value
  // pairs (as in add them to the strings) until they are written to a file
  resetCollectedData(); 
  
  // task scheduling..
  // (1) sensor data collection
  t.every(5 * SECOND, collectDataForSensorX);
  t.every(10 * SECOND, collectDataForSensorY);
  t.every(20 *SECOND, collectDataForSensorZ);
  // (2) data logging
  t.every(20 * SECOND, logData);
  // (3) data transfer to server
  t.every(MINUTE, sendData);
}

void loop () {
  t.update();
}

// 1. Data Collection (creating fake Sensor values):
//    helper functions that build *strings* of data, one for each sensor
String createSensorData() {
  return " (" + getTimeStamp() + " " + random(300) + ")";
}
String getTimeStamp() {
  String result;
  Process time;
  time.begin("date");
  time.addParameter("+%s"); // as seconds since 1970
  time.run();

  while(time.available()>0) {
    char c = time.read();
    if(c != '\n')
      result += c;
  }

  return result;
}
void collectDataForSensorX() {
  xData += createSensorData();
}
void collectDataForSensorY() {
  yData += createSensorData();
}
void collectDataForSensorZ() {
  zData += createSensorData();
}

// 2. Data Logging -- writes the Strings containing sensor data to a file
//    and resets them to be empty.
void logData() {
  Serial.println(xData);
  Serial.println(yData);
  Serial.println(zData);
  File dataFile = FileSystem.open(LOGFILE, FILE_APPEND);
  if (dataFile) {
    dataFile.println(xData);
    dataFile.println(yData);
    dataFile.println(zData);
    dataFile.close();
    resetCollectedData();
  }
  else {
    Serial.println("error opening datalog.txt");
  }

}
void resetCollectedData() {
  xData = "SensorX";
  yData = "SensorY";
  zData = "SensorZ";
}

// 3. Data Transfer to server
void sendData() {
  Serial.println("Preparing to send data");
  xDataGrouped = "SensorX";
  yDataGrouped = "SensorY";
  zDataGrouped = "SensorZ";
  readData();
  Serial.println(xDataGrouped);
  Serial.println(yDataGrouped);
  Serial.println(zDataGrouped);
}
void readData() {
  Serial.println("reading data");
  File f = FileSystem.open(LOGFILE, FILE_APPEND);
  String line = readLine(f);
  String sensorName;
  if (f) {
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
    while (c != '\n') {
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
  return line.substring(firstComma+1, line.length());
}



#include <FileIO.h>
#include "Timer.h"

Timer t;

String xData;
String yData;
String zData;

void setup() {
  Bridge.begin();
  Serial.begin(9600);
  FileSystem.begin();


  Serial.println("Filesystem datalogger\n");


  resetCollectedData();

  t.every(5000, collectDataForSensorX);
  t.every(10000, collectDataForSensorY);
  t.every(20000, collectDataForSensorZ);
  t.every(20000, logData);
}

void loop () {
  t.update();
}

void resetCollectedData() {
  xData = "SensorX";
  yData = "SensorY";
  zData = "SensorZ";
}

void collectDataForSensorX() {
  xData += getSensorData();
}

void collectDataForSensorY() {
  yData += getSensorData();
}

void collectDataForSensorZ() {
  zData += getSensorData();
}

void logData() {
  Serial.println(xData);
  Serial.println(yData);
  Serial.println(zData);
  File dataFile = FileSystem.open("/mnt/sda1/datalog.txt", FILE_APPEND);
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

String getSensorData() {
  return ", [" + getTimeStamp() + ", " + random(300) + "]";
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

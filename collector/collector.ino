#include "TestSensor.h"

#include "DHT.h"
#include "Dht22.h"

#include <Wire.h>
#include "RTClib.h"

#define REPORT_RESOLUTION 5 // when to check for sensors to report [s]

#define COLLECTOR 0x1
#define STORAGE 0x2

#define OPEN_DATAPOINT '('
#define CLOSE_DATAPOINT ")"
#define SEPARATOR ' '

#define MAX_VALUE_SIZE 10
#define TIMESTAMP_SIZE 10
#define MAX_DATAPOINT_SIZE MAX_VALUE_SIZE + TIMESTAMP_SIZE + 3


RTC_DS1307 rtc;

// the actural sensors and related info this sketch knows about
#define NUMBER_OF_SENSORS 2
// overview over used PINs
#define DHT22_PIN 5
Sensor* sensors[NUMBER_OF_SENSORS] = { new Dht22('a', 5, DHT22_PIN),
                                       new TestSensor('c', 10) };

// uncomment to inspect for memory leaks (call this function where leaks suspected):
int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void reportFreeRam() {
  Serial.println("Free Ram : " + String(freeRam()));
}

void setup() {
  Serial.begin(9600);
  Wire.begin(COLLECTOR);
  rtc.adjust(DateTime(__DATE__, __TIME__));
}

void collectData(unsigned long seconds) {
  for (byte i=0;i<NUMBER_OF_SENSORS;i++) {
    Sensor* sensor = sensors[i];
    if (sensor->wantsToReport(seconds)) {
      char dataPoint[MAX_DATAPOINT_SIZE];
      getDataPoint(sensor, dataPoint);
      submitDataPoint(dataPoint);
    }
  }
}

// Writes one or more value(s) to a dataPoint of format
// "(<sensorName> <timestamp> <value> [<value> ..])"
void getDataPoint(Sensor* sensor, char* dataPoint) {
  Serial.println("getting datapoint for: " + String(sensor->name));
  sprintf(dataPoint, "%c%c%c%ld%c", OPEN_DATAPOINT, sensor->name, SEPARATOR,
      rtc.now().unixtime(), SEPARATOR);
  char chArray[MAX_VALUE_SIZE];
  sensor->getData(chArray);
  strcat(dataPoint, chArray);
  strcat(dataPoint, CLOSE_DATAPOINT);
}

void submitDataPoint(char* dataPoint) {
  Wire.beginTransmission(STORAGE);
  Wire.write(dataPoint);
  Wire.endTransmission();
  Serial.println("Just sent datapoint: " + String(dataPoint));
}

// we need a global `previousValue` in order not to run collectData
// a million billion times while loop has found the right second
unsigned long previousValue = 0;
void loop() {
  unsigned long seconds = millis() / 1000;
  if (seconds != previousValue && seconds % (REPORT_RESOLUTION) == 0) {
    reportFreeRam();
    collectData(seconds);
  }
  previousValue = seconds;
}

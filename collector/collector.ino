#include "TestSensor.h"
#include "UltrasonicSR04.h"

#include <Wire.h>
#include "RTClib.h"

#define REPORT_RESOLUTION 5 // when to check for sensors to report [s]

#define COLLECTOR 0x1
#define STORAGE 0x2
#define TRANSMITTER 0x3

#define OPEN_DATAPOINT '('
#define CLOSE_DATAPOINT ")"
#define SEPARATOR ' '

#define MAX_VALUE_LENGTH 10
#define TIMESTAMP_LENGTH 10
#define MAX_DATAPOINT_LENGTH MAX_VALUE_LENGTH + TIMESTAMP_LENGTH + 3

// the actural sensors and related info this sketch knows about
#define NUMBER_OF_SENSORS 3
// overview over used PINs
#define US_TRIG_PIN 13
#define US_ECHO_PIN 12
Sensor* sensors[NUMBER_OF_SENSORS] = { new UltrasonicSR04('a', 5,
                                                          US_TRIG_PIN,
                                                          US_ECHO_PIN),
                                       new TestSensor('c', 10) };

RTC_DS1307 rtc;

// uncomment to inspect for memory leaks (call this function where leaks suspected):
int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void reportFreeRam() {
  Serial.println("Free Ram : " + String(freeRam()));
}

void collectData(unsigned long seconds) {
  for (byte i=0;i<NUMBER_OF_SENSORS;i++) {
    Sensor* sensor = sensors[i];
    if (sensor->wantsToReport(seconds)) {
      char dataPoint[MAX_DATAPOINT_LENGTH];
      getDataPoint(sensor, dataPoint);
      submitDataPoint(dataPoint);
    }
  }
}

void getTimestamp(char* ts) {
  sprintf(ts, "%ld", rtc.now().unixtime());
}

// Writes one or more value(s) to a dataPoint of format
// "(<sensorName> <timestamp> <value> [<value> ..])"
void getDataPoint(Sensor* sensor, char* dataPoint) {
  char unixTimestamp[TIMESTAMP_LENGTH + 1];
  Serial.println("getting timestamp..");

  getTimestamp(unixTimestamp);
  sprintf(dataPoint, "%c%c%c%s%c", OPEN_DATAPOINT, sensor->name, SEPARATOR,
          unixTimestamp, SEPARATOR);
  char chArray[MAX_VALUE_LENGTH];
  sensor->getData(chArray);
  strcat(dataPoint, chArray);
  strcat(dataPoint, CLOSE_DATAPOINT);
}

void submitDataPoint(char* dataPoint) {
  Serial.println("C: Submitting datapoint: " + String(dataPoint));
  Wire.beginTransmission(STORAGE);
  Wire.write('W'); // begin with instruction for STORAGE to write datapoint
  Wire.write(dataPoint);
  Wire.endTransmission();
}

void setup() {
  Wire.begin(COLLECTOR);
  Serial.begin(9600);
  rtc.begin();
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
  }
  // following line sets the RTC to the date & time this sketch was compiled
  // TODO: try to update via ntp
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}


// we need a global `previousValue` in order not to run collectData
// a million billion times while loop has found the right second
unsigned long previousValue = 0;
void loop() {
  unsigned long seconds = millis() / 1000;
  if (seconds != previousValue) {
    if (seconds % (REPORT_RESOLUTION) == 0) {
      collectData(seconds);
    }
    // TODO: add NTP update for timestamp
  }
  previousValue = seconds;
}

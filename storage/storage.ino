#include <SdFat.h>
#include <Wire.h>

#define DATAPOINT_MAX 5

#define CLOSED_SUFFIX "C"
#define SENT_SUFFIX "S"
#define LOG_SUFFIX "L"

#define LABEL_LENGTH 1

#define CLOSE_DATAPOINT ')'

#define DOT '.'
#define DOT_LENGTH 1

#define LOGDIR ""
#define LOGDIR_LENGTH 0

#define TIMESTAMP_LENGTH 10                    // number of chars of a unix ts
#define FILEPATH_LENGTH TIMESTAMP_LENGTH + DOT_LENGTH + LABEL_LENGTH

#define COLLECTOR 0x1
#define STORAGE 0x2
#define TRANSMITTER 0x3

#define TS_INDEX_DP 3

#define CHIP_SELECT SS

#define MAX_VALUE_LENGTH 10
#define MAX_DATAPOINT_LENGTH MAX_VALUE_LENGTH + TIMESTAMP_LENGTH + 3

#define NO_DATA "!"

#define SEND_RESOLUTION 10 // when to check for sensors to report [s]

SdFat sd;


// uncomment to inspect for memory leaks (call this function where leaks suspected):
int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void reportFreeRam() {
  Serial.println("Free Ram : " + String(freeRam()));
}

char filePath[FILEPATH_LENGTH + 1];
char sendFilePath[FILEPATH_LENGTH + 1];
byte dataPointCounter = DATAPOINT_MAX; // the number of dataPoints in one dataFile

void createFile(char* fPath) {
  SdFile f(fPath, O_CREAT | O_WRITE | O_EXCL);
  f.close();
}

// resets provided `fPath` to the default LOGDIR
void resetFilepath(char* fPath) {
  fPath[0] = '\0';
  strcpy(fPath, LOGDIR);
}

void createFilePath(char* fPath, char* label, char* ts) {
  resetFilepath(fPath);
  strcat(fPath, label); // start actual filename with `label`
  // now we have to deal with the fact that there *has* to be a dot in our
  // filename :-|
  // 1. fast-forward to the index after the label
  byte i = 0;
  while (fPath[i] != '\0') {
    i++;
  }
  // 2. write the timestamp 'around' the dot
  byte k = 0;
  for (i;i<FILEPATH_LENGTH;i++) {
    if (k == 8 - LABEL_LENGTH) { fPath[i] = DOT;
    } else if (k < 8 - LABEL_LENGTH) {
      fPath[i] = ts[k];      // until the dot we write as normal
    } else {
      fPath[i] = ts[k - 1];  // after the dot we still want the ts index - 1
    }
    k++;
  }
  fPath[i] = '\0';
  //Serial.println("S: creating filepath " + String(fPath));
}

void renameFile(char* oldName, char* newName) {
  sd.rename(oldName, newName);
}

// replaces chars from index `lIndex` of an array with `label`
// note: creates a copy, hence doesn't overwrite the array
void relabelFile(char* oldName, char* label, byte lIndex) {
  char newName[strlen(oldName)];
  strncpy(newName, oldName, lIndex);
  newName[lIndex] = '\0';
  strcat(newName, label);
  strcat(newName, oldName+lIndex+1);

  renameFile(oldName, newName);
  Serial.println("renamed " + String(oldName) + " to " + String(newName));
}

void removeTransferredFile(char* checksum) {
  char rmfPath[FILEPATH_LENGTH + 1];
  char ts[TIMESTAMP_LENGTH + 1];
  strncpy(ts, checksum, TIMESTAMP_LENGTH);
  ts[TIMESTAMP_LENGTH] = '\0';
  createFilePath(rmfPath, SENT_SUFFIX, ts);

  if (sd.remove(rmfPath)) {
    Serial.println("[S:] removed file: " + String(rmfPath));
  }
}

void createNewDataFile(char* dataPoint){
  char ts[TIMESTAMP_LENGTH + 1];
  // copy the timestamp from the datapoint into the ts array
  strncpy(ts, dataPoint+TS_INDEX_DP, TIMESTAMP_LENGTH);
  ts[TIMESTAMP_LENGTH] = '\0';
  createFilePath(filePath, LOG_SUFFIX, ts);
  createFile(filePath);
}

bool startNewFile() {
  if (dataPointCounter >= DATAPOINT_MAX) {
    dataPointCounter = 1;
    return true;
  }
  dataPointCounter ++;
  return false;
}

void writeDataPoint(char* dataPoint) {
  if (startNewFile()) {
    if (sd.exists(filePath)) {
      // rename current file
      sendFilePath[0] = '\0';
      strcat(sendFilePath, filePath);
      sendFilePath[0] = 'C';
      Serial.println(sendFilePath);
      relabelFile(filePath, CLOSED_SUFFIX, LOGDIR_LENGTH);
    }
    // create a new file with timestamped name
    createNewDataFile(dataPoint);
  }
  SdFile f;
  if(!f.open(filePath, O_RDWR | O_CREAT | O_AT_END)) {
    Serial.println(F("Error writing file"));
  } else {
    for (byte i=0;i<strlen(dataPoint);i++) {
      f.write(dataPoint[i]);
    }
    f.close();
    Serial.println("Written " + String(dataPoint) + " to " + String(filePath));
  }
}

// string s, filter f
bool matchesFilter(const char* s, const char* f) {
  for (byte i=0;i<strlen(f);i++) {
    if (f[i] != s[LOGDIR_LENGTH + i]) {
      return false;
    }
  }
  return true;
}

// path: where to write the next matching path (gets passed prepopulated with the directory)
// labelFilter: label characters of filename we want
bool sendNextFileContent(char* labelFilter) {
  SdFile f;
  reportFreeRam();
  Serial.println("checking files in directory..");
  while(f.openNext(sd.vwd(), O_READ)) {
    char fName[13];
    f.getFilename(fName);
    Serial.println("checking " + String(fName));
    if (matchesFilter(fName, labelFilter)) {
      Serial.println(fName);
      sendFileContent(f);
      f.close();
      return true;
    }
    f.close();
  }
  return false;
}

void sendFileContent(char* fPath) {
  SdFile f;
  if (!f.open(fPath, O_READ)) {
    sd.errorHalt("opening file for read failed");
    return;
  }
  Serial.println(fPath);

  sendFileContent(f);

  f.close();

  fPath[0] = '\0';
}

void sendFileContent(SdFile f) {
  Wire.beginTransmission(TRANSMITTER);
  byte b;
  for (int i=0;i<BUFFER_LENGTH-2;i++) {
    b = f.read();
    if (b == 255) {
      Wire.write('!');
      Wire.endTransmission();
      return;
    }
    Wire.write((char)b);
  }
  Wire.endTransmission();
  sendFileContent(f);
}


void getDataForSending() {
//  Wire.beginTransmission(TRANSMITTER);
  if (sendNextFileContent(CLOSED_SUFFIX)) { // any data available?
    Serial.println("hurray");
    reportFreeRam();
    //relabelFile(sendFilePath, SENT_SUFFIX, LOGDIR_LENGTH);
  } else {
    Serial.println("S: can't find any files to send from");
//    Wire.write(NO_DATA);
  }
//  Wire.endTransmission();
  sd.chdir();
}

void doJob(int dataSize) {
  char* msg = new char[dataSize];
  byte instruction = 'F'; // fault by default
  byte i = 0;
  while (Wire.available() > 0) {
    if (i == 0) {
      // read the first char, which by convention is our instruction what to do
      instruction = Wire.read();
    } else {
      // write everything else into our message array
      char c = Wire.read();
      msg[i - 1] = c;
      if (c == ')') {
        msg[i] = '\0';
        break;
      }
    }
    i ++;
  }
  switch (instruction) {
    case 68: // char 'D' == delete file instruction
      Serial.println("S: have2remove file");
      removeTransferredFile(msg);
      break;
    case 87: // char 'W' == write datapoint instruction
      Serial.println("S: have2write datapoint");
      writeDataPoint(msg);
      break;
    default:
      Serial.println("S Error!");
  }
  delete[] msg;
}

void setup() {
  Serial.begin(9600);
  Wire.begin(STORAGE);
  if (!sd.begin(CHIP_SELECT, SPI_HALF_SPEED)) sd.initErrorHalt();
  Serial.println(BUFFER_LENGTH);
  Wire.onReceive(doJob);
}

// we need a global `previousValue` in order not to run collectData
// a million billion times while loop has found the right second
unsigned long previousValue = 0;
void loop() {
  unsigned long seconds = millis() / 1000;
  if (seconds != previousValue) {
    Serial.println(seconds);
    if (seconds % (SEND_RESOLUTION) == 0) {
      Serial.println("S: send resolution triggered.");
      if (sendFilePath[0] != '\0') {
        sendFileContent(sendFilePath);
      }
    }
  }
  previousValue = seconds;
}

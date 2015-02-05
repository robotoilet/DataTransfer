#include <SdFat.h>
#include <Wire.h>

#define DATAPOINT_MAX 3

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
#define MAX_DATAPOINT_LENGTH

#define NO_DATA "!"

SdFat sd;


char filePath[FILEPATH_LENGTH + 1];
byte dataPointCounter = 1; // the number of dataPoints in one dataFile

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
  Serial.println("S: creating filepath " + String(fPath));
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

bool checkCounter(char* dataPoint) {
  //Serial.print("dp" + String(dataPointCounter) + " - ");
  boolean bol = true;
  if (dataPointCounter == 1) {
    // rename current file
    if (sd.exists(filePath)) {
      relabelFile(filePath, CLOSED_SUFFIX, LOGDIR_LENGTH);
    }
    // create a new file with timestamped name
    createNewDataFile(dataPoint);
    dataPointCounter ++;
  } else if (dataPointCounter < DATAPOINT_MAX) {
    dataPointCounter ++;
  } else {
    bol = false;
    dataPointCounter = 0;
  }
  return bol;
}

void writeDataPoint(char* dataPoint) {
  checkCounter(dataPoint);
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
bool nextPathInDir(char* path, char* labelFilter) {
  SdFile f;
  Serial.println("checking files in directory..");
  while(f.openNext(sd.vwd(), O_READ)) {
    Serial.println("hellooo..");
    char fName[13];
    f.getFilename(fName);
    Serial.println("checking file " + String(fName));
    if (matchesFilter(fName, labelFilter)) {
      path[0] = '\0';
      strcat(path, fName);
      return true;
    }
    f.close();
  }
  return false;
}

bool readFile(SdFile f, char* buffer) {
  byte i = 0;
  int b = f.read();
  while (b != -1) {
    Serial.print((char)b);
    buffer[i] = (char)b;
    i++;
    if ((char)b == CLOSE_DATAPOINT) {
      buffer[i] = '\0';
      return true;
    }
    b = f.read();
  }
  f.close();
  return false;
}

char sendFilePath[FILEPATH_LENGTH + 1] = "";
SdFile sendFile;
void sendDataPoint() {
  char sendBuffer[MAX_DATAPOINT_LENGTH + 1];
  Serial.println("checking file " + String(sendFilePath));
  if (readFile(sendFile, sendBuffer)) { // still data available?
    Serial.println("sending datapoint: " + String(sendBuffer));
    Wire.write(sendBuffer);
  } else { // finished reading this file
    Serial.println("sending no data char: " + String(NO_DATA));
    Wire.write(NO_DATA);
    relabelFile(sendFilePath, SENT_SUFFIX, LOGDIR_LENGTH);
    sendFilePath[0] = '\0';
  }
}

void getDataForSending() {
  if (sendFilePath[0] == '\0') {  // starting at the beginning of a fresh file
    Serial.println("sendfilepath empty");
    if (nextPathInDir(sendFilePath, CLOSED_SUFFIX)) { // any data available?
      Serial.println("sendFilePath: " + String(sendFilePath));
      if(sendFile.open(sendFilePath, O_READ)) {
        sendDataPoint();
      } else {
        Serial.println(F("Error reading file"));
      }
    } else {
      Serial.println("S: can't find any matching files at " + String(sendFilePath));
      Wire.write(NO_DATA);
    }
  } else {
    sendDataPoint();
  }
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
      Serial.println("S: got instruction to remove file for checksum " + String(msg));
      removeTransferredFile(msg);
      break;
    case 87: // char 'W' == write datapoint instruction
      Serial.println("S: got instruction to write datapoint " + String(msg));
      writeDataPoint(msg);
      break;
    default:
      Serial.println("S: ERROR! Unknown instruction from Wire: " + String((char)instruction));
  }
  delete[] msg;
}

void setup() {
  Serial.begin(9600);
  Wire.begin(STORAGE);
  if (!sd.begin(CHIP_SELECT, SPI_HALF_SPEED)) sd.initErrorHalt();
  Wire.onReceive(doJob);
  Wire.onRequest(getDataForSending);
}

void loop() {
}

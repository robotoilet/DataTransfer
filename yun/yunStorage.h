#include "Arduino.h"
#include "FileIO.h"
#include "YunClient.h"

#include "yunRelabel.h"

#define LOGDIR  "/mnt/sda1/"
#define LOGDIR_WITHOUT_SLASH  "/mnt/sda1"
#define LOGDIR_LENGTH  sizeof(LOGDIR) - 1

#define TIMESTAMP_LENGTH 10                    // number of chars of a unix ts

#define CLOSED_SUFFIX "C"
#define LOG_SUFFIX "L"
#define SENT_SUFFIX "S"
#define LABEL_LENGTH 1

#define DOT '.'
#define DOT_LENGTH 1

#define FILEPATH_LENGTH LOGDIR_LENGTH + TIMESTAMP_LENGTH + DOT_LENGTH + LABEL_LENGTH

#define DATAPOINT_MAX 5

#define TS_INDEX_DP 3



char filePath[FILEPATH_LENGTH + 1];
byte dataPointCounter = 0;

void createFile(char* fPath) {
  File f = FileSystem.open(fPath, FILE_WRITE);
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

void removeTransferredFile(byte* checksum, unsigned int length) {
  char rmfPath[FILEPATH_LENGTH + 1] = LOGDIR;
  char ts[TIMESTAMP_LENGTH + 1];

  for (byte i=0;i<TIMESTAMP_LENGTH;i++) {
    ts[i] = (char)checksum[i];
  }
  ts[TIMESTAMP_LENGTH] = '\0';
  createFilePath(rmfPath, SENT_SUFFIX, ts);
  if (FileSystem.remove(rmfPath)) {
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
  if (dataPointCounter == 0) {
    // rename current file
    if (FileSystem.exists(filePath)) {
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

void writeDataPoint(char* data) {
  checkCounter(data);
  Serial.println("S: Writing data " + String(data) + " to " + String(filePath));
  File f = FileSystem.open(filePath, FILE_APPEND);
  for (byte i=0;i<strlen(data);i++) {
    if (data[i] == '\0') {
      break;
    }
    f.print(data[i]);
  }
  f.close();
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
// suffixFilter: label characters of filename we want
bool nextPathInDir(char* path, char* labelFilter) {
  File dir = FileSystem.open(path);
  while(File f = dir.openNextFile()) {
    if (matchesFilter(f.name(), labelFilter)) {
      path[0] = '\0';
      strcat(path, f.name());
      return true;
    }
  }
  return false;
}

long fileSize(char* fPath) {
  File f = FileSystem.open(fPath);
  long size = f.size();
  f.close();
  return size;
}

unsigned long readFile(char* fPath, char* buffer) {
  File f = FileSystem.open(fPath);
  unsigned long checksumByteSum = 0;
  unsigned int i = 0;
  int b = f.read();
  while (b != -1) {
    buffer[i] = (char)b;
    checksumByteSum += b;
    i++;
    b = f.read();
  }
  buffer[i] = '\0';
  f.close();
  return checksumByteSum;
}


bool getDataForSending(char* sendBuffer) {
  char* sendFilePath = new char[FILEPATH_LENGTH + 1];
  strcat(sendFilePath, LOGDIR_WITHOUT_SLASH);
  unsigned long checksumBytes = 0;
  bool success = false;
  if (nextPathInDir(sendFilePath, CLOSED_SUFFIX)) {
    char startEnd[10];
    // checksumBytes: sum of all byte-values of the file
    readFile(sendFilePath, sendBuffer);
    relabelFile(sendFilePath, SENT_SUFFIX, LOGDIR_LENGTH);
    success = true;
  } else {
    Serial.println("S: can't find any matching files at " + String(sendFilePath));
  }
  delete[] sendFilePath;
  return success;
}

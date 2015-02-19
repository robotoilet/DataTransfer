#include "YunClient.h"

void renameFile(char* oldName, char* newName) {
  Process rename;
  rename.begin("mv");
  rename.addParameter(oldName);
  rename.addParameter(newName);
  rename.run();
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

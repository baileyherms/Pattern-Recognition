#ifndef __Object__
#define __Object__

#include "Arduino.h"

// FIXME: Remove features that aren't needed
typedef struct {
  String type;
  int pattern[5];
  // Maybe add pattern length feature
} Object;


#endif

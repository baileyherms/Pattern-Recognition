#ifndef __Object__
#define __Object__

#include "Arduino.h"

// FIXME: Remove features that aren't needed
typedef struct {
  String type;
  float height;
  float width;
  float weight;
  float red;
  float green;
  float blue;
} Object;


#endif


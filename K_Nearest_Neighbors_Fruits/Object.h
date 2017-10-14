#ifndef __Object__
#define __Object__

#include "Arduino.h"

typedef struct {
  String type;
  //float height;
  //float width;
  float weight; // In grams
  float r;
  float g;
  float b;
} Object;


#endif

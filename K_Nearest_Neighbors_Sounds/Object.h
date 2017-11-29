#ifndef __Object__
#define __Object__

#include "Arduino.h"

typedef struct {
  String type;
  int freq[];
  int freq_size;
} Object;


#endif


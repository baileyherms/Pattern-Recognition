#ifndef __Object__
#define __Object__

#include "Arduino.h"

typedef struct {
    String category;
    float weight;
    float r;
    float g;
    float b;
} Object;

#endif
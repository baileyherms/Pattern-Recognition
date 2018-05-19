#ifndef __Object__
#define __Object__

#include "Arduino.h"

typedef struct {
    String category;
    float height;
    float width;
    float length;
} Object;

#endif
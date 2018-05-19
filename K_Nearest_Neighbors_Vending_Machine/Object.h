#ifndef __Object__
#define __Object__

#include "Arduino.h"

typedef struct {
    String category;
    float x_accel_diff;
    float y_accel_diff;
    float z_accel_diff;
} Object;

#endif
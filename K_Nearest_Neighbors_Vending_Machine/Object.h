#ifndef __Object__
#define __Object__

#include "Arduino.h"

// FIXME: Add and remove features
typedef struct {
  String type;
  float x_accel_diff;
  float y_accel_diff;
  float z_accel_diff;
} Object;

#endif


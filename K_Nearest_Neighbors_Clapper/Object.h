#ifndef __Object__
#define __Object__

#include "Arduino.h"

typedef struct {
  String type;
  uint16_t frequency1; // Frequency of first clap
  uint16_t frequency2; // Frequency of second clap
  unsigned int voltage1; // Voltage of first clap
  unsigned int voltage2; // Voltage of second clap
  unsigned long delay; // Time between claps (ms)
} Object;


#endif

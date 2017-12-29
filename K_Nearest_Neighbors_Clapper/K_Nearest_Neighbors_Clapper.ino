
//#include <UART_TINY_1MHz_38400.h>
//#include <digitalWriteFast.h>

// Distance Sensor
// Feature is height
// Pre-known object of different heights
// Pattern is see what the object is closest to.
#include <Arduino.h>
// arduino-tone-detector - Version: Latest 
#include <ToneDetector.h>

#include "Object.h"
//#include "CompareObject.h"

// #define IR_SENSOR_PIN_1 6

bool wait = false;

// K Nearest Neighbors
const int K = 3;

const size_t NUM_OF_KNOWN_OBJECTS = 8;
const size_t NUM_OF_OBJECT_TYPES = 2;
// FIXME: Change to sound types
String Object_Types[NUM_OF_OBJECT_TYPES] = {"Acceptable", "Unacceptable"};

Object knownObjects[NUM_OF_KNOWN_OBJECTS];

// Define max height/weight/etc.
// FIXME: For each feature
const int FREQUENCY_MIN = 1000;
const int FREQUENCY_MAX = 2500;
const int VOLTAGE_MIN = 0;
const int VOLTAGE_MAX = 1024;
const int DELAY_MIN = 0;
const int DELAY_MAX = 2000;

// FIXME: Change to the correct trigger voltage
unsigned int trigger_voltage = 800;

// Takes the features from the current object and converts them to strings and integers
// **
void ObjectFeatureExtraction(Object &currObject) {
  // Get the various sound features
  // Call the sound functions and stuff
  unsigned int voltage = analogRead(1);
  uint16_t frequency = readSignal();
  int num_signals = 0;
  unsigned long signal_begin;
  unsigned long signal_end;
  
  while(1) {
    voltage = analogRead(1);
    frequency = readSignal();
    // If the voltage is loud enough to trigger
    if(voltage > trigger_voltage && voltage < 1024) { // 1024 is an error
      // If first time triggered
      if(num_signals == 0) {
        signal_begin = millis();
        num_signals++;
        currObject.frequency1 = frequency;
        currObject.voltage1 = voltage;
      }
      // If second time triggered
      else {
        signal_end = millis();
        num_signals++;
        currObject.frequency2 = frequency;
        currObject.voltage2 = voltage;
        currObject.delay = signal_end - signal_begin;
        return;
      }
    }
  }
}

float RescaleValue(float value, const float min, const float max) {
  return (value-min)/(max-min);
}

// Rescale object features to 1-0 range
void RescaleObject(Object *object) {
  // FIXME: For each feature
  object->frequency1 = RescaleValue(object->frequency1, FREQUENCY_MIN, FREQUENCY_MAX);
  object->frequency2 = RescaleValue(object->frequency2, FREQUENCY_MIN, FREQUENCY_MAX);
  object->voltage1 = RescaleValue(object->voltage1, VOLTAGE_MIN, VOLTAGE_MAX);
  object->voltage2 = RescaleValue(object->voltage2, VOLTAGE_MIN, VOLTAGE_MAX);
  object->delay = RescaleValue(object->delay, DELAY_MIN, DELAY_MAX);
}

// Computes the euclidean distance between the known and the current object's features
float ComputeDistanceofObjects(Object currObject, Object knownObject) {
  // FIXME: For each feature
  float freq1_dist = (currObject.frequency1 - knownObject.frequency1);
  float freq2_dist = (currObject.frequency2 - knownObject.frequency2);
  float volt1_dist = (currObject.voltage1 - knownObject.voltage1);
  float volt2_dist = (currObject.voltage2 - knownObject.voltage2);
  float delay_dist = (currObject.delay - knownObject.delay);
  
  float freq1 = freq1_dist*freq1_dist;
  float freq2 = freq2_dist*freq2_dist;
  float volt1 = volt1_dist*volt1_dist;
  float volt2 = volt1_dist*volt1_dist;
  float delay = delay_dist*delay_dist;
  float dist = freq1 + freq2 + volt1 + volt2 + delay;
  
  return dist;
}

// Finds the patterns from the current object and compares them to patterns from the given objects
// **
String ObjectPatternRecognition(Object currObject, Object knownObjects[]) {
  int count = 0;
  int max_count = 0;
  String most_frequent_type;

  Object kNearestObjects[K];
 
  // First K known objects are the closest K, obviously, so just fill array
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].type = knownObjects[i].type;
    kNearestObjects[i].frequency1 = knownObjects[i].frequency1;
    kNearestObjects[i].frequency2 = knownObjects[i].frequency2;
    kNearestObjects[i].voltage1 = knownObjects[i].voltage1;
    kNearestObjects[i].voltage2 = knownObjects[i].voltage2;
    kNearestObjects[i].delay = knownObjects[i].delay;
  }
  
  // Now, determine if each remaining object is among the K closest, and if so insert into array (thus dropping one obj)
  for(int i = K; i < NUM_OF_KNOWN_OBJECTS; ++i) { // For each remaining known object
    int j;

    // Find the current max difference in the K nearest objects
    float max_diff_of_K_nearest = 0;
    int max_index = 0;
    float temp_dist = 0;

    for(j = 0; j < K; ++j) {
      // kNearest Objects is the difference between the currObject and the knownObjects
      // ComputeDistanceofObjects should compute the total distance for kNearestObjects[j]
      // May just add this to the above
      temp_dist = ComputeDistanceofObjects(currObject, kNearestObjects[j]);

      if(temp_dist > max_diff_of_K_nearest) { // Update max
        max_diff_of_K_nearest = temp_dist;
        max_index = j;
      }
    }

    // If the current known object's difference < the max difference in the current K nearest neighbors
    temp_dist = ComputeDistanceofObjects(currObject, knownObjects[i]);

    if(temp_dist < max_diff_of_K_nearest) {
      // Replace the existing neighbor having max_diff_of_K_nearest, by the current known object, in the K nearest neighbors array
      kNearestObjects[max_index].type = knownObjects[i].type;
      kNearestObjects[i].frequency1 = knownObjects[i].frequency1;
      kNearestObjects[i].frequency2 = knownObjects[i].frequency2;
      kNearestObjects[i].voltage1 = knownObjects[i].voltage1;
      kNearestObjects[i].voltage2 = knownObjects[i].voltage2;
      kNearestObjects[i].delay = knownObjects[i].delay;
    }
  }

  // For testing purposes
  for(int i = 0; i < K; ++i) {
    Serial.println(kNearestObjects[i].type);
    Serial.println(kNearestObjects[i].frequency1);
    Serial.println(kNearestObjects[i].frequency2);
    Serial.println(kNearestObjects[i].voltage1);
    Serial.println(kNearestObjects[i].voltage2);
    Serial.println(kNearestObjects[i].delay);
  }

  // Find out which object type occurs most frequently
  for(int i = 0; i < NUM_OF_OBJECT_TYPES; ++i) {
    int j;
    count = 0;
    for(j = 0; j < K; ++j) {
      if(kNearestObjects[j].type == Object_Types[i]) {
        count++;
      }
    }
    if(count > max_count) {
      max_count = count;
      most_frequent_type = Object_Types[i];
    }
  }
  
  return most_frequent_type;
}

// Prints the type of object that is being passed through the project
// **
void Actuation(String object) {
  if(object != "") {
    Serial.print("\t");
    Serial.println(object);
  }
}

void PopulateKnownObjects() {
  // Change values and measure
  knownObjects[0].type = "Acceptable";
  knownObjects[0].frequency1 = 200;
  knownObjects[0].frequency2 = 200;
  knownObjects[0].voltage1 = 200;
  knownObjects[0].voltage2 = 200;
  knownObjects[0].delay = 200;
  
  knownObjects[1].type = "Unacceptable";
  knownObjects[1].frequency1 = 200;
  knownObjects[1].frequency2 = 200;
  knownObjects[1].voltage1 = 200;
  knownObjects[1].voltage2 = 200;
  knownObjects[1].delay = 200;
}

void setup() {
  Serial.begin(9600);
  
  PopulateKnownObjects();
  
  // May need to use the below for this case
  //Serial.begin(115200);
  
  // Microphone function
  setToneDetectorControlDefaults();
  // Tone range
  setToneDetectorMatchValues(1000, 2500);
}

void loop() {
  String closestObject;
  
  Object currObject;
  
  ObjectFeatureExtraction(currObject);

  closestObject = ObjectPatternRecognition(currObject, knownObjects);

  Actuation(closestObject);

  delay(1000);
  
}

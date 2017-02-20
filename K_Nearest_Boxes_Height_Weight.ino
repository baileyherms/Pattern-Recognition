//Distance Sensor
//Feature is height
//Pre-known object of different heights
//Pattern is see what the object is closest to.
#include <Arduino.h>
#include <NewPing.h>
#include <HX711.h>

#include <math.h>

#include "Object.h"

// Distance Sensor (Height)
#define TRIGGER_PIN 12
#define ECHO_PIN 11

// Weight Sensor
#define DOUT 3
#define CLK 2

const int DISTANCE_SENSOR_MAX_DISTANCE = 50;

// HX711 Weight Sensor
// Use HX711_Calibration sketch to define
const float calibration_factor = 0;

// K Nearest Neighbors
const int K = 3;

const size_t NUM_OF_KNOWN_OBJECTS = 9;
const size_t NUM_OF_OBJECT_TYPES = 3;
String Known_Object_Types[3] = {"Small", "Medium", "Large"};

NewPing sonar(TRIGGER_PIN, ECHO_PIN, DISTANCE_SENSOR_MAX_DISTANCE);
HX711 scale(DOUT, CLK);

float PythagoreanTheorem(int a, float b) {
  float c;
  c = a*a + b*b;
  c = sqrt(c);
  // Nearest 100ths place
  c = roundf(c * 100) / 100;
  
  return c;
}

Object ObjectFeatureExtraction() {
  // Pass in an object (Pass by pointer) (Maybe?)
  Object currObject;
  int currDistance;
  
  // Get height feature (in cm)
  currDistance = sonar.ping_cm();
  currObject.height = DISTANCE_SENSOR_MAX_DISTANCE - currDistance;

  // Get weight feature (in lbs) to the nearest 100ths place
  currObject.weight = roundf(scale.get_units() * 100) / 100;
  
  return currObject; 
}

String PatternRecognition(Object currObject, Object knownObjects[]) {   // FIXME: Efficiency of the array passing??
  
  Object kNearestObjects[K];
  
  Serial.print("Object height: ");
  Serial.println(currObject.height);
  Serial.print("Object weight: ");
  Serial.println(currObject.weight);

  // First K known objects are the closest K, obviously, so just fill array
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].height = abs(knownObjects[i].height - currObject.height);
    kNearestObjects[i].weight = abs(knownObjects[i].weight - currObject.weight);
    kNearestObjects[i].type = knownObjects[i].type;
  }  
  
  // Now, determine if each remaining object is among the K closest, and if so insert into array (thus dropping one obj)
  for(int i = K; i < NUM_OF_KNOWN_OBJECTS; ++i) { // For each remaining known object
    
    // Find the current max difference in the K nearest objects
    float max_diff = 0;
    int max_index = 0;
    int temp_height = 0;
    float temp_weight = 0;
    float temp_pythag = 0;

    for(int j = 0; j < K; ++j) {
      temp_height = kNearestObjects[j].height;
      temp_weight = kNearestObjects[j].weight;
      temp_pythag = PythagoreanTheorem(temp_height, temp_weight);
      
      if(temp_pythag > max_diff) { 
        max_diff = temp_pythag;
        max_index = j;
      }
    }
    
    // If the current known object's difference < the max difference in the current K nearest neighbors
    temp_height = abs(knownObjects[i].height - currObject.height);
    temp_weight = abs(knownObjects[i].weight - currObject.weight);
    temp_pythag = PythagoreanTheorem(temp_height, temp_weight);
    if(temp_pythag < max_diff) {
      // Replace the existing neighbor having max_diff, by the current known object, in the K nearest neighbors array
      kNearestObjects[max_index].type = knownObjects[i].type;
      kNearestObjects[max_index].height = temp_height;
      kNearestObjects[max_index].weight = temp_weight;
    }
  }

  // For testing purposes
  for(int i = 0; i < K; ++i) {
    Serial.println(kNearestObjects[i].type);
    Serial.println(kNearestObjects[i].height);
    Serial.println(kNearestObjects[i].weight);
  }

  int count = 0;
  int max_count = 0;
  String max_type;
  // Find out which object type occurs the most
  for(int i = 0; i < NUM_OF_OBJECT_TYPES; ++i) {
    count = 0;
    for(int j = 0; j < K; ++j) {
      if(kNearestObjects[j].type == Known_Object_Types[i]) {
        count++;
      }
    }
    if(count > max_count) {
      max_count = count;
      max_type = Known_Object_Types[i];
    }
  }

  return max_type;
}

void Actuation(String object) {
  if(object != "") {
    Serial.print("\t");
    Serial.println(object);
  }
}

void setup() {
  Serial.begin(9600);

  // Weight sensor, found with calibration sketch
  scale.set_scale(calibration_factor);
  //Resets scale to 0
  scale.tare();
}

void loop() {
  // Turns both LEDs off before each reading

  // Should put feature extraction and pattern recognition in one multi-second for loop to
    //give the box time to go through the sensors.

  // {Type, height (cm.), weight (lbs.)}
  Object knownObjects[NUM_OF_KNOWN_OBJECTS] = { {"Small", 11, 1.2},
                                                {"Small", 15, 0.8},
                                                {"Small", 12, 1.6},
                                                {"Medium", 27, 2.5},
                                                {"Medium", 29, 3.2},
                                                {"Medium", 30, 4.1},
                                                {"Large", 40, 4.6},
                                                {"Large", 42, 4.2},
                                                {"Large", 42, 5.3}};
    
  Object currObject; 
  String closestObject;
    
  currObject = ObjectFeatureExtraction();

  closestObject = PatternRecognition(currObject, knownObjects);

  Actuation(closestObject);

  delay(1000);
}


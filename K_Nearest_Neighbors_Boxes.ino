//Distance Sensor
//Feature is height
//Pre-known object of different heights
//Pattern is see what the object is closest to.
#include <Arduino.h>
#include <NewPing.h>

#include "Object.h"

#define TRIGGER_PIN 12
#define ECHO_PIN 11

const int DISTANCE_SENSOR_MAX_DISTANCE = 50;

//K Nearest Neighbors
const int K = 3;

const size_t NUM_OF_KNOWN_OBJECTS = 9;
const size_t NUM_OF_OBJECT_TYPES = 3;
String Known_Object_Types[3] = {"Small", "Medium", "Large"};

NewPing sonar(TRIGGER_PIN, ECHO_PIN, DISTANCE_SENSOR_MAX_DISTANCE);

Object ObjectFeatureExtraction() {
  //Pass in an object (Pass by pointer) (Maybe?)
  Object currObject;
  int currDistance;
  
  //Get height feature
  currDistance = sonar.ping_cm();
  
  currObject.height = DISTANCE_SENSOR_MAX_DISTANCE - currDistance;
  
  return currObject; 
}


String PatternRecognition(Object currObject, Object knownObjects[]) {   // FIXME: Efficiency of the array passing??
  
  Object kNearestObjects[K];
  
  Serial.print("Object height: ");
  Serial.println(currObject.height);

 
  // First K known objects are the closest K, obviously, so just fill array
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].height = abs(knownObjects[i].height - currObject.height);
    kNearestObjects[i].type = knownObjects[i].type;
  }  
  
  
  // Now, determine if each remaining object is among the K closest, and if so insert into array (thus dropping one obj)
  for(int i = K; i < NUM_OF_KNOWN_OBJECTS; ++i) { // For each remaining known object
  
    // Find the current max difference in the K nearest objects
    int max_diff = 0;
    int max_index = 0;
    for(int j = 0; j < K; ++j) {
      if(kNearestObjects[j].height > max_diff) { 
        max_diff = kNearestObjects[j].height;
        max_index = j;
      }
    }
    
    // If the current known object's difference < the max difference in the current K nearest neighbors
    if(abs(knownObjects[i].height - currObject.height) < max_diff) {
      // Replace the existing neighbor having max_diff, by the current known object, in the K nearest neighbors array
      kNearestObjects[max_index].type = knownObjects[i].type;
      kNearestObjects[max_index].height = abs(knownObjects[i].height - currObject.height);
    }
  }

  // For testing purposes
  for(int i = 0; i < K; ++i) {
    Serial.println(kNearestObjects[i].type);
    Serial.println(kNearestObjects[i].height);
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
  
}

void loop() {
  //Turns both LEDs off before each reading

  //Should put feature extraction and pattern recognition in one multi-second for loop to
    //give the box time to go through the sensors.

  Object knownObjects[NUM_OF_KNOWN_OBJECTS] = {{"Small", 11},
                        {"Small", 15},
                        {"Small", 12},
                        {"Medium", 27},
                        {"Medium", 29},
                        {"Medium", 30},
                        {"Large", 40},
                        {"Large", 42},
                        {"Large", 42}};
    
  Object currObject; 
  String closestObject;
    
  currObject = ObjectFeatureExtraction();

  closestObject = PatternRecognition(currObject, knownObjects);

  Actuation(closestObject);

  delay(1000);
}

//Distance Sensor
//Feature is height
//Pre-known object of different heights
//Pattern is see what the object is closest to.
#include <Arduino.h>
#include <NewPing.h>

#include "Known_Objects.h"

#define TRIGGER_PIN 12
#define ECHO_PIN 11

// MAKE THESE CONST
const int DISTANCE_SENSOR_MAX_DISTANCE = 50;

//K Nearest Neighbors
const int K = 3;

const size_t NUM_OF_KNOWN_OBJECTS = 9;
const size_t NUM_OF_OBJECT_TYPES = 3;
String Known_Types[3] = {"Small", "Medium", "Large"};



NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);


Object ObjectFeatureExtraction() {
  Object currObject;
  int currDistance;
  
  //Get height feature
  currDistance = sonar.ping_cm();
  
  currObject.height = DISTANCE_SENSOR_MAX_DISTANCE - currDistance;
  
  return currObject; 
}

String PatternRecognition(Object currObject, Object knownObjects[]) {   // FIXME: Efficiency of the array passing??

  Object knownObjectsDifferences[9];
  
  Serial.print("Object height: ");
  Serial.println(currObject.height);

  // k nearest neighbors

  // Checks the min difference between two points
  int min_difference = 100; // FIXME needed?

  // Find differences between current object's distance and known object's distances and store in difference array
  for(int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) {
    knownObjectsDifferences[i].type   = knownObjects[i].type;
    knownObjectsDifferences[i].height = abs(knownObjects[i].height - currObject.height); // Record height difference
  }

/*  String temp_type;
  int temp_height;
  // Sort the difference array using bubble sort. FIXME FUTURE: Use qsort in stdlib
  for(int i = 0; i < NUM_OF_KNOWN; ++i) {
    for(int j = 0; j < NUM_OF_KNOWN - i - 1; ++j) {
      if(knownObjectsDifferences[j].height > knownObjectsDifferences[j+1].height) {
        temp_type = knownObjectsDifferences[j].type;
        temp_height = knownObjectsDifferences[j].height;
        knownObjectsDifferences[j].type = knownObjectsDifferences[j+1].type;
        knownObjectsDifferences[j].height = knownObjectsDifferences[j+1].height;
        knownObjectsDifferences[j+1].type = temp_type;
        knownObjectsDifferences[j+1].height = temp_height;
      }
    }
  }
  
*/ 

  //Create new array holding only K elements
  Object kNearestObjects[K];
  //FIXME
/*  
  for (int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) {
     if ((i < K) || (knownObjectsDifferences[i].height < kNearestObjects[K-1].height)) {
        // Insert knownObjectsDifferences[i] into kNearestObjects, replace the new value with the largest             value in the array
        
        
  }
*/ 
  
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].type = diff_Boxes[i].type;
    kNearestObjects[i].height = diff_Boxes[i].height;
  }

  // Loop through the K nearest objects, and count each object type
  for (int i = 0; i < NUM_OF_TYPES; ++i) {


  int count = 0;
  int max_count = 0;
  String max_type;
  //Find out which object type occurs most frequently
  for(int i = 0; i < NUM_OF_OBJECT_TYPES; ++i) {
    for(int j = 0; j < K; ++j) {
      if(kNearestObjects[j].type == known_Types[i]) {
        count++;
      }
    }
    if(count > max_count) {
      max_count = count;
      max_type = Known_Types[i];
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

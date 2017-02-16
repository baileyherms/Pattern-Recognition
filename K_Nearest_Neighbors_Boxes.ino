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
String Known_Types[3] = {"Small", "Medium", "Large"};

NewPing sonar(TRIGGER_PIN, ECHO_PIN, DISTANCE_SENSOR_MAX_DISTANCE);

Object ObjectFeatureExtraction() {
  Object currObject;
  int currDistance;
  
  //Get height feature
  currDistance = sonar.ping_cm();
  
  currObject.height = DISTANCE_SENSOR_MAX_DISTANCE - currDistance;
  
  return currObject; 
}

String PatternRecognition(Object currObject, Object knownObjects[]) {   // FIXME: Efficiency of the array passing??

  //Object knownObjectsDifferences[9];
  
  Serial.print("Object height: ");
  Serial.println(currObject.height);

  // k nearest neighbors

  // Checks the min difference between two points
  /*
  int min_difference = 100; // FIXME needed?

  // Find differences between current object's distance and known object's distances and store in difference array
  for(int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) {
    knownObjectsDifferences[i].type   = knownObjects[i].type;
    knownObjectsDifferences[i].height = abs(knownObjects[i].height - currObject.height); // Record height difference
  }
*/
  
  /*
  for(int i = 0; i < NUM_OF_KNOWN; ++i) {
    for(int j = 0; j < NUM_OF_KNOWN - i - 1; ++j) {
      if(Boxes[j].height > Boxes[j+1].height) {
        temp_type = Boxes[j].type;
        temp_height = Boxes[j].height;
        Boxes[j].type = Boxes[j+1].type;
        Boxes[j].height = Boxes[j+1].height;
        Boxes[j+1].type = temp_type;
        Boxes[j+1].height = temp_height;
      }
    }
  }
  */
  
  //MOST NEW CODE STARTS HERE
  //Find differences between distance and known box sizes and store in difference array
  /*
  int diff_pos = 0;
  for(int j = 0; j < K; ++j) {
    for(int i = 0; i < NUM_OF_KNOWN; ++i) {
      
      if(abs(Boxes[i].height - features.height) <= min_difference) {
        //min_difference = abs(Boxes[i].height - features.height);
        //pos_min_difference[0] = i;
        diff_Boxes[diff_pos].type = Boxes[i].type;
        diff_Boxes[diff_pos].height = abs(Boxes[i].height - features.height);
        min_difference = abs(Boxes[i].height - features.height);
      }
      
      //diff_Boxes[i].type = Boxes[i].type;
      //diff_Boxes[i].height = abs(Boxes[i].height - features.height);
    }
    diff_pos++;
  }
  */

  Object kNearestObjects[K];
  
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].height = abs(knownObjects[i].height - currObject.height);
    kNearestObjects[i].type = knownObjects[i].type;
  }
  for(int i = K; i < NUM_OF_KNOWN_OBJECTS; ++i) {
    int max_diff = 0;
    int max_index = 0;
    for(int j = 0; j < K; ++j) {
      if(kNearestObjects[j].height > max_diff) {
        max_diff = kNearestObjects[j].height;
        max_index = j;
      }
    }
    if(abs(knownObjects[i].height - currObject.height) < max_diff) {
      kNearestObjects[max_index].type = knownObjects[i].type;
      kNearestObjects[max_index].height = abs(knownObjects[i].height - currObject.height);
    }
  }

  for(int i = 0; i < K; ++i) {
    Serial.println(kNearestObjects[i].type);
    Serial.println(kNearestObjects[i].height);
  }

  /*
  String temp_type;
  int temp_height;
  //Sort the difference array
  for(int i = 0; i < NUM_OF_KNOWN; ++i) {
    for(int j = 0; j < NUM_OF_KNOWN - i - 1; ++j) {
      if(diff_Boxes[j].height > diff_Boxes[j+1].height) {
        temp_type = diff_Boxes[j].type;
        temp_height = diff_Boxes[j].height;
        diff_Boxes[j].type = diff_Boxes[j+1].type;
        diff_Boxes[j].height = diff_Boxes[j+1].height;
        diff_Boxes[j+1].type = temp_type;
        diff_Boxes[j+1].height = temp_height;
      }
    }
  }
  */
  /*
  //Create new array holding only K elements
  Known_Objects k_Nearest_Boxes[K];
  for(int i = 0; i < K; ++i) {
    k_Nearest_Boxes[i].type = diff_Boxes[i].type;
    k_Nearest_Boxes[i].height = diff_Boxes[i].height;
  }
  */
  int count = 0;
  int max_count = 0;
  String max_type;
  //Find out which box type occurs the most
  for(int i = 0; i < NUM_OF_OBJECT_TYPES; ++i) {
    count = 0;
    for(int j = 0; j < K; ++j) {
      if(kNearestObjects[j].type == Known_Types[i]) {
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

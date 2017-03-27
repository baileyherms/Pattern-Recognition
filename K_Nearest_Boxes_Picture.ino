//Camera Sensor
//Feature is r, g, b
//Pre-known object of different colors
//Pattern is see what the object is closest to.
#include <Arduino.h>

#include "Object.h"

// For connection to Python
int incomingByte = 0;

const int DISTANCE_SENSOR_MAX_DISTANCE = 50;

//K Nearest Neighbors
const int K = 3;

const size_t NUM_OF_KNOWN_OBJECTS = 9;
const size_t NUM_OF_OBJECT_TYPES = 3;
String Known_Object_Types[3] = {"Small", "Medium", "Large"};

float ColorDistance(Object currObject, Object knownObject) {
  float red = currObject.red - knownObject.red;
  float green = currObject.green - knownObject.green;
  float blue = currObject.blue - knownObject.blue;
  float dist = pow(red, 2) + pow(green, 2) + pow(blue, 2);
  dist = roundf(dist* 100.0) / 100.0;
  return dist;
}

float ComputeDistance(Object currObject, Object knownObject) {
  float colorDistance = ColorDistance(currObject, knownObject);
  float dist = pow(colorDistance, 2);
  dist = sqrt(dist);
  // Nearest 100ths place
  dist = roundf(dist * 100.0) / 100.0;
  
  return dist;
}

Object ObjectFeatureExtraction() {
  // Pass in an object (Pass by pointer)
  Object currObject;
  int currDistance;
  
  // Get Image from camera
  
  // Send Image to Python for processing
  
  // Get average r, g, b values back
  if(Serial.available() > 0) {
    currObject.red = Serial.read();
    currObject.green = Serial.read();
    currObject.blue = Serial.read();
  }
  
  return currObject;
}

String PatternRecognition(Object currObject, Object knownObjects[]) {
  Object kNearestObjects[K];
  
  // For testing purposes
  for(int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) {
    Serial.println(ComputeDistance(currObject, knownObjects[i]));
  }

  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].red = abs(knownObjects[i].red - currObject.red);
    kNearestObjects[i].green = abs(knownObjects[i].green - currObject.green);
    kNearestObjects[i].blue = abs(knownObjects[i].blue - currObject.blue);
    kNearestObjects[i].type = knownObjects[i].type;
  }  
  
  // Now, determine if each remaining object is among the K closest, and if so insert into array (thus dropping one obj)
  for(int i = K; i < NUM_OF_KNOWN_OBJECTS; ++i) { // For each remaining known object
    
    // Find the current max difference (CHANGE TO DISTANCE, EVERYWHERE) in the K nearest objects
    float max_diff = 0;
    int max_index = 0;
    float temp_red = 0;
    float temp_green = 0;
    float temp_blue = 0;
    float temp_dist = 0;

    for(int j = 0; j < K; ++j) {
      /*
      temp_height = kNearestObjects[j].height;
      temp_weight = kNearestObjects[j].weight;
      temp_red = kNearestObjects[j].red;
      temp_green = kNearestObjects[j].green;
      temp_blue = kNearestObjects[j].blue;
      */
      //temp_dist = ComputeDistance(temp_height, temp_weight);
      
      //ISSUE MIGHT BE HERE
      temp_dist = ComputeDistance(currObject, kNearestObjects[j]);
      
      if(temp_dist > max_diff) { // Update max
        max_diff = temp_dist;
        max_index = j;
      }
    }
    
    // If the current known object's difference < the max difference in the current K nearest neighbors
    temp_red = abs(knownObjects[i].red - currObject.red);
    temp_green = abs(knownObjects[i].green - currObject.green);
    temp_blue = abs(knownObjects[i].blue - currObject.blue);
    //temp_dist = ComputeDistance(temp_height, temp_weight);
    temp_dist = ComputeDistance(currObject, knownObjects[i]);
    
    if(temp_dist < max_diff) {
      // Replace the existing neighbor having max_diff, by the current known object, in the K nearest neighbors array
      kNearestObjects[max_index].type = knownObjects[i].type;
      kNearestObjects[max_index].red = temp_red;
      kNearestObjects[max_index].green = temp_green;
      kNearestObjects[max_index].blue = temp_blue;
      max_diff = temp_dist;
    }
  }

  // For testing purposes
  /*
  for(int i = 0; i < K; ++i) {
    Serial.println(kNearestObjects[i].type);
    Serial.println(ComputeDistance(currObject, kNearestObjects[i]));
  }
  */

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


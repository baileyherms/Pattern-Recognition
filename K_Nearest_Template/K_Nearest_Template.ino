#include <Arduino.h>
#include <math.h>
// FIXME: Add the libraries needed for each pin

#include "Object.h"

// Define Pins for each sensor
// Distance Sensor (Height)

int test = 0;

// Define anything else needed for sensors

// K Nearest Neighbors
// FIXME: Change when necessary
const int K = [number];

// FIXME: Change when necessary
const size_t NUM_OF_KNOWN_OBJECTS = [number];
const size_t NUM_OF_OBJECT_TYPES = [number];

// FIXME: Change object types
String Known_Object_Types[[number]] = {[object types]};
const int STRUCT_FIELDS = [number];

// Define max height/weight/etc.
// FIXME: For each feature
const int [feature]_MAX = [number];
const int [feature]_MIN = [number];

// Computes the euclidean distance between the known and the current object's rgb values
float ComputeDistanceofColors(Object currObject, Object knownObject) {
  float red = currObject.red - knownObject.red;
  float green = currObject.green - knownObject.green;
  float blue = currObject.blue - knownObject.blue;
  float dist = pow(red, 2) + pow(green, 2) + pow(blue, 2);
  dist = roundf(dist* 100.0) / 100.0;
  return dist;
}

// Computes the euclidean distance between the known and the current object's features
// Can add or remove features
float ComputeDistanceofObjects(Object currObject, Object knownObject) {
  // FIXME: For non-color features
  float [feature] = ((currObject.[feature] - knownObject.[feature]) - [feature]_MIN) / ([feature]_MAX - [feature]_MIN);
  
  // FIXME (optional): For color feature
  float colorDistance = ComputeDistanceofColors(currObject, knownObject);
  
  // FIXME: For each feature
  float dist = pow([feature 1], 2) + pow([feature 2], 2) + pow([feature 3], 2);
  dist = sqrt(dist);
  
  // Nearest 100ths place
  dist = roundf(dist * 100.0) / 100.0;
  
  return dist;
}

// Takes the features from the current object and converts them to strings and integers
void ObjectFeatureExtraction(Object &currObject) {
  int currDistance;
  // Define necessary variables for height/weight/etc.
  
  // Read features from sensors (ex: sonar.ping_cm())
  
  // Assign each of those features to currObject.
}

// Finds the patterns from the current object and compares them to patterns from the given objects
String PatternRecognition(Object currObject, Object knownObjects[], size_t NUM_OF_KNOWN_OBJECTS) {
  
  Object kNearestObjects[K];
  
  // The first K Object differences are added to the kNearestObjects array
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].type = knownObjects[i].type;
    // FIXME: For each feature
    kNearestObjects[i].[feature] = abs(knownObjects[i].[feature] - currObject.[feature]);
  }  
  
  // Now, determine if each remaining object is among the K closest, and if so insert into array (thus dropping one obj)
  for(int i = K; i < NUM_OF_KNOWN_OBJECTS; ++i) { // For each remaining known object
    
    // Find the current max difference (CHANGE TO DISTANCE, EVERYWHERE) in the K nearest objects
    float max_diff = 0;
    int max_index = 0;
    // FIXME: For each feature
    int temp_[feature] = 0;

    for(int j = 0; j < K; ++j) {
      temp_dist = ComputeDistanceofObjects(currObject, kNearestObjects[j]);
      
      if(temp_dist > max_diff) { // Update max
        max_diff = temp_dist;
        max_index = j;
      }
    }
    
    // If the current known object's difference < the max difference in the current K nearest neighbors
    // For each feature
    temp_[feature] = abs(knownObjects[i].[feature] - currObject.[feature]);
    temp_dist = ComputeDistanceofObjects(currObject, knownObjects[i]);
    
    if(temp_dist < max_diff) {
      // Replace the existing neighbor having max_diff, by the current known object, in the K nearest neighbors array
      kNearestObjects[max_index].type = knownObjects[i].type;
      // FIXME: For each feature
      kNearestObjects[max_index].[feature] = temp_[feature];
      max_diff = temp_dist;
    }
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

// Prints the type of object that is being passed through the project
void Actuation(String object) {
  if(object != "") {
    Serial.print("\t");
    Serial.println(object);
  }
}

void setup() {
  Serial.begin(9600);

  // Setup any features that need it
}

void loop() {
  // Turns both LEDs off before each reading

  // Should put feature extraction and pattern recognition in one multi-second for loop to
    //give the box time to go through the sensors.
  // FIXME: Change number if needed
  const size_t NUM_OF_KNOWN_OBJECTS = 9;
  
  // FIXME: Add known objects
  Object knownObjects[NUM_OF_KNOWN_OBJECTS] = { {"[type]", [Put features in here]]},
                                                ...};
                                                
  // FIXME: Change number if needed
  const size_t NUM_OF_TEST_OBJECTS = 9;
  
  // FIXME: Add test objects
  Object testObjects[NUM_OF_KNOWN_OBJECTS] = { {"", [Put features in here]]},
                                                ...};
  
  Object currObject; 
  String closestObject;
    
  ObjectFeatureExtraction(currObject);

  closestObject = PatternRecognition(currObject, knownObjects, NUM_OF_KNOWN_OBJECTS);

  Actuation(closestObject);
  
  delay(1000);
}


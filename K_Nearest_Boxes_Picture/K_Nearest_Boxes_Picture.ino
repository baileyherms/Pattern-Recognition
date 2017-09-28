//Camera Sensor
//Feature is r, g, b
//Pre-known object of different colors
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

// Takes the features from the current object and converts them to strings and integers
// **
void ObjectFeatureExtraction(Object &currObject) {
  // Pass in an object (Pass by pointer)
  int currDistance;
  
  // Get Image from camera
  
  // Send Image to Python for processing
  
  // Get average r, g, b values back
  if(Serial.available() > 0) {
    currObject.red = Serial.read();
    currObject.green = Serial.read();
    currObject.blue = Serial.read();
  }
}

//  Computes the euclidean distance between the known and the current object's rgb values
float ComputeDistanceofColors(Object currObject, Object knownObject) {
  float red = currObject.red - knownObject.red;
  float green = currObject.green - knownObject.green;
  float blue = currObject.blue - knownObject.blue;
  float dist = pow(red, 2) + pow(green, 2) + pow(blue, 2);
  dist = roundf(dist* 100.0) / 100.0;
  return dist;
}

//  Computes the euclidean distance between the known and the current object's features
float ComputeDistanceofObjects(Object currObject, Object knownObject) {
  // Compute color distances
  float ColorDistance = ComputeDistanceofColors(currObject, knownObject);
  // Square all the the feature distances between the known and current object (necessary when there are more features)
  float dist = pow(ColorDistance, 2);
  // Take the square root of all the distances
  dist = sqrt(dist);
  // Nearest 100ths place
  dist = roundf(dist * 100.0) / 100.0;
  
  return dist;
}

// Finds the patterns from the current object and compares them to patterns from the given objects
// **
String ObjectPatternRecognition(Object currObject, Object knownObjects[]) {
  Object kNearestObjects[K];
  
  // For testing purposes
  for(int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) {
    Serial.println(ComputeDistanceofObjects(currObject, knownObjects[i]));
  }

  // The first K Object differences are added to the kNearestObjects array
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].red = abs(knownObjects[i].red - currObject.red);
    kNearestObjects[i].green = abs(knownObjects[i].green - currObject.green);
    kNearestObjects[i].blue = abs(knownObjects[i].blue - currObject.blue);
    kNearestObjects[i].type = knownObjects[i].type;
  }  
  
  // Now, determine if each remaining object is among the K closest, and if so insert into array (thus dropping one obj)
  for(int i = K; i < NUM_OF_KNOWN_OBJECTS; ++i) { // For each remaining known object
    
    // Find the current max difference in the K nearest objects
    float max_dist = 0;
    int max_index = 0;
    float temp_red = 0;
    float temp_green = 0;
    float temp_blue = 0;
    float temp_dist = 0;

    for(int j = 0; j < K; ++j) {
      //temp_dist = ComputeDistanceofObjects(currObject, kNearestObjects[j]);
      temp_dist = ComputeDistanceofObjects(currObject, knownObjects[j]);
      if(temp_dist > max_dist) { // Update max
        max_dist = temp_dist;
        max_index = j;
      }
    }
    
    // If the current known object's difference < the max difference in the current K nearest neighbors
    temp_red = abs(knownObjects[i].red - currObject.red);
    temp_green = abs(knownObjects[i].green - currObject.green);
    temp_blue = abs(knownObjects[i].blue - currObject.blue);
    temp_dist = ComputeDistanceofObjects(currObject, knownObjects[i]);
    
    if(temp_dist < max_dist) {
      // Replace the existing neighbor having max_dist, by the current known object, in the K nearest neighbors array
      kNearestObjects[max_index].type = knownObjects[i].type;
      kNearestObjects[max_index].red = temp_red;
      kNearestObjects[max_index].green = temp_green;
      kNearestObjects[max_index].blue = temp_blue;
      max_dist = temp_dist;
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
// **
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

  Object knownObjects[NUM_OF_KNOWN_OBJECTS] =
                        {{"Orange", 222, 186, 43},
                        {"Orange", 247, 174, 47},
                        {"Orange", 233, 151, 9},
                        {"Apple", 233, 61, 9},
                        {"Apple", 215, 26, 26},
                        {"Apple", 225, 20, 20}};
    
  Object currObject; 
  String closestObject;
    
  ObjectFeatureExtraction(currObject);

  closestObject = ObjectPatternRecognition(currObject, knownObjects);

  Actuation(closestObject);

  delay(1000);
}


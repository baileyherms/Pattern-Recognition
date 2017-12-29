#include <Arduino.h>
#include <math.h>
// FIXME: Add the libraries needed for each pin

#include "Object.h"

// FIXME: Can change pin number
#define IR_SENSOR_PIN_1 6

// Define Pins for each sensor
// Distance Sensor (Height)

int test = 0;
bool wait = false;

// Define anything else needed for sensors

// K Nearest Neighbors
// FIXME: Change when necessary
const int K = [number];

// FIXME: Change when necessary
const size_t NUM_OF_KNOWN_OBJECTS = [number];
const size_t NUM_OF_OBJECT_TYPES = [number];

// FIXME: Change object types
String Known_Object_Types[NUM_OF_OBJECT_TYPES] = {[object types]};

Object knownObjects[NUM_OF_KNOWN_OBJECTS];

// Define max height/weight/etc.
// FIXME: For each feature
const int [feature]_MIN = [number];
const int [feature]_MAX = [number];

// Takes the features from the current object and converts them to strings and integers
// **
void ObjectFeatureExtraction(Object &currObject) {
  // Define necessary variables for height/weight/etc.
  
  // Read features from sensors (ex: sonar.ping_cm())
  
  // Assign each of those features to currObject.
}

float RescaleValue(float value, const float min, const float max) {
  return (value-min)/(max-min);
}

// Rescale object features to 1-0 range
void RescaleObject(Object *object) {
  // FIXME: For each feature
  object->[feature] = RescaleValue(object->[feature], [feature]_MIN, [feature]_MAX);
}

//  Computes the euclidean distance between the known and the current object's rgb values
// FIXME: Remove if there are no colors
float ComputeDistanceofColors(Object currObject, Object knownObject) {
  float red = currObject.r - knownObject.r;
  float green = currObject.g - knownObject.g;
  float blue = currObject.b - knownObject.b;
  float dist = pow(red, 2) + pow(green, 2) + pow(blue, 2);
  dist = roundf(dist* 100.0) / 100.0;
  return dist;
}

// Computes the euclidean distance between the known and the current object's features
float ComputeDistanceofObjects(Object currObject, Object knownObject) {
  // FIXME: For each feature
  float [feature]_dist = (currObject.[feature] - knownObject.[feature]);
  // FIXME: Remove if there are no colors
  float color_dist = ComputeDistanceofColors(currObject, knownObject);
  // FIXME: For each feature, remove color_dist if there are no colors
  float dist = [feature]_dist*[feature]_dist + color_dist*color_dist;
  
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
    // FIXME: For each feature
    kNearestObjects[i].[feature] = knownObjects[i].[feature];
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
      // FIXME: For each feature
      kNearestObjects[max_index].[feature] = knownObjects[i].[feature];
    }
  }

  // For testing purposes
  for(int i = 0; i < K; ++i) {
    Serial.println(kNearestObjects[i].type);
    // FIXME: For each feature
    Serial.println(kNearestObjects[i].[feature]);
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
  // FIXME: For each feature
  // FIXME: Add as many known objects as needed
  knownObjects[0].type = "[Object type]";
  knownObjects[0].[feature] = [feature value];
}


void setup() {
  Serial.begin(9600);
  
  PopulateKnownObjects();
  
  pinMode(IR_SENSOR_PIN_1, INPUT);
  digitalWrite(IR_SENSOR_PIN_1, HIGH);
  
  // Setup any features that need it
}

void loop() {
  bool IR_Sensor1;
  String closestObject;
  
  IR_Sensor1 = digitalRead(IR_SENSOR_PIN_1);
  
  // Sensor was tripped and wasn't right before.
  if(IR_Sensor1 == LOW && !wait) {
    // FIXME: Can change what the object is (ex: Box, Fruit, etc.)
    Serial.println("[Object] Detected");
    
    delay(1000);
    
    digitalWrite(LED_BUILTIN, HIGH);
    
    Object currObject;
    
    ObjectFeatureExtraction(currObject);
  
    closestObject = ObjectPatternRecognition(currObject, knownObjects);
  
    Actuation(closestObject);
    
    wait = true;
  
    delay(1000);
  }
  // Sensor was tripped and was right before.
  else if(IR_Sensor1 == LOW && wait) {
    digitalWrite(LED_BUILTIN, LOW);
  }
  // Sensor was not tripped.
  else if(IR_Sensor1 == HIGH){
    digitalWrite(LED_BUILTIN, LOW);
    wait = false;
  }
  
}

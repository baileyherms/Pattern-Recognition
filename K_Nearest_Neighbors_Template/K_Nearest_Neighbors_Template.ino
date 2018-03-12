#include <Arduino.h>
#include <math.h>
// FIXME: Add the libraries needed for each pin
#include <NewPing.h>

#include "Object.h"

// Define Pins for each sensor
// Distance Sensor (Height)

// Height
#define TRIGGER_PIN_1 12
#define ECHO_PIN_1 11

// Define anything else needed for sensors
// IR Sensor
#define IR_SENSOR_PIN_1 6

// K Nearest Neighbors
// FIXME: Change when necessary
const int K = 3;

// FIXME: Change when necessary
const size_t NUM_OF_KNOWN_OBJECTS = 9;
const size_t NUM_OF_OBJECT_TYPES = 3;

Object knownObjects[NUM_OF_KNOWN_OBJECTS];

// FIXME: Change object types
String Known_Object_Types[NUM_OF_OBJECT_TYPES] = {"Small", "Medium", "Large"};

// Define max height/weight/etc.
// FIXME: For each feature
// const int [feature]_MAX = [number];
// const int [feature]_MIN = [number];
const int DISTANCE_SENSOR_MAX_HEIGHT = 39;
const int DISTANCE_SENSOR_MIN_HEIGHT = 2;

// FIXME: Setup sensors if necessary
NewPing sonar_height(TRIGGER_PIN_1, ECHO_PIN_1, DISTANCE_SENSOR_MAX_HEIGHT);

// Computes the euclidean distance between the known and the current object's rgb values
// FIXME: Add function in if using a color sensor.
/*
float ComputeDistanceofColors(Object currObject, Object knownObject) {
  float red = currObject.red - knownObject.red;
  float green = currObject.green - knownObject.green;
  float blue = currObject.blue - knownObject.blue;
  float dist = pow(red, 2) + pow(green, 2) + pow(blue, 2);
  dist = roundf(dist* 100.0) / 100.0;
  return dist;
}
*/

// Computes the euclidean distance between the known and the current object's features
// Can add or remove features
float ComputeDistanceofObjects(Object currObject, Object knownObject) {
  // FIXME: For non-color features
  //float [feature] = ((currObject.[feature] - knownObject.[feature]) - [feature]_MIN) / ([feature]_MAX - [feature]_MIN);
  float height = (currObject.height - knownObject.height);
  // FIXME (optional): For color feature
  //float colorDistance = ComputeDistanceofColors(currObject, knownObject);
  
  // FIXME: For each feature
  //float dist = pow([feature 1], 2) + pow([feature 2], 2) + pow([feature 3], 2);
  float dist = height*height;
  dist = sqrt(dist);
  
  // Nearest 100ths place
  dist = roundf(dist * 100.0) / 100.0;
  
  return dist;
}

// Takes the features from the current object and converts them to strings and integers
void ObjectFeatureExtraction(Object &currObject) {
  int currDistance;
  // Define necessary variables for height/weight/etc.
  int currHeight;
  
  // Read features from sensors (ex: sonar.ping_cm())
  currHeight = sonar_height.ping_cm();
  
  // Assign each of those features to currObject.
  currObject.height = DISTANCE_SENSOR_MAX_HEIGHT - currHeight;
}

// Finds the patterns from the current object and compares them to patterns from the given objects
String ObjectPatternRecognition(Object currObject, Object knownObjects[]) {
  Object kNearestObjects[K];
  int count = 0;
  int max_count = 0;
  String most_frequent_type;
  
  // The first K Object differences are added to the kNearestObjects array
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].type = knownObjects[i].type;
    // FIXME: For each feature
    //kNearestObjects[i].[feature] = abs(knownObjects[i].[feature] - currObject.[feature]);
    kNearestObjects[i].height = knownObjects[i].height;
  }  
  
  // Now, determine if each remaining object is among the K closest, and if so insert into array (thus dropping one obj)
  for(int i = K; i < NUM_OF_KNOWN_OBJECTS; ++i) { // For each remaining known object
    
    // Find the current max difference (CHANGE TO DISTANCE, EVERYWHERE) in the K nearest objects
    float max_diff_of_K_nearest = 0;
    int max_index = 0;
    int temp_dist = 0;

    for(int j = 0; j < K; ++j) {
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
      //kNearestObjects[max_index].[feature] = temp_[feature];
      kNearestObjects[max_index].height = knownObjects[i].height;
      max_diff_of_K_nearest = temp_dist;
    }
  }

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
      most_frequent_type = Known_Object_Types[i];
    }
  }

  return most_frequent_type;
}

// Prints the type of object that is being passed through the project
void Actuation(String object) {
  if(object != "") {
    Serial.print("\t");
    Serial.println(object);
  }
}

void PopulateKnownObjects() {
  // FIXME: Change for every test object.
  /* 
  knownObjects[#].type = [type];
  knownObjects[#].height = [height];
  */
  knownObjects[0].type = "Small";
  knownObjects[0].height = 8.0;
}

void setup() {
  PopulateKnownObjects();

  // FIXME: Setup any features that need it
  pinMode(TRIGGER_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(IR_SENSOR_PIN_1, INPUT);
  digitalWrite(IR_SENSOR_PIN_1, HIGH);
  
  Serial.begin(9600);
}

void loop() {
  String closestObject;

  // FIXME: Change how object is detected.
  bool IR_Sensor1;
  bool wait = false;
  IR_Sensor1 = digitalRead(IR_SENSOR_PIN_1);
  
  // Sensor was tripped and wasn't right before.
  if(IR_Sensor1 == LOW && !wait) {
    Serial.println("Object Detected");
    
    delay(2000);
    
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


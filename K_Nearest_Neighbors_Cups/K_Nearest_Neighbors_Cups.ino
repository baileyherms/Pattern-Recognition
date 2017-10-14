// Distance Sensor
// Feature is height
// Pre-known object of different heights
// Pattern is see what the object is closest to.
#include <Arduino.h>
#include <NewPing.h>
#include <HX711.h>

#include "Object.h"

// Height
#define TRIGGER_PIN_1 12
#define ECHO_PIN_1 11

// Width
#define TRIGGER_PIN_2 10
#define ECHO_PIN_2 9

#define TRIGGER_PIN_3 8
#define ECHO_PIN_3 7

// IR Sensor
#define IR_SENSOR_PIN_1 6

// Weight Sensor
#define DOUT 3
#define CLK 2

bool wait = false;

// Needs to be int for the distance sensor.
//const int DISTANCE_SENSOR_MAX_HEIGHT = 100;
//const int DISTANCE_SENSOR_MAX_WIDTH = 60;
// In centimeters
const int DISTANCE_SENSOR_MAX_HEIGHT = 39;
const int DISTANCE_SENSOR_MAX_WIDTH = 46;

const int DISTANCE_SENSOR_MIN_HEIGHT = 2;
const int DISTANCE_SENSOR_MIN_WIDTH = 1;

const float WEIGHT_MAX = 600.0;
const float WEIGHT_MIN = 100.0;

// K Nearest Neighbors
const int K = 3;

const size_t NUM_OF_KNOWN_OBJECTS = 9;
const size_t NUM_OF_OBJECT_TYPES = 3;
String Known_Object_Types[NUM_OF_OBJECT_TYPES] = {"Travel Mug", "11oz Mug", "16oz Mug"};

Object knownObjects[NUM_OF_KNOWN_OBJECTS];

NewPing sonar_height(TRIGGER_PIN_1, ECHO_PIN_1, DISTANCE_SENSOR_MAX_HEIGHT);
NewPing sonar_width_1(TRIGGER_PIN_2, ECHO_PIN_2, DISTANCE_SENSOR_MAX_WIDTH);
NewPing sonar_width_2(TRIGGER_PIN_3, ECHO_PIN_3, DISTANCE_SENSOR_MAX_WIDTH);
HX711 scale(DOUT, CLK);

// Takes the features from the current object and converts them to strings and integers
// **
void ObjectFeatureExtraction(Object &currObject) {
  // Pass in an object (Pass by pointer)
  int currHeight;
  int currWidth1;
  int currWidth2;
  
  // Get height feature
  int temp;
  delay(50);
  temp =  sonar_height.ping_median(10);
  delay(50);
  currHeight = sonar_height.convert_cm(temp);
  delay(50);
  temp =  sonar_width_1.ping_median(10);
  delay(50);
  currWidth1 = sonar_width_1.convert_cm(temp);
  delay(50);
  temp =  sonar_width_2.ping_median(10);
  currWidth2 = sonar_width_2.convert_cm(temp);
  
  currObject.height = DISTANCE_SENSOR_MAX_HEIGHT - currHeight;
  currObject.width = DISTANCE_SENSOR_MAX_WIDTH - currWidth1 - currWidth2;
  
  // Get weight feature (in lbs) to the nearest 100ths place
  // Gets the average of 5 readings minus the tare weight
  // Need to debug with fruits
  currObject.weight = (scale.get_units(5), 1)*1000.0;
}

float RescaleValue(float value, const float min, const float max) {
  return (value-min)/(max-min);
}

// Rescale object features to 1-0 range
void RescaleObject(Object *object) {
  object->height = RescaleValue(object->height, DISTANCE_SENSOR_MIN_HEIGHT, DISTANCE_SENSOR_MAX_HEIGHT);
  object->width = RescaleValue(object->width, DISTANCE_SENSOR_MIN_WIDTH, DISTANCE_SENSOR_MAX_WIDTH);
  object->weight = RescaleValue(object->weight, WEIGHT_MIN, WEIGHT_MAX);
}

// Computes the euclidean distance between the known and the current object's features
float ComputeDistanceofObjects(Object currObject, Object knownObject) {
  // RescaleObject(&currObject);
  // RescaleObject(&knownObject);
  float height = (currObject.height - knownObject.height);
  float width = (currObject.width - knownObject.width);
  float weight = (currObject.weight - knownObject.weight);
  float dist = width*width + height*height + weight*weight;
  // dist = sqrt(dist);

  return dist;
}

// Finds the patterns from the current object and compares them to patterns from the given objects
// **
String ObjectPatternRecognition(Object currObject, Object knownObjects[]) {
  int count = 0;
  int max_count = 0;
  String most_frequent_type;

  Object kNearestObjects[K];
  
  Serial.print("Object height: ");
  Serial.println(currObject.height);
  Serial.print("Object width: ");
  Serial.println(currObject.width);
  Serial.print("Object weight: ");
  Serial.println(currObject.weight);

  // TODO: Use Min Heap Data Structure to Optimize this part
  
  // First K known objects are the closest K, obviously, so just fill array
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].height = knownObjects[i].height;
    kNearestObjects[i].width = knownObjects[i].width;
    kNearestObjects[i].weight = knownObjects[i].weight;
    kNearestObjects[i].type = knownObjects[i].type;
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
      kNearestObjects[max_index].height = knownObjects[i].height;
      kNearestObjects[max_index].width = knownObjects[i].width;
      kNearestObjects[max_index].weight = knownObjects[i].weight;
    }
  }

  // For testing purposes
  for(int i = 0; i < K; ++i) {
    Serial.println(kNearestObjects[i].type);
    Serial.print("Object height: ");
    Serial.println(kNearestObjects[i].height);
    Serial.print("Object width: ");
    Serial.println(kNearestObjects[i].width);
    Serial.print("Object weight: ");
    Serial.println(kNearestObjects[i].weight);
  }

  // Find out which object type occurs most frequently
  for(int i = 0; i < NUM_OF_OBJECT_TYPES; ++i) {
    int j;
    count = 0;
    for(j = 0; j < K; ++j) {
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
// **
void Actuation(String object) {
  if(object != "") {
    Serial.print("\t");
    Serial.println(object);
  }
}

void PopulateKnownObjects() {
  
  // Types are: Travel Mug, 11oz Mug, 16oz Mug
  // First of each type is the actual measurement
  knownObjects[0].type = "Travel Mug";
  knownObjects[0].height = 18.5;
  knownObjects[0].width = 7.0;
  knownObjects[0].weight = 219.5;
  
  knownObjects[1].type = "Travel Mug";
  knownObjects[1].height = 17.0;
  knownObjects[1].width = 8.0;
  knownObjects[1].weight = 250.0;
  
  knownObjects[2].type = "Travel Mug";
  knownObjects[2].height = 19.0;
  knownObjects[2].width = 6.0;
  knownObjects[2].weight = 210.0;
  
  knownObjects[3].type = "11oz Mug";
  knownObjects[3].height = 10.0;
  knownObjects[3].width = 8.0;
  knownObjects[3].weight = 351.3;
  
  knownObjects[4].type = "11oz Mug";
  knownObjects[4].height = 11.0;
  knownObjects[4].width = 7.0;
  knownObjects[4].weight = 360.0;
  
  knownObjects[5].type = "11oz Mug";
  knownObjects[5].height = 9.0;
  knownObjects[5].width = 9.0;
  knownObjects[5].weight = 340.0;
  
  knownObjects[6].type = "16oz Mug";
  knownObjects[6].height = 15.0;
  knownObjects[6].width = 8.4;
  knownObjects[6].weight = 517.4;
  
  knownObjects[7].type = "16oz Mug";
  knownObjects[7].height = 16.0;
  knownObjects[7].width = 8.0;
  knownObjects[7].weight = 500.0;
  
  knownObjects[8].type = "16oz Mug";
  knownObjects[8].height = 14.0;
  knownObjects[8].width = 10.0;
  knownObjects[8].weight = 530.0;
}


void setup() {
  
  // Should put feature extraction and pattern recognition in one multi-second for loop to
    // give the box time to go through the sensors.
  
  PopulateKnownObjects();
  
  // Height
  pinMode(TRIGGER_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  // Width
  pinMode(TRIGGER_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);
  pinMode(TRIGGER_PIN_3, OUTPUT);
  pinMode(ECHO_PIN_3, INPUT);
  // Weight
  scale.set_scale(calibration_factor);
  scale.tare();
  
  //Check if there is a cup
  pinMode(IR_SENSOR_PIN_1, INPUT);
  digitalWrite(IR_SENSOR_PIN_1, HIGH);
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  String closestObject;
  
  Serial.begin(9600);
}

void loop() {
  bool IR_Sensor1;
  String closestObject;
  
  IR_Sensor1 = digitalRead(IR_SENSOR_PIN_1);
  
  // Sensor was tripped and wasn't right before.
  if(IR_Sensor1 == LOW && !wait) {
    Serial.println("Box Detected");
    
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


//Distance Sensor   UPDATE
//Feature is height
//Pre-known object of different heights
//Pattern is see what the object is closest to.
#include <Arduino.h>
#include <NewPing.h>
#include <HX711.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"

#include <math.h>

#include "Object.h"

// Distance Sensor (Height)
#define TRIGGER_PIN 12
#define ECHO_PIN 11

// Weight Sensor
#define DOUT 3
#define CLK 2

int test = 0;

const int DISTANCE_SENSOR_MAX_DISTANCE = 50;

// HX711 Weight Sensor
// Use HX711_Calibration sketch to define
const float calibration_factor = 0;

// K Nearest Neighbors
// May want to declare locally and pass as parameter
const int K = 3;

byte gammatable[256];
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

const size_t NUM_OF_KNOWN_OBJECTS = 9;
const size_t NUM_OF_OBJECT_TYPES = 3;
String Known_Object_Types[3] = {"Small", "Medium", "Large"};
const int STRUCT_FIELDS = 5;

const int HEIGHT_MAX = 50;
const int HEIGHT_MIN = 0;
const int WEIGHT_MAX = 6;
const int WEIGHT_MIN = 0;
const int RGB_MAX = 256;
const int RGB_MIN = 0;

NewPing sonar(TRIGGER_PIN, ECHO_PIN, DISTANCE_SENSOR_MAX_DISTANCE);
HX711 scale(DOUT, CLK);

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
  float height = ((currObject.height - knownObject.height) - HEIGHT_MIN) / (HEIGHT_MAX - HEIGHT_MIN);
  float weight =  ((currObject.weight - knownObject.weight) - WEIGHT_MIN) / (WEIGHT_MAX - WEIGHT_MIN);;
  float colorDistance = ComputeDistanceofColors(currObject, knownObject);
  float dist = pow(height, 2) + pow(weight, 2) + pow(colorDistance, 2);
  dist = sqrt(dist);
  // Nearest 100ths place
  dist = roundf(dist * 100.0) / 100.0;
  
  return dist;
}

// Takes the features from the current object and converts them to strings and integers
// **
void ObjectFeatureExtraction(Object &currObject) {
  // Pass in an object (Pass by pointer) (Maybe?)
  int currDistance;
  //Need a fourth value for getRawData()
  uint16_t red, green, blue, clear_val;
  uint32_t sum;
  float r, g, b;
  
  // Get height feature (in cm)
  currDistance = sonar.ping_cm();
  currObject.height = DISTANCE_SENSOR_MAX_DISTANCE - currDistance;

  // Get weight feature (in lbs) to the nearest 100ths place
  currObject.weight = roundf(scale.get_units() * 100.0) / 100.0;

  // RGB VALUES
  // turn on LED
  tcs.setInterrupt(false);
  // takes 60ms to read
  delay(60);
  tcs.getRawData(&red, &green, &blue, &clear_val);
  //turn off LED
  tcs.setInterrupt(true);
  sum = clear_val;
  r = red/sum;
  g = green/sum;
  b = blue/sum;
  r *= 256;
  g *= 256;
  b *= 256;
  currObject.red = (int)r;
  currObject.green = (int)g;
  currObject.blue = (int)b;
  
}

// Finds the patterns from the current object and compares them to patterns from the given objects
// **
String PatternRecognition(Object currObject, Object knownObjects[], size_t NUM_OF_KNOWN_OBJECTS) {
  
  Object kNearestObjects[K];
  // For testing purposes
  /*
  Serial.print("Object standardized height: ");
  Serial.println(currObject.height);
  Serial.print("Object standardized weight: ");
  Serial.println(currObject.weight);
  Serial.print("Object standardized red: ");
  Serial.println(currObject.red);
  Serial.print("Object standardized green: ");
  Serial.println(currObject.green);
  Serial.print("Object standardized blue: ");
  Serial.println(currObject.blue);
  */
  
  // For testing purposes
  for(int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) {
    Serial.println(ComputeDistanceofObjects(currObject, knownObjects[i]));
  }
  
  // The first K Object differences are added to the kNearestObjects array
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].height = abs(knownObjects[i].height - currObject.height);
    kNearestObjects[i].weight = abs(knownObjects[i].weight - currObject.weight);
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
    int temp_height = 0;
    float temp_weight = 0;
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
      //temp_dist = ComputeDistanceofObjects(temp_height, temp_weight);
      
      //ISSUE MIGHT BE HERE
      temp_dist = ComputeDistanceofObjects(currObject, kNearestObjects[j]);
      
      if(temp_dist > max_diff) { // Update max
        max_diff = temp_dist;
        max_index = j;
      }
    }
    
    // If the current known object's difference < the max difference in the current K nearest neighbors
    temp_height = abs(knownObjects[i].height - currObject.height);
    temp_weight = abs(knownObjects[i].weight - currObject.weight);
    temp_red = abs(knownObjects[i].red - currObject.red);
    temp_green = abs(knownObjects[i].green - currObject.green);
    temp_blue = abs(knownObjects[i].blue - currObject.blue);
    //temp_dist = ComputeDistanceofObjects(temp_height, temp_weight);
    temp_dist = ComputeDistanceofObjects(currObject, knownObjects[i]);
    
    if(temp_dist < max_diff) {
      // Replace the existing neighbor having max_diff, by the current known object, in the K nearest neighbors array
      kNearestObjects[max_index].type = knownObjects[i].type;
      kNearestObjects[max_index].height = temp_height;
      kNearestObjects[max_index].weight = temp_weight;
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
    Serial.println(ComputeDistanceofObjects(currObject, kNearestObjects[i]));
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
  // Small is red, Medium is green, Large is blue
  const size_t NUM_OF_KNOWN_OBJECTS = 9;
  Object knownObjects[NUM_OF_KNOWN_OBJECTS] = { {"Small", 11, 1.2, 230, 54, 27},
                                                {"Small", 15, 0.8, 205, 43, 18},
                                                {"Small", 12, 1.6, 253, 40, 7},
                                                {"Medium", 27, 2.5, 5, 253, 7},
                                                {"Medium", 29, 3.2, 16, 196, 16},
                                                {"Medium", 30, 4.1, 45, 246, 45},
                                                {"Large", 40, 4.6, 30, 18, 191},
                                                {"Large", 42, 4.2, 66, 53, 235},
                                                {"Large", 42, 5.3, 17, 9, 134}};

  const size_t NUM_OF_TEST_OBJECTS = 9;

  Object testObjects[NUM_OF_KNOWN_OBJECTS] = {  {"", 11, 1.2, 230, 54, 27},
                                                {"", 15, 0.8, 205, 43, 18},
                                                {"", 12, 1.6, 253, 40, 7},
                                                {"", 27, 2.5, 5, 253, 7},
                                                {"", 29, 3.2, 16, 196, 16},
                                                {"", 30, 4.1, 45, 246, 45},
                                                {"", 40, 4.6, 30, 18, 191},
                                                {"", 42, 4.2, 66, 53, 235},
                                                {"", 42, 5.3, 17, 9, 134}};
  
  Object currObject; 
  String closestObject;
    
  ObjectFeatureExtraction(currObject);
  // Testing purposes
  /*
  Serial.print("Test: ");
  Serial.println(test);
  currObject = testObjects[test];
  if(test == 9) {
    test = 0;
  }
  else {
    ++test;
  }
  */

  //StandardizeObject(currObject, knownObjects);
  /*
  Serial.println(currObject.type);
  Serial.println(currObject.height);
  Serial.println(currObject.weight);
  Serial.println(currObject.red);
  Serial.println(currObject.green);
  Serial.println(currObject.blue);
  
  for(int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) {
    Serial.println(knownObjects[i].type);
    Serial.println(knownObjects[i].height);
    Serial.println(knownObjects[i].weight);
    Serial.println(knownObjects[i].red);
    Serial.println(knownObjects[i].green);
    Serial.println(knownObjects[i].blue);
  }
  */
  closestObject = PatternRecognition(currObject, knownObjects, NUM_OF_KNOWN_OBJECTS);

  Actuation(closestObject);
  
  delay(1000);
}

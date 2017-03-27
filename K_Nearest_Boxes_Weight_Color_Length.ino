//Distance Sensor   UPDATE
//Feature is width
//Pre-known object of different widths
//Pattern is see what the object is closest to.

// This file has the most up-to-date code of all the files

#include <Arduino.h>
#include <NewPing.h>
#include <HX711.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"

#include <math.h>

#include "Object.h"

// IR Breakbeam (Width)
#define IR_SENSOR_PIN_1 4
#define IR_SENSOR_PIN_2 5
#define IR_SENSOR_PIN_3 6

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

// Needed for standardizing values
const int WIDTH_MAX = 3;
const int WIDTH_MIN = 1;
const int WEIGHT_MAX = 6;
const int WEIGHT_MIN = 0;
const int RGB_MAX = 256;
const int RGB_MIN = 0;

HX711 scale(DOUT, CLK);

// Takes the features from the current object and converts them to strings and integers
// **
void ObjectFeatureExtraction(Object &currObject) {
  //Object currObject;
  int currDistance;
  // clear_val is needed b/c the getRawData() requries 4 values
  uint16_t red, green, blue, clear_val;
  uint32_t sum;
  float r, g, b;
  bool IR_Sensor1;
  bool IR_Sensor2;
  bool IR_Sensor3;
  
  // Get width feature (in cm)
  IR_Sensor1 = digitalRead(IR_SENSOR_PIN_1);
  IR_Sensor2 = digitalRead(IR_SENSOR_PIN_2);
  IR_Sensor3 = digitalRead(IR_SENSOR_PIN_3);

  // currObject.width {1, 2, 3} {Short, Medium, Long Box}
  // LOW means beam is broken
  // Once IR_Sensor3 is broken, then IR_Sensor1 and IR_Sensor2 are tested to see if they are broken
  // If S3 is the only sensor broken then the box is short, if S2 is also broken then it is meaium,
    // is S1 is also broken then it is long 
  // {Start} S1 S2 S3 {End}
  if(IR_Sensor3 == LOW) {
    if(IR_Sensor1 == LOW && IR_Sensor2 == LOW) {
      currObject.width = 3;
    }
    //If one beam is broken
    else if(IR_Sensor1 == HIGH && IR_Sensor2 == LOW) {
      currObject.width = 2;
    }
    //If neither beam is broken
    else if(IR_Sensor1 == HIGH && IR_Sensor2 == HIGH) {
      //box_size = "None";
      currObject.width = 1;
    }
  }

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
  
  //return currObject; 
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
  float width = ((currObject.width - knownObject.width) - WIDTH_MIN) / (WIDTH_MAX - WIDTH_MIN);
  float weight =  ((currObject.weight - knownObject.weight) - WEIGHT_MIN) / (WEIGHT_MAX - WEIGHT_MIN);;
  float colorDistance = ComputeDistanceofColors(currObject, knownObject);
  float dist = pow(width, 2) + pow(weight, 2) + pow(colorDistance, 2);
  dist = sqrt(dist);
  // Nearest 100ths place
  dist = roundf(dist * 100.0) / 100.0;
  
  return dist;
}

// Finds the patterns from the current object and compares them to patterns from the given objects
// **
String ObjectPatternRecognition(Object currObject, Object knownObjects[], size_t NUM_OF_KNOWN_OBJECTS) {

  // Maintains the differences of the currObject and the knownObjects
  Object kNearestObjects[K];
  
  // For testing purposes
  for(int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) {
    Serial.println(ComputeDistanceofObjects(currObject, knownObjects[i]));
  }

  // The first K Object differences are added to the kNearestObjects array
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].width = abs(knownObjects[i].width - currObject.width);
    kNearestObjects[i].weight = abs(knownObjects[i].weight - currObject.weight);
    kNearestObjects[i].red = abs(knownObjects[i].red - currObject.red);
    kNearestObjects[i].green = abs(knownObjects[i].green - currObject.green);
    kNearestObjects[i].blue = abs(knownObjects[i].blue - currObject.blue);
    kNearestObjects[i].type = knownObjects[i].type;
  }  
  
  // Now, determine if each remaining object is among the K closest, and if so insert into array (thus dropping one obj)
  for(int i = K; i < NUM_OF_KNOWN_OBJECTS; ++i) { // For each remaining known object
    
    // Find the current max difference (CHANGE TO DISTANCE, EVERYWHERE) in the K nearest objects
    float max_diff_of_K_nearest = 0;
    int max_index = 0;
    float temp_dist = 0;

    for(int j = 0; j < K; ++j) {
      
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
      kNearestObjects[max_index].width = abs(knownObjects[i].width - currObject.width);
      kNearestObjects[max_index].weight = abs(knownObjects[i].weight - currObject.weight);
      kNearestObjects[max_index].red = abs(knownObjects[i].red - currObject.red);
      kNearestObjects[max_index].green = abs(knownObjects[i].green - currObject.green);
      kNearestObjects[max_index].blue = abs(knownObjects[i].blue - currObject.blue);
      max_diff_of_K_nearest = temp_dist;
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
  String most_frequent_type;
  // Find out which object type occurs most frequently
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
// **
void Actuation(String object) {
  if(object != "") {
    Serial.print("\t");
    Serial.println(object);
  }
}

void setup() {
  // Weight sensor, found with calibration sketch
  scale.set_scale(calibration_factor);
  //Resets scale to 0
  scale.tare();
  pinMode(IR_SENSOR_PIN_1, INPUT);
  digitalWrite(IR_SENSOR_PIN_1, HIGH);
  pinMode(IR_SENSOR_PIN_2, INPUT);
  digitalWrite(IR_SENSOR_PIN_2, HIGH);
  pinMode(IR_SENSOR_PIN_3, INPUT);
  digitalWrite(IR_SENSOR_PIN_3, HIGH);

  Serial.begin(9600);
}

void loop() {
  // Turns both LEDs off before each reading

  // Should put feature extraction and pattern recognition in one multi-second for loop to
    //give the box time to go through the sensors.

  // {Type, width (cm.), weight (lbs.)}
  // Small is red, Medium is green, Large is blue
  const size_t NUM_OF_KNOWN_OBJECTS = 9;
  Object knownObjects[NUM_OF_KNOWN_OBJECTS] = { {"Small", 1, 1.2, 230, 54, 27},
                                                {"Small", 1, 0.8, 205, 43, 18},
                                                {"Small", 1, 1.6, 253, 40, 7},
                                                {"Medium", 2, 2.5, 5, 253, 7},
                                                {"Medium", 2, 3.2, 16, 196, 16},
                                                {"Medium", 2, 4.1, 45, 246, 45},
                                                {"Large", 3, 4.6, 30, 18, 191},
                                                {"Large", 3, 4.2, 66, 53, 235},
                                                {"Large", 3, 5.3, 17, 9, 134}};

  /*
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
  */

  
  Object currObject; 
  String closestObjectType;

  // Once 3rd IR breakbeam is broken go through the process
  bool IR_Sensor3 = digitalRead(IR_SENSOR_PIN_2);
  if(IR_Sensor3 == LOW) {
    Object currObject;
    ObjectFeatureExtraction(currObject);
    
    closestObjectType = ObjectPatternRecognition(currObject, knownObjects, NUM_OF_KNOWN_OBJECTS);
  
    Actuation(closestObjectType);
    
    delay(1000);
  }
}

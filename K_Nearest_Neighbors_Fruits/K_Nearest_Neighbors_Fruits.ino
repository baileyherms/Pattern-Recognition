// Distance Sensor
// Feature is height
// Pre-known object of different heights
// Pattern is see what the object is closest to.
#include <Arduino.h>
//#include <NewPing.h>
#include <HX711.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"

#include "Object.h"

#define IR_SENSOR_PIN_1 6

// Weight Sensor
#define DOUT 3
#define CLK 2

bool wait = false;

// Needs to be int for the distance sensor.
//const int DISTANCE_SENSOR_MAX_HEIGHT = 100;
//const int DISTANCE_SENSOR_MAX_WIDTH = 60;
const float WEIGHT_MAX = 300.0;
const float WEIGHT_MIN = 100.0;
const int COLOR_MAX = 255;
const int COLOR_MIN = 0;

// HX711 Weight Sensor
// Use HX711_Calibration sketch to define
float calibration_factor = -204340.00;

// K Nearest Neighbors
const int K = 3;

byte gammatable[256];
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_60X);

const size_t NUM_OF_KNOWN_OBJECTS = 12;
const size_t NUM_OF_OBJECT_TYPES = 4;
String Object_Types[NUM_OF_OBJECT_TYPES] = {"Apple", "Orange", "Pear", "Lemon"};

Object knownObjects[NUM_OF_KNOWN_OBJECTS];

HX711 scale(DOUT, CLK);

// Takes the features from the current object and converts them to strings and integers
// **
void ObjectFeatureExtraction(Object &currObject) {
  // clear_val is needed b/c the getRawData() requries 4 values
  uint16_t r, g, b, c, colorTemp, lux;
  
  // Get weight feature (in lbs) to the nearest 100ths place
  // Gets the average of 5 readings minus the tare weight
  // Need to debug with fruits
  currObject.weight = (scale.get_units(5), 1)*1000.0;
  
  // Get color feature
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);
  // c is clear and it is the sum of the red, green, and blue values.
  uint32_t sum = c;
  float red, green, blue;

  /*
  // RGB VALUES
  // turn on LED
  tcs.setInterrupt(false);
  // takes 60ms to read
  delay(60);
  //turn off LED
  tcs.setInterrupt(true);
  // turn on LED
  */
  
  // Converts the values to 0-255
  red = r;
  red /= sum;
  red *= 255;
  
  green = g;
  green /= sum;
  green *= 255;
  
  blue = b;
  blue /= sum;
  blue *= 255;
  
  currObject.r = (int)red;
  currObject.g = (int)green;
  currObject.b = (int)blue;
}

float RescaleValue(float value, const float min, const float max) {
  return (value-min)/(max-min);
}

// Rescale object features to 1-0 range
void RescaleObject(Object *object) {
  object->weight = RescaleValue(object->weight, WEIGHT_MIN, WEIGHT_MAX);
  object->r = RescaleValue(object->r, COLOR_MIN, COLOR_MAX);
  object->g = RescaleValue(object->g, COLOR_MIN, COLOR_MAX);
  object->b = RescaleValue(object->b, COLOR_MIN, COLOR_MAX);
}

//  Computes the euclidean distance between the known and the current object's rgb values
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
  float weight = (currObject.weight - knownObject.weight);
  float color_dist = ComputeDistanceofColors(currObject, knownObject);
  float dist = weight*weight + color_dist*color_dist;
  
  return dist;
}

// Finds the patterns from the current object and compares them to patterns from the given objects
// **
String ObjectPatternRecognition(Object currObject, Object knownObjects[]) {
  int count = 0;
  int max_count = 0;
  String most_frequent_type;

  Object kNearestObjects[K];
  
  Serial.print("Object weight: ");
  Serial.println(currObject.weight);
  Serial.print("Object red: ");
  Serial.println(currObject.r);
  Serial.print("Object green: ");
  Serial.println(currObject.g);
  Serial.print("Object blue: ");
  Serial.println(currObject.b);
 
  // First K known objects are the closest K, obviously, so just fill array
  for(int i = 0; i < K; ++i) {
    kNearestObjects[i].type = knownObjects[i].type;
    kNearestObjects[i].weight = knownObjects[i].weight;
    kNearestObjects[i].r = knownObjects[i].r;
    kNearestObjects[i].g = knownObjects[i].g;
    kNearestObjects[i].b = knownObjects[i].b;
  }
  
  // TODO: Use Min Heap Data Structure to Optimize this part
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
      kNearestObjects[max_index].weight = knownObjects[i].weight;
      kNearestObjects[max_index].r = knownObjects[i].r;
      kNearestObjects[max_index].g = knownObjects[i].g;
      kNearestObjects[max_index].b = knownObjects[i].b;
    }
  }

  // For testing purposes
  for(int i = 0; i < K; ++i) {
    Serial.println(kNearestObjects[i].type);
    Serial.println(kNearestObjects[i].weight);
    Serial.println(kNearestObjects[i].r);
    Serial.println(kNearestObjects[i].g);
    Serial.println(kNearestObjects[i].b);
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
  knownObjects[0].type = "Orange";
  knownObjects[0].weight = 270.0;
  knownObjects[0].r = 255;
  knownObjects[0].g = 139;
  knownObjects[0].b = 85;
  
  knownObjects[1].type = "Orange";
  knownObjects[1].weight = 257.0;
  knownObjects[1].r = 255;
  knownObjects[1].g = 140;
  knownObjects[1].b = 88;
  
  knownObjects[2].type = "Orange";
  knownObjects[2].weight = 250.0;
  knownObjects[2].r = 255;
  knownObjects[2].g = 128;
  knownObjects[2].b = 85;
  
  knownObjects[3].type = "Apple";
  knownObjects[3].weight = 228.0;
  knownObjects[3].r = 118;
  knownObjects[3].g = 76;
  knownObjects[3].b = 62;
  
  knownObjects[4].type = "Apple";
  knownObjects[4].weight = 216.0;
  knownObjects[4].r = 111;
  knownObjects[4].g = 80;
  knownObjects[4].b = 63;
  
  knownObjects[5].type = "Apple";
  knownObjects[5].weight = 224.0;
  knownObjects[5].r = 124;
  knownObjects[5].g = 80;
  knownObjects[5].b = 65;
  
  knownObjects[6].type = "Lemon";
  knownObjects[6].weight = 154.0;
  knownObjects[6].r = 255;
  knownObjects[6].g = 229;
  knownObjects[6].b = 127;
  
  knownObjects[7].type = "Lemon";
  knownObjects[7].weight = 148.0;
  knownObjects[7].r = 255;
  knownObjects[7].g = 242;
  knownObjects[7].b = 139;
  
  knownObjects[8].type = "Lemon";
  knownObjects[8].weight = 141.0;
  knownObjects[8].r = 255;
  knownObjects[8].g = 255;
  knownObjects[8].b = 141;
    
  knownObjects[9].type = "Pear";
  knownObjects[9].weight = 217.0;
  knownObjects[9].r = 198;
  knownObjects[9].g = 183;
  knownObjects[9].b = 110;
    
  knownObjects[10].type = "Pear";
  knownObjects[10].weight = 230.0;
  knownObjects[10].r = 172;
  knownObjects[10].g = 150;
  knownObjects[10].b = 90;
    
  knownObjects[11].type = "Pear";
  knownObjects[11].weight = 222.0;
  knownObjects[11].r = 207;
  knownObjects[11].g = 190;
  knownObjects[11].b = 109;
}


void setup() {
  Serial.begin(9600);
  
  scale.set_scale(calibration_factor);
  scale.tare();
  
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    //Serial.println("No TCS34725 found ... check your connections");
    Serial.println("No Color Sensor found ... check your connections");
    while (1);
  }
  
  PopulateKnownObjects();
  
  pinMode(IR_SENSOR_PIN_1, INPUT);
  digitalWrite(IR_SENSOR_PIN_1, HIGH);
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  String closestObject;
}

void loop() {
  bool IR_Sensor1;
  String closestObject;
  
  IR_Sensor1 = digitalRead(IR_SENSOR_PIN_1);
  
  // Sensor was tripped and wasn't right before.
  if(IR_Sensor1 == LOW && !wait) {
    Serial.println("Fruit Detected");
    
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


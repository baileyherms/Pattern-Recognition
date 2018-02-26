#include <Arduino.h>
#include <math.h>
// FIXME: Add the libraries needed for each pin
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include "Object.h"

// Define Pins for each sensor
// Distance Sensor (Height)

int test = 0;
bool wait = false;

// Define anything else needed for sensors
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

// K Nearest Neighbors
const int K = 3;

const size_t NUM_OF_KNOWN_OBJECTS = 8;
const size_t NUM_OF_OBJECT_TYPES = 2;

String Known_Object_Types[NUM_OF_OBJECT_TYPES] = {"Normal Shaking", "Abnormal Shaking"};

Object knownObjects[NUM_OF_KNOWN_OBJECTS];

// Define max height/weight/etc.
// FIXME: For each feature
// Change the values!
const float x_accel_diff_MIN = -10;
const float x_accel_diff_MAX = 10;
const float y_accel_diff_MIN = -10;
const float y_accel_diff_MAX = 10;
const float z_accel_diff_MIN = -12;
const float z_accel_diff_MAX = 7.5;

float x_resting_avg;
float y_resting_avg;
float z_resting_avg;

float x_tolerance;
float y_tolerance;
float z_tolerance;

const int length_detect_values = 100; // how many values to detect
const int time_between_values = 10; // in ms

// Takes the features from the current object and converts them to strings and integers
// **
void ObjectFeatureExtraction(Object &currObject) {
  // Define necessary variables for height/weight/etc.
  
  // Read features from sensors (ex: sonar.ping_cm())
  
  // Assign each of those features to currObject.
  float xMin = x_resting_avg - x_tolerance;
  float xMax = x_resting_avg + x_tolerance;
  float yMin = y_resting_avg - y_tolerance;
  float yMax = y_resting_avg + y_tolerance;
  float zMin = z_resting_avg - z_tolerance;
  float zMax = z_resting_avg + z_tolerance;

  float xTemp, yTemp, zTemp;
  // Get the max and min of x, y, and z for length_detect_values.
  for(int i = 0; i < length_detect_values; ++i) {
    lis.read();
    sensors_event_t event; 
    lis.getEvent(&event);

    xTemp = event.acceleration.x;
    yTemp = event.acceleration.y;
    zTemp = event.acceleration.z;

    if(xTemp > xMax) {
      xMax = xTemp;
    }
    else if(xTemp < xMin) {
      xMin = xTemp;
    }
    
    if(yTemp > yMax) {
      yMax = yTemp;
    }
    else if(yTemp < yMin) {
      yMin = yTemp;
    }
    
    if(zTemp > zMax) {
      zMax = zTemp;
    }
    else if(xTemp < xMin) {
      zMin = zTemp;
    }
    delay(time_between_values);
  }
  
  // Need to get the difference between max and resting and min and resting.
  currObject.x_accel_diff = abs(abs(xMin - x_resting_avg) - abs(xMax - x_resting_avg));
  currObject.y_accel_diff = abs(abs(yMin - y_resting_avg) - abs(yMax - y_resting_avg));
  currObject.z_accel_diff = abs(abs(zMin - z_resting_avg) - abs(zMax - z_resting_avg));
  Serial.print("X difference: "); Serial.println(currObject.x_accel_diff);
  Serial.print("Y difference: "); Serial.println(currObject.y_accel_diff);
  Serial.print("Z difference: "); Serial.println(currObject.z_accel_diff);

  // FIXME: Testing only.
  Serial.print("X max: "); Serial.println(xMax);
  Serial.print("Y max: "); Serial.println(yMax);
  Serial.print("Z max: "); Serial.println(zMax);
  
  Serial.print("X min: "); Serial.println(xMin);
  Serial.print("Y min: "); Serial.println(yMin);
  Serial.print("Z min: "); Serial.println(zMin);
}

float RescaleValue(float value, const float min, const float max) {
  return (value-min)/(max-min);
}

// Rescale object features to 1-0 range
void RescaleObject(Object *object) {
  object->x_accel_diff = RescaleValue(object->x_accel_diff, x_accel_diff_MIN, x_accel_diff_MAX);
  object->y_accel_diff = RescaleValue(object->y_accel_diff, y_accel_diff_MIN, y_accel_diff_MAX);
  object->z_accel_diff = RescaleValue(object->z_accel_diff, z_accel_diff_MIN, z_accel_diff_MAX);
}

// Computes the euclidean distance between the known and the current object's features
float ComputeDistanceofObjects(Object currObject, Object knownObject) {
  float x_accel_diff_dist = (currObject.x_accel_diff - knownObject.x_accel_diff);
  float y_accel_diff_dist = (currObject.y_accel_diff - knownObject.y_accel_diff);
  float z_accel_diff_dist = (currObject.z_accel_diff - knownObject.z_accel_diff);
  float dist = x_accel_diff_dist*x_accel_diff_dist + y_accel_diff_dist*y_accel_diff_dist + z_accel_diff_dist*z_accel_diff_dist;
  
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
    kNearestObjects[i].x_accel_diff = knownObjects[i].x_accel_diff;
    kNearestObjects[i].y_accel_diff = knownObjects[i].y_accel_diff;
    kNearestObjects[i].z_accel_diff = knownObjects[i].z_accel_diff;
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
      kNearestObjects[max_index].x_accel_diff = knownObjects[i].x_accel_diff;
      kNearestObjects[max_index].y_accel_diff = knownObjects[i].y_accel_diff;
      kNearestObjects[max_index].z_accel_diff = knownObjects[i].z_accel_diff;
    }
  }

  // For testing purposes
  for(int i = 0; i < K; ++i) {
    Serial.println(kNearestObjects[i].type);
    // FIXME: For each feature
    Serial.println(kNearestObjects[i].x_accel_diff);
    Serial.println(kNearestObjects[i].y_accel_diff);
    Serial.println(kNearestObjects[i].z_accel_diff);
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
  // Change values and measure
  // FIXME: For each feature
  // FIXME: Add as many known objects as needed
  
  // 4 for Normal Shaking
  knownObjects[0].type = "Normal Shaking";
  knownObjects[0].x_accel_diff = 0.26;
  knownObjects[0].y_accel_diff = 0.15;
  knownObjects[0].z_accel_diff = 0.27;
  
  knownObjects[2].type = "Normal Shaking";
  knownObjects[2].x_accel_diff = 0.31;
  knownObjects[2].y_accel_diff = 0.74;
  knownObjects[2].z_accel_diff = 1.11;
  
  knownObjects[4].type = "Normal Shaking";
  knownObjects[4].x_accel_diff = 0.19;
  knownObjects[4].y_accel_diff = 2.46;
  knownObjects[4].z_accel_diff = 2.35;
  
  knownObjects[6].type = "Normal Shaking";
  knownObjects[6].x_accel_diff = 0.29;
  knownObjects[6].y_accel_diff = 2.47;
  knownObjects[6].z_accel_diff = 1.59;

  // 4 for Abnormal Shaking
  knownObjects[1].type = "Abnormal Shaking";
  knownObjects[1].x_accel_diff = 4.74;
  knownObjects[1].y_accel_diff = 1.60;
  knownObjects[1].z_accel_diff = 6.74;
  
  knownObjects[3].type = "Abnormal Shaking";
  knownObjects[3].x_accel_diff = 2.06;
  knownObjects[3].y_accel_diff = 4.61;
  knownObjects[3].z_accel_diff = 12.33;
  
  knownObjects[5].type = "Abnormal Shaking";
  knownObjects[5].x_accel_diff = 6.31;
  knownObjects[5].y_accel_diff = 2.11;
  knownObjects[5].z_accel_diff = 8.31;
  
  knownObjects[7].type = "Abnormal Shaking";
  knownObjects[7].x_accel_diff = 3.82;
  knownObjects[7].y_accel_diff = 4.42;
  knownObjects[7].z_accel_diff = 8.67;
}

void CalibrateSystem() {
  Serial.println("Calibrating System, do not move machine.");
  
  float x_sum = 0;
  float y_sum = 0;
  float z_sum = 0;
  float x_min, x_max, y_min, y_max, z_min, z_max;
  float x_temp, y_temp, z_temp;
    
  lis.read();

  sensors_event_t event;
  lis.getEvent(&event);

  x_min = event.acceleration.x;
  x_max = event.acceleration.x;
  y_min = event.acceleration.y;
  y_max = event.acceleration.y;
  z_min = event.acceleration.z;
  z_max = event.acceleration.z;
  
  for(int i = 0; i < 50; ++i) {
    lis.read();
  
    sensors_event_t event;
    lis.getEvent(&event);

    x_temp = event.acceleration.x;
    y_temp = event.acceleration.y;
    z_temp = event.acceleration.z;

    if(x_temp > x_max) {
      x_max = x_temp;
    }
    else if(x_temp < x_min) {
      x_min = x_temp;
    }
    if(y_temp > y_max) {
      y_max = y_temp;
    }
    else if(y_temp < y_min) {
      y_min = y_temp;
    }
    if(z_temp > z_max) {
      z_max = z_temp;
    }
    else if(z_temp < z_min) {
      z_min = z_temp;
    }
    
    x_sum += x_temp;
    y_sum += y_temp;
    z_sum += z_temp;
    //Serial.print("X temp: "); Serial.println(x_temp);
    //Serial.print("Y temp: "); Serial.println(y_temp);
    //Serial.print("Z temp: "); Serial.println(z_temp);
  }
  x_resting_avg = x_sum / 50;
  y_resting_avg = y_sum / 50;
  z_resting_avg = z_sum / 50;

  // Tolerance will be absolute difference between min and max value / 2, so that the acceptable resting range is the measured resting min and max.
  x_tolerance = abs(x_min - x_max)/2;
  y_tolerance = abs(y_min - y_max)/2;
  z_tolerance = abs(z_min - z_max)/2;

  Serial.println("System Calibrated");
  Serial.print("X resting avg: "); Serial.println(x_resting_avg);
  Serial.print("Y resting avg: "); Serial.println(y_resting_avg);
  Serial.print("Z resting avg: "); Serial.println(z_resting_avg);
  Serial.print("X tolerance: "); Serial.println(x_tolerance);
  Serial.print("Y tolerance: "); Serial.println(y_tolerance);
  Serial.print("Z tolerance: "); Serial.println(z_tolerance);
}

void setup() {
  Serial.begin(9600);
  if (! lis.begin(0x18)) {   // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start");
    while (1);
  }
  Serial.println("LIS3DH found!");
  
  lis.setRange(LIS3DH_RANGE_2_G);

  CalibrateSystem();
  
  PopulateKnownObjects();
  
  // FIXME: Setup any features that need it
}

void loop() {
  String closestObject;
  
  Object currObject;
  
  ObjectFeatureExtraction(currObject);

  closestObject = ObjectPatternRecognition(currObject, knownObjects);

  Actuation(closestObject);

  delay(1000);
}

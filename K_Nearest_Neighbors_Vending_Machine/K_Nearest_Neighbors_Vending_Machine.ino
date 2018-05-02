#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include "Object.h"

Adafruit_LIS3DH lis = Adafruit_LIS3DH();

const int NUM_OF_CATEGORIES = 2;
String ObjectCategories[NUM_OF_CATEGORIES] = {"Normal Shaking", "Abnormal Shaking"};

int test_num = 0; //FIXME: Remove

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

// K Nearest Neighbors
// FIXME: Change when necessary
const int K = 3;

// FIXME: Change when necessary
const int NUM_OF_KNOWN_OBJECTS = 8;
Object knownObjects[NUM_OF_KNOWN_OBJECTS];

/*
    Known Objects Preperation
*/

float RescaleValue(float value, const float min, const float max) {
    return (value-min)/(max-min);
}

// Rescale object features to 1-0 range
Object RescaleObject(Object object) {
    Object rescaledObject;
    rescaledObject.category = object.category;
    rescaledObject.x_accel_diff = RescaleValue(object.x_accel_diff, x_accel_diff_MIN, x_accel_diff_MAX);
    rescaledObject.y_accel_diff = RescaleValue(object.y_accel_diff, y_accel_diff_MIN, y_accel_diff_MAX);
    rescaledObject.z_accel_diff = RescaleValue(object.z_accel_diff, z_accel_diff_MIN, z_accel_diff_MAX);
    return rescaledObject;
}

/*
    Populating known objects
*/

// Add an object to the known objects array
// Change for each object (may need to add eight, colors, etc.)
void AddToKnownObjects(int i, String category, float x_accel_diff, float y_accel_diff, float z_accel_diff) {
    knownObjects[i].category = category;
    knownObjects[i].x_accel_diff = x_accel_diff;
    knownObjects[i].y_accel_diff = y_accel_diff;
    knownObjects[i].z_accel_diff = z_accel_diff;
    knownObjects[i] = RescaleObject(knownObjects[i]);
}

// Insert all Known objects into the array
void PopulateKnownObjects() {
    // Even indices for "Normal Shaking"
    AddToKnownObjects(0, "Normal Shaking", 0.26, 0.15, 0.27);
    AddToKnownObjects(2, "Normal Shaking", 0.31, 0.74, 1.11);
    AddToKnownObjects(4, "Normal Shaking", 0.19, 2.46, 2.35);
    AddToKnownObjects(6, "Normal Shaking", 0.29, 2.47, 1.59);
    
    // Odd indices for "Abnormal Shaking"
    AddToKnownObjects(1, "Abnormal Shaking", 4.74, 1.60, 6.74);
    AddToKnownObjects(3, "Abnormal Shaking", 2.06, 4.61, 12.33);
    AddToKnownObjects(5, "Abnormal Shaking", 6.31, 2.11, 8.31);
    AddToKnownObjects(7, "Abnormal Shaking", 3.82, 4.42, 8.67);
}

/*
    Feature Extraction
*/

// Takes the features from the current object and converts them to strings and integers
Object FeatureExtraction() {
    Object inputObject;
    
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
    inputObject.x_accel_diff = abs(abs(xMin - x_resting_avg) - abs(xMax - x_resting_avg));
    inputObject.y_accel_diff = abs(abs(yMin - y_resting_avg) - abs(yMax - y_resting_avg));
    inputObject.z_accel_diff = abs(abs(zMin - z_resting_avg) - abs(zMax - z_resting_avg));
    Serial.print("X difference: "); Serial.println(inputObject.x_accel_diff);
    Serial.print("Y difference: "); Serial.println(inputObject.y_accel_diff);
    Serial.print("Z difference: "); Serial.println(inputObject.z_accel_diff);

    // FIXME: Testing only.
    Serial.print("X max: "); Serial.println(xMax);
    Serial.print("Y max: "); Serial.println(yMax);
    Serial.print("Z max: "); Serial.println(zMax);

    Serial.print("X min: "); Serial.println(xMin);
    Serial.print("Y min: "); Serial.println(yMin);
    Serial.print("Z min: "); Serial.println(zMin);
    
    return RescaleObject(inputObject);
}

// Computes the euclidean distance between the known and the current object's features
float ComputeDistanceofObjects(Object inputObject, Object knownObject) {
    float x_accel_diff_dist = (inputObject.x_accel_diff - knownObject.x_accel_diff);
    float y_accel_diff_dist = (inputObject.y_accel_diff - knownObject.y_accel_diff);
    float z_accel_diff_dist = (inputObject.z_accel_diff - knownObject.z_accel_diff);
    float dist = pow(x_accel_diff_dist, 2) + pow(y_accel_diff_dist, 2) + pow(z_accel_diff_dist, 2);

    return dist;
}

// Sorts all the provided distances from small to large
void Sort(float* distances, String* categories) {
    float temp_dist;
    String temp_category;
    for(int i = NUM_OF_KNOWN_OBJECTS - 1; i >= 0; --i) {
        for(int j = 0; j < i; ++j) {
            if(distances[i] < distances[j]) {
                temp_dist = distances[i];
                distances[i] = distances[j];
                distances[j] = temp_dist;
                temp_category = categories[i];
                categories[i] = categories[j];
                categories[j] = temp_category;
            }
        }
    }
}

/*
    K-Nearest Neighbors (KNN)
*/

// Implementation of KNN algorithm
// It takes an input object and a list of known objects and predicts the category of the input object.
String ClassifyKNN(Object inputObject, Object knownObjects[]) {
    int count = 0;
    int max_count = 0;
    String most_frequent_category;

    Object kNearestObjects[K];
    float distances[NUM_OF_KNOWN_OBJECTS];
    String categories[NUM_OF_KNOWN_OBJECTS];
    
    // Compute the distance of each known object to the input object
    for(int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) {
        distances[i] = ComputeDistanceofObjects(inputObject, knownObjects[i]);
        categories[i] = knownObjects[i].x_accel_diff;
        categories[i] = knownObjects[i].y_accel_diff;
        categories[i] = knownObjects[i].z_accel_diff;
    }

    // Sort distances in ascending order
    Sort(distances, categories);

    // Find out which object type occurs most frequently
    for(int i = 0; i < NUM_OF_CATEGORIES; ++i) {
        count = 0;
        for(int j = 0; j < K; ++j) {
            if(categories[j] == ObjectCategories[i]) {
                count++;
            }
        }
        if(count > max_count) {
            max_count = count;
            most_frequent_category = ObjectCategories[i];
        }
    }

    return most_frequent_category;
    
    // Need to test
    /*
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
            temp_dist = ComputeDistanceofObjects(inputObject, kNearestObjects[j]);

            if(temp_dist > max_diff_of_K_nearest) { // Update max
                max_diff_of_K_nearest = temp_dist;
                max_index = j;
            }
        }

        // If the current known object's difference < the max difference in the current K nearest neighbors
        temp_dist = ComputeDistanceofObjects(inputObject, knownObjects[i]);

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
    for(int i = 0; i < NUM_OF_CATEGORIES; ++i) {
        int j;
        count = 0;
        for(j = 0; j < K; ++j) {
            if(kNearestObjects[j].type == ObjectCategories[i]) {
            count++;
            }
        }
        if(count > max_count) {
            max_count = count;
            most_frequent_type = ObjectCategories[i];
        }
    }

    return most_frequent_type;
    */
}

/* 
    Object Actuation
*/

// Prints the type of object that is being passed through the project
void Actuation(String category) {
    if(category != "") {
        for(int i = 0; i < NUM_OF_CATEGORIES; ++i) {
            if(category == ObjectCategories[i]) {
                Serial.println(category);
            }
        }
    }
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
    PopulateKnownObjects();
    
    if (! lis.begin(0x18)) {   // change this to 0x19 for alternative i2c address
        Serial.println("Couldnt start");
        while (1);
    }
    Serial.println("LIS3DH found!");

    lis.setRange(LIS3DH_RANGE_2_G);

    CalibrateSystem();
}

Object testCode(int& test_num) {
    Object inputObject;

    Serial.print("Test num: ");
    Serial.println(test_num);
    
    switch(test_num) {
      case 0: {
        inputObject.x_accel_diff = 0.26;
        inputObject.y_accel_diff = 0.15;
        inputObject.z_accel_diff = 0.27;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 1: {
        inputObject.x_accel_diff = 4.74;
        inputObject.y_accel_diff = 1.60;
        inputObject.z_accel_diff = 6.74;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 2: {
        inputObject.x_accel_diff = 0.31;
        inputObject.y_accel_diff = 0.74;
        inputObject.z_accel_diff = 1.11;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 3: {
        inputObject.x_accel_diff = 2.06;
        inputObject.y_accel_diff = 4.61;
        inputObject.z_accel_diff = 12.33;
        inputObject = RescaleObject(inputObject);
        test_num = 0;
        break;
      }
      default:
        break;
    }
    Serial.print("Test num: ");
    Serial.println(test_num);

    /*
    // Even indices for "Normal Shaking"
    AddToKnownObjects(0, "Normal Shaking", 0.26, 0.15, 0.27);
    AddToKnownObjects(2, "Normal Shaking", 0.31, 0.74, 1.11);
    AddToKnownObjects(4, "Normal Shaking", 0.19, 2.46, 2.35);
    AddToKnownObjects(6, "Normal Shaking", 0.29, 2.47, 1.59);
    
    // Odd indices for "Abnormal Shaking"
    AddToKnownObjects(1, "Abnormal Shaking", 4.74, 1.60, 6.74);
    AddToKnownObjects(3, "Abnormal Shaking", 2.06, 4.61, 12.33);
    AddToKnownObjects(5, "Abnormal Shaking", 6.31, 2.11, 8.31);
    AddToKnownObjects(7, "Abnormal Shaking", 3.82, 4.42, 8.67);
    */
    
    return inputObject;
}


void loop() {
    String closest_object_category;
    
    Object inputObject = FeatureExtraction();

    closest_object_category = ClassifyKNN(inputObject, knownObjects);

    Actuation(closest_object_category);

    delay(1000);
}

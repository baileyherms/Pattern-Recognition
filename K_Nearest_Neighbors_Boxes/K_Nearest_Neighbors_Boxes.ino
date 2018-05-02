// Distance Sensor
// Feature is height
// Pre-known object of different heights
// Pattern is see what the object is closest to.
#include <Arduino.h>
#include <math.h>
#include <NewPing.h>
#include "Object.h"

// IR Sensor
#define IR_SENSOR_PIN_1 6

// Height
#define TRIGGER_PIN_1 12
#define ECHO_PIN_1 11

// Width
#define TRIGGER_PIN_2 10
#define ECHO_PIN_2 9

#define TRIGGER_PIN_3 8
#define ECHO_PIN_3 7

// Length
#define TRIGGER_PIN_4 16
#define ECHO_PIN_4 15

#define TRIGGER_PIN_5 14
#define ECHO_PIN_5 13

// FIXME: Change object categories
const int NUM_OF_CATEGORIES = 3;
String ObjectCategories[NUM_OF_CATEGORIES] = {"Small", "Medium", "Large"};

bool wait = false;
int test_num = 0; // FIXME: Remove

// Needs to be int for the distance sensor.
//const int DISTANCE_SENSOR_MAX_HEIGHT = 100;
//const int DISTANCE_SENSOR_MAX_WIDTH = 60;
// In centimeters
const int DISTANCE_SENSOR_MAX_HEIGHT = 39;
const int DISTANCE_SENSOR_MAX_WIDTH = 45;
const int DISTANCE_SENSOR_MAX_LENGTH = 119;

const int DISTANCE_SENSOR_MIN_HEIGHT = 2;
const int DISTANCE_SENSOR_MIN_WIDTH = 1;
const int DISTANCE_SENSOR_MIN_LENGTH = 1;

NewPing sonar_height(TRIGGER_PIN_1, ECHO_PIN_1, DISTANCE_SENSOR_MAX_HEIGHT);
NewPing sonar_width_1(TRIGGER_PIN_2, ECHO_PIN_2, DISTANCE_SENSOR_MAX_WIDTH);
NewPing sonar_width_2(TRIGGER_PIN_3, ECHO_PIN_3, DISTANCE_SENSOR_MAX_WIDTH);
NewPing sonar_length_1(TRIGGER_PIN_4, ECHO_PIN_4, DISTANCE_SENSOR_MAX_LENGTH);
NewPing sonar_length_2(TRIGGER_PIN_5, ECHO_PIN_5, DISTANCE_SENSOR_MAX_LENGTH);

// K Nearest Neighbors
const int K = 3;

const int NUM_OF_KNOWN_OBJECTS = 9;
Object knownObjects[NUM_OF_KNOWN_OBJECTS];

/*
    Known Objects Preperation
*/

// Rescale a value to 0-1 range
float RescaleValue(float value, const float min, const float max) {
    return (value-min)/(max-min);
}

// Rescale object features to 1-0 range
Object RescaleObject(Object object) {
    Object rescaledObject;
    rescaledObject.category = object.category;
    rescaledObject.height = RescaleValue(object.height, DISTANCE_SENSOR_MIN_HEIGHT, DISTANCE_SENSOR_MAX_HEIGHT);
    rescaledObject.width = RescaleValue(object.width, DISTANCE_SENSOR_MIN_WIDTH, DISTANCE_SENSOR_MAX_WIDTH);
    rescaledObject.length = RescaleValue(object.length, DISTANCE_SENSOR_MIN_LENGTH, DISTANCE_SENSOR_MAX_LENGTH);
    return rescaledObject;
}

/*
    Populating known objects
*/

// Add an object to the known objects array
// Change for each object (may need to add eight, colors, etc.)
void AddToKnownObjects(int i, String category, float height, float width, float length) {
    knownObjects[i].category = category;
    knownObjects[i].height = height;
    knownObjects[i].width = width;
    knownObjects[i].length = length;
    knownObjects[i] = RescaleObject(knownObjects[i]);
}

// Insert all Known objects into the array
void PopulateKnownObjects() {
    AddToKnownObjects(0, "Small", 8.0, 10.0, 10.0);
    AddToKnownObjects(1, "Small", 15.0, 15.0, 15.0);
    AddToKnownObjects(2, "Small", 18.0, 18.0, 18.0);
    
    AddToKnownObjects(3, "Medium", 24.0, 20.0, 20.0);
    AddToKnownObjects(4, "Medium", 28.0, 26.0, 28.0);
    AddToKnownObjects(5, "Medium", 25.0, 18.0, 18.0);
    
    AddToKnownObjects(6, "Large", 30.0, 34.0, 34.0);
    AddToKnownObjects(7, "Large", 28.0, 32.0, 32.0);
    AddToKnownObjects(8, "Large", 32.0, 36.0, 36.0);
}

// Takes the features from the current object and converts them to strings and integers
// **
Object FeatureExtraction() {
    Object inputObject;
    int currHeight;
    int currWidth1;
    int currWidth2;
    int currLength1;
    int currLength2;

    // Get height feature
    currHeight = sonar_height.ping_cm();
    currWidth1 = sonar_width_1.ping_cm();
    currWidth2 = sonar_width_2.ping_cm();
    currLength1 = sonar_length_1.ping_cm();
    currLength2 = sonar_length_2.ping_cm();

    inputObject.height = DISTANCE_SENSOR_MAX_HEIGHT - currHeight;
    inputObject.width = DISTANCE_SENSOR_MAX_WIDTH - currWidth1 - currWidth2;
    inputObject.length = DISTANCE_SENSOR_MAX_LENGTH - currLength1 - currLength2;

    return RescaleObject(inputObject);
}

// Computes the euclidean distance between the known and the current object's features
float ComputeDistanceofObjects(Object inputObject, Object knownObject) {
    float height = (inputObject.height - knownObject.height);
    float width = (inputObject.width - knownObject.width);
    float length = (inputObject.length - knownObject.length);
    float dist = pow(height, 2) + pow(width, 2) + pow(length, 2);
    dist = sqrt(dist);
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
    
    Serial.print("Object height: ");
    Serial.println(inputObject.height);
    Serial.print("Object width: ");
    Serial.println(inputObject.width);
    Serial.print("Object length: ");
    Serial.println(inputObject.length);
  
    for(int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) {
        distances[i] = ComputeDistanceofObjects(inputObject, knownObjects[i]);
        categories[i] = knownObjects[i].category;
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

void setup() {
    Serial.begin(9600);
    PopulateKnownObjects();

    // Height
    pinMode(TRIGGER_PIN_1, OUTPUT);
    pinMode(ECHO_PIN_1, INPUT);
    // Width
    pinMode(TRIGGER_PIN_2, OUTPUT);
    pinMode(ECHO_PIN_2, INPUT);
    pinMode(TRIGGER_PIN_3, OUTPUT);
    pinMode(ECHO_PIN_3, INPUT);
    // Length
    pinMode(TRIGGER_PIN_4, OUTPUT);
    pinMode(ECHO_PIN_4, INPUT);
    pinMode(TRIGGER_PIN_5, OUTPUT);
    pinMode(ECHO_PIN_5, INPUT);

    pinMode(IR_SENSOR_PIN_1, INPUT);
    digitalWrite(IR_SENSOR_PIN_1, HIGH);

    pinMode(LED_BUILTIN, OUTPUT);
}

Object testCode(int& test_num) {
    Object inputObject;

    Serial.print("Test num: ");
    Serial.println(test_num);
    
    switch(test_num) {
      case 0: {
        inputObject.height = 8;
        inputObject.width = 10;
        inputObject.length = 10;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 1: {
        inputObject.height = 28;
        inputObject.width = 26;
        inputObject.length = 28;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 2: {
        inputObject.height = 28;
        inputObject.width = 32;
        inputObject.length = 32;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 3: {
        inputObject.height = 18;
        inputObject.width = 18;
        inputObject.length = 18;
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
    AddToKnownObjects(0, "Small", 8.0, 10.0, 10.0);
    AddToKnownObjects(1, "Small", 15.0, 15.0, 15.0);
    AddToKnownObjects(2, "Small", 18.0, 18.0, 18.0);
    
    AddToKnownObjects(3, "Medium", 24.0, 20.0, 20.0);
    AddToKnownObjects(4, "Medium", 28.0, 26.0, 28.0);
    AddToKnownObjects(5, "Medium", 25.0, 18.0, 18.0);
    
    AddToKnownObjects(6, "Large", 30.0, 34.0, 34.0);
    AddToKnownObjects(7, "Large", 28.0, 32.0, 32.0);
    AddToKnownObjects(8, "Large", 32.0, 36.0, 36.0);
    */
    
    return inputObject;
}

void loop() {
    bool Detection_Sensor;
    String closest_object_category;
    Detection_Sensor = digitalRead(IR_SENSOR_PIN_1);

    // Sensor was tripped and wasn't right before.
    //if(Detection_Sensor == LOW && !wait) { // FIXME: Remove
        Serial.println("Box Detected");

        delay(2000);

        digitalWrite(LED_BUILTIN, HIGH);

        //Object inputObject = FeatureExtraction(); // FIXME: Put back
        Object inputObject = testCode(test_num); // FIXME: Remove

        closest_object_category = ClassifyKNN(inputObject, knownObjects);

        Actuation(closest_object_category);

        wait = true;

        delay(1000);
    //} // FIXME: Remove
    /* // FIXME: Remove
    // Sensor was tripped and was right before.
    else if(Detection_Sensor == LOW && wait) {
        digitalWrite(LED_BUILTIN, LOW);
    }
    // Sensor was not tripped.
    else if(Detection_Sensor == HIGH){
        digitalWrite(LED_BUILTIN, LOW);
        wait = false;
    }
    */ // FIXME: Remove
}



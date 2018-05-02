#include <Arduino.h>
#include <math.h>
// FIXME: Add the libraries needed for each pin
#include <NewPing.h>
#include "Object.h"

// Define anything else needed for sensors
// IR Sensor
#define IR_SENSOR_PIN_1 6

// Define Pins for each sensor
// Distance Sensor (Height)
#define TRIGGER_PIN_1 12
#define ECHO_PIN_1 11

// FIXME: Change object categories
const int NUM_OF_CATEGORIES = 3;
String ObjectCategories[NUM_OF_CATEGORIES] = {"Small", "Medium", "Large"};

bool wait = false;

// Define max height/weight/etc.
// FIXME: For each feature
// const int [sensor]_MAX_[feature] = [number];
// const int [sensor]_MIN_[feature] = [number];
const int DISTANCE_SENSOR_MAX_HEIGHT = 39;
const int DISTANCE_SENSOR_MIN_HEIGHT = 2;

// FIXME: Setup sensors if necessary
NewPing sonar_height(TRIGGER_PIN_1, ECHO_PIN_1, DISTANCE_SENSOR_MAX_HEIGHT);

// K Nearest Neighbors
// FIXME: Change when necessary
const int K = 3;

// FIXME: Change when necessary
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
    return rescaledObject;
}

/*
    Populating known objects
*/

// Add an object to the known objects array
// Change for each object (may need to add eight, colors, etc.)
void AddToKnownObjects(int i, String category, float height) {
    knownObjects[i].category = category;
    knownObjects[i].height = height;
    knownObjects[i] = RescaleObject(knownObjects[i]);
}

// Insert all Known objects into the array
void PopulateKnownObjects() {
    // FIXME: Change for every test object.
    /* 
    AddToKnownObjects(#, "[category]", [height]);
    */
    AddToKnownObjects(0, "Small", 8.0);
}

/*
    Feature Extraction
*/

// Takes the features from the current object and converts them to strings and integers
Object FeatureExtraction() {
    Object inputObject;

    // Define necessary variables for height/weight/etc.
    int currHeight;

    // Read features from sensors (ex: sonar.ping_cm())
    currHeight = sonar_height.ping_cm();

    // Assign each of those features to inputObject.
    inputObject.height = DISTANCE_SENSOR_MAX_HEIGHT - currHeight;

    return RescaleObject(inputObject);
}

// Computes the euclidean distance between the known and the current object's rgb values
// FIXME: Add function in if using a color sensor.
/*
float ComputeDistanceofColors(Object inputObject, Object knownObject) {
    float red = inputObject.red - knownObject.red;
    float green = inputObject.green - knownObject.green;
    float blue = inputObject.blue - knownObject.blue;
    float dist = pow(red, 2) + pow(green, 2) + pow(blue, 2);
    dist = sqrt(dist);
    return dist;
}
*/

// Computes the euclidean distance between the known and the current object's features
// Can add or remove features
float ComputeDistanceofObjects(Object inputObject, Object knownObject) {
    // FIXME: For non-color features
    //float [feature] = ((inputObject.[feature] - knownObject.[feature]) - [feature]_MIN) / ([feature]_MAX - [feature]_MIN);
    float height = (inputObject.height - knownObject.height);
    // FIXME (optional): For color feature
    //float colorDistance = ComputeDistanceofColors(inputObject, knownObject);
    // FIXME: For each feature
    //float dist = pow([feature 1], 2) + pow([feature 2], 2) + pow([feature 3], 2);
    float dist = pow(height, 2);
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

    // Compute the distance of each known object to the input object
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
    
    // FIXME: Setup any features that need it
    pinMode(TRIGGER_PIN_1, OUTPUT);
    pinMode(ECHO_PIN_1, INPUT);
    pinMode(IR_SENSOR_PIN_1, INPUT);
    digitalWrite(IR_SENSOR_PIN_1, HIGH);
}

void loop() {
    String closest_object_category;
    
    // FIXME: Change how object is detected.
    bool Detection_Sensor;
    Detection_Sensor = digitalRead(IR_SENSOR_PIN_1);
    
    // Sensor was tripped and wasn't right before.
    if(Detection_Sensor == LOW && !wait) {
        Serial.println("Object Detected");
        
        delay(2000);
        
        digitalWrite(LED_BUILTIN, HIGH);
        
        Object inputObject = FeatureExtraction();
    
        // Classification
        closest_object_category = ClassifyKNN(inputObject, knownObjects);
    
        // Actuation
        Actuation(closest_object_category);
        
        wait = true;
      
        delay(1000);
    }
    // Sensor was tripped and was right before.
    else if(Detection_Sensor == LOW && wait) {
        digitalWrite(LED_BUILTIN, LOW);
    }
    // Sensor was not tripped.
    else if(Detection_Sensor == HIGH){
        digitalWrite(LED_BUILTIN, LOW);
        wait = false;
    }
}


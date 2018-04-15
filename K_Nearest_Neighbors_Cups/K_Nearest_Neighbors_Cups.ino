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

const size_t NUM_OF_CATEGORIES = 3;
String ObjectCategories[NUM_OF_CATEGORIES] = {"Travel Mug", "11oz Mug", "16oz Mug"};

bool wait = false;

// Needs to be int for the distance sensor.
//const int DISTANCE_SENSOR_DISTANCE_SENSOR_HEIGHT_MAX = 100;
//const int DISTANCE_SENSOR_DISTANCE_SENSOR_HEIGHT_MAX = 60;
// In centimeters
const int DISTANCE_SENSOR_HEIGHT_MAX = 39;
const int DISTANCE_SENSOR_HEIGHT_MIN = 2;

const int DISTANCE_SENSOR_WIDTH_MAX = 46;
const int DISTANCE_SENSOR_WIDTH_MIN = 1;

const float WEIGHT_SENSOR_WEIGHT_MAX = 600.0;
const float WEIGHT_SENSOR_WEIGHT_MIN = 100.0;

// HX711 Weight Sensor
// Use HX711_Calibration sketch to define
float calibration_factor = -204340.00;

// K Nearest Neighbors
const int K = 3;

NewPing sonar_height(TRIGGER_PIN_1, ECHO_PIN_1, DISTANCE_SENSOR_HEIGHT_MAX);
NewPing sonar_width_1(TRIGGER_PIN_2, ECHO_PIN_2, DISTANCE_SENSOR_WIDTH_MAX);
NewPing sonar_width_2(TRIGGER_PIN_3, ECHO_PIN_3, DISTANCE_SENSOR_WIDTH_MAX);
HX711 scale(DOUT, CLK);

const size_t NUM_OF_KNOWN_OBJECTS = 9;
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
    rescaledObject.height = RescaleValue(object.height, DISTANCE_SENSOR_HEIGHT_MIN, DISTANCE_SENSOR_HEIGHT_MAX);
    rescaledObject.width = RescaleValue(object.width, DISTANCE_SENSOR_WIDTH_MIN, DISTANCE_SENSOR_WIDTH_MAX);
    rescaledObject.weight = RescaleValue(object.weight, WEIGHT_SENSOR_WEIGHT_MIN, WEIGHT_SENSOR_WEIGHT_MAX);
    return rescaledObject;
}

/*
    Populating known objects
*/

// Add an object to the known objects array
// Change for each object (may need to add eight, colors, etc.)
void AddToKnownObjects(int i, char* category, float height, float width, float weight) {
    knownObjects[i].category = category;
    knownObjects[i].height = height;
    knownObjects[i].width = width;
    knownObjects[i].weight = weight;
    knownObjects[i] = RescaleObject(knownObjects[i]);
}

// Insert all Known objects into the array
void PopulateKnownObjects() {
    AddToKnownObjects(0, "Travel Mug", 18.5, 7.0, 219.5);
    AddToKnownObjects(1, "Travel Mug", 17.0, 8.0, 250.0);
    AddToKnownObjects(2, "Travel Mug", 19.0, 6.0, 210.0);
    
    AddToKnownObjects(3, "11oz Mug", 10.0, 8.0, 351.3);
    AddToKnownObjects(4, "11oz Mug", 11.0, 7.0, 360.0);
    AddToKnownObjects(5, "11oz Mug", 9.0, 9.0, 340.0);
    
    AddToKnownObjects(6, "16oz Mug", 15.0, 8.4, 517.4);
    AddToKnownObjects(7, "16oz Mug", 16.0, 8.0, 500.0);
    AddToKnownObjects(8, "16oz Mug", 14.0, 10.0, 530.0);
}

// Takes the features from the current object and converts them to strings and integers
// **
Object FeatureExtraction() {
    Object inputObject;
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

    inputObject.height = DISTANCE_SENSOR_HEIGHT_MAX - currHeight;
    inputObject.width = DISTANCE_SENSOR_WIDTH_MAX - currWidth1 - currWidth2;

    // Get weight feature (in lbs) to the nearest 100ths place
    // Gets the average of 5 readings minus the tare weight
    // Need to debug with fruits
    inputObject.weight = (scale.get_units(5), 1)*1000.0;

    return RescaleObject(inputObject);
}

// Computes the euclidean distance between the known and the current object's features
float ComputeDistanceofObjects(Object inputObject, Object knownObject) {
    float height = (inputObject.height - knownObject.height);
    float width = (inputObject.width - knownObject.width);
    float weight = (inputObject.weight - knownObject.weight);
    float dist = pow(height, 2) + pow(width, 2) + pow(weight, 2);
    dist = sqrt(dist);
    return dist;
}

// Sorts all the provided distances from small to large
void Sort(float *distances, String* categories) {
    float temp_dist;
    String temp_category;
    for(int i = NUM_OF_CATEGORIES - 1; i >= 0; --i) {
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
    float distances[NUM_OF_CATEGORIES];
    String categories[NUM_OF_CATEGORIES];
    
    Serial.print("Object height: ");
    Serial.println(inputObject.height);
    Serial.print("Object width: ");
    Serial.println(inputObject.width);
    Serial.print("Object weight: ");
    Serial.println(inputObject.weight);

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
    PopulateKnownObjects();
}

void loop() {
    bool Detection_Sensor;
    String closest_object_category;

    Detection_Sensor = digitalRead(IR_SENSOR_PIN_1);

    // Sensor was tripped and wasn't right before.
    if(Detection_Sensor == LOW && !wait) {
        Serial.println("Cup Detected");

        delay(1000);

        digitalWrite(LED_BUILTIN, HIGH);

        Object inputObject = FeatureExtraction();

        closest_object_category = ClassifyKNN(inputObject, knownObjects);

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



#include <Arduino.h>
#include <math.h>
#include <LiquidCrystal.h>
#include "Object.h"

// FIXME: Add necessary libraries
#include <NewPing.h>

// LCD Screen
// FIXME: Change to fit LCD Screen wiring
const int rs = 41, en = 39, d4 = 37, d5 = 35, d6 = 33, d7 = 31;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
String lcd_current = "";

// FIXME: Define anything needed for sensors

// IR Sensor
#define IR_SENSOR_PIN_1 6

// Height Sensor
// FIXME: Define Pins for each sensor
#define TRIGGER_PIN_1 12
#define ECHO_PIN_1 11

// Category definitions
// FIXME: Change object categories
const int NUM_OF_CATEGORIES = 3;
String ObjectCategories[NUM_OF_CATEGORIES] = {"Small", "Medium", "Large"};

bool wait = false;

// Used for test code
int test_num = 0;

// Max and min feature values, needed for normalization
// FIXME: For each feature
// const int [feature]_MAX = [number];
// const int [feature]_MIN = [number];
const int HEIGHT_MAX = 39;
const int HEIGHT_MIN = 2;

// Distance Sensor
// FIXME: Setup sensors if necessary
NewPing sonar_height(TRIGGER_PIN_1, ECHO_PIN_1, HEIGHT_MAX);
// Classification definitions

// Classification definitions
// FIXME: Change when necessary
#define K_Parameter 3 // User in KNN

// Setup knownObjects
// FIXME: Change when necessary
#define NUM_OF_KNOWN_OBJECTS 1
Object knownObjects[NUM_OF_KNOWN_OBJECTS];

// Rescale a value to 0-1 range
float RescaleValue(float value, const float min, const float max) {
    return (value-min)/(max-min);
}

// Rescale object features to 0-1 range
Object RescaleObject(Object object) {
    Object rescaledObject;
    rescaledObject.category = object.category;
    // FIXME: Add for each feature
    rescaledObject.height = RescaleValue(object.height, HEIGHT_MIN, HEIGHT_MAX);
    return rescaledObject;
}

// Add new object to the known objects array
// Change for each object (may need to add weight, colors, etc.)
void AddToKnownObjects(int i, String category, float height) {
    knownObjects[i].category = category;
    knownObjects[i].height = height;
    knownObjects[i] = RescaleObject(knownObjects[i]);
}

// Insert all known objects into the known objects array ("training data")
void PopulateKnownObjects() {
    // FIXME: Add for every test object.
    /* 
    AddToKnownObjects(#, "[category]", [height]);
    */
    AddToKnownObjects(0, "Small", 8.0);
}

/* PHASE 1: FEATURE EXTRACTION */

// Extract features from sensors and create a new object with those features
// Default example below just uses sensor values as features
// For other applications, calculations may convert sensor values to features
Object FeatureExtraction() {
    Object inputObject;

    // FIXME: Define necessary variables for height/weight/etc.
    int currHeight;

    // FIXME: Read features from sensors (ex: sonar.ping_cm())
    currHeight = sonar_height.ping_cm();

    // FIXME: Assign each of those features to inputObject
    inputObject.height = HEIGHT_MAX - currHeight;
    
    // FIXME: Print each feature measurement
    Serial.print("Height: ");
    Serial.println(inputObject.height);

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

/* PHASE 2: CLASSIFICATION */

// Computes Euclidean distance between two objects for any # of dimensions.
float ComputeDistanceofObjects(Object object1, Object object2) {
    // FIXME: For non-color features
    //float [feature] = (object1.[feature] - object2.[feature]);
    float height = (object1.height - object2.height);
    // FIXME (optional): For color feature
    //float colorDistance = ComputeDistanceofColors(object1, object2);
    // FIXME: For each feature
    //float dist = pow([feature 1], 2) + pow([feature 2], 2) + pow([feature 3], 2);
    float dist = pow(height, 2);
    dist = sqrt(dist);
    return dist;
}

// Sorts the provided distances from small to large
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

// KNN classification: Predicts the input object's category given known objects
String ClassifyKNN(Object inputObject, Object knownObjects[]) {
    int count = 0;
    int max_count = 0;
    String most_frequent_category;
    
    // Maintains K nearest knownObjects
    Object kNearestObjects[K_Parameter];
    float distances[NUM_OF_KNOWN_OBJECTS];
    String categories[NUM_OF_KNOWN_OBJECTS];
    
    // Compute the distance of each known object to the input object
    for(int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) {
        distances[i] = ComputeDistanceofObjects(inputObject, knownObjects[i]);
        categories[i] = knownObjects[i].category;
    }

    // Sort distances in ascending order
    Sort(distances, categories);

    // For each category, determine if it’s the most frequent 
    // among the K closest known objects
    for(int i = 0; i < NUM_OF_CATEGORIES; ++i) {
        count = 0;
        // Count frequency of this category in K closest objects
        for(int j = 0; j < K_Parameter; ++j) {
            if(categories[j] == ObjectCategories[i]) {
                count++;
            }
        }
        if(count > max_count) { // Most frequent category so far
            max_count = count;
            most_frequent_category = ObjectCategories[i];
        }
    }

    return most_frequent_category;
}

/* PHASE 3: ACTUATION */

// Turns on corresponding output bit to show category of the input object
void Actuation(String category) {
    if(category != "") {
        for(int i = 0; i < NUM_OF_CATEGORIES; ++i) {
            if(category == ObjectCategories[i]) {
                Serial.println(category);
                
                // If the category has changed, print the new category to LCD
                if(lcd_current != category) {
                  lcd.setCursor(0,0);
                  lcd.print("                ");
                  lcd.setCursor(0,0);
                  lcd.print(category);
                  delay(500);
                  lcd_current = category;
                }
            }
        }
    }
}

void setup() {
    Serial.begin(9600);
    
    PopulateKnownObjects();
    
    // FIXME: Setup any features that need it
    // IR Sensor
    pinMode(IR_SENSOR_PIN_1, INPUT);
    digitalWrite(IR_SENSOR_PIN_1, HIGH);
    
    // Height
    pinMode(TRIGGER_PIN_1, OUTPUT);
    pinMode(ECHO_PIN_1, INPUT);

    // Arduino Built-in LED
    pinMode(LED_BUILTIN, OUTPUT);
}

// Can be used to test objects without sensors
// Need to change loop() to use this.
/*
Object testCode(int& test_num) {
    Object inputObject;

    Serial.print("Test num: ");
    Serial.println(test_num);
    
    switch(test_num) {
      case 0: {
        // FIXME: Change for each feature
        inputObject.height = 8;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 1: {
        inputObject.height = 28;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 2: {
        inputObject.height = 28;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 3: {
        inputObject.height = 18;
        inputObject = RescaleObject(inputObject);
        test_num = 0;
        break;
      }
      default:
        break;
    }
    Serial.print("Test num: ");
    Serial.println(test_num);
    
    return inputObject;
}
*/

void loop() {
    String closest_object_category;
    
    // FIXME: Change how object is detected.
    bool Detection_Sensor;
    Detection_Sensor = digitalRead(IR_SENSOR_PIN_1);
    
    // Sensor was tripped and wasn't right before.
    // FIXME: Change based on how object is detected
    if(Detection_Sensor == LOW && !wait) {
        // FIXME: Change "Object" to what is being detected
        Serial.println("Object Detected");
        
        delay(2000);
        
        // The built in LED is lit to show that the sensor was tripped.
        digitalWrite(LED_BUILTIN, HIGH);
        
        // Phase 1
        Object inputObject = FeatureExtraction();
    
        // Phase 2
        closest_object_category = ClassifyKNN(inputObject, knownObjects);
    
        // Phase 3
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
    
    // Test Code without sensors
    // Comment out above code
    /*
    String closest_object_category;
    
    Serial.println("Object Detected");
    
    delay(2000);

    Object inputObject = testCode(test_num);

    closest_object_category = ClassifyKNN(inputObject, knownObjects);

    Actuation(closest_object_category);

    delay(1000);
    */
}


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

// Sensor MIN/MAX Values
// FIXME: For each feature
// const int [sensor]_[feature]_MAX = [number];
// const int [sensor]_[feature]_MIN = [number];
const int DISTANCE_SENSOR_HEIGHT_MAX = 39;
const int DISTANCE_SENSOR_HEIGHT_MIN = 2;

// FIXME: Setup sensors if necessary
// Distance Sensor
NewPing sonar_height(TRIGGER_PIN_1, ECHO_PIN_1, DISTANCE_SENSOR_HEIGHT_MAX);

// K Nearest Neighbors
// FIXME: Change when necessary
const int K = 3;

// Setup knownObjects
// FIXME: Change when necessary
const int NUM_OF_KNOWN_OBJECTS = 9;
Object knownObjects[NUM_OF_KNOWN_OBJECTS];

/*
    Known Objects Preparation
*/

// Rescale a value to 0-1 range
float RescaleValue(float value, const float min, const float max) {
    return (value-min)/(max-min);
}

// Rescale object features
Object RescaleObject(Object object) {
    Object rescaledObject;
    rescaledObject.category = object.category;
    // FIXME: Add for each object
    rescaledObject.height = RescaleValue(object.height, DISTANCE_SENSOR_HEIGHT_MIN, DISTANCE_SENSOR_HEIGHT_MAX);
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
    // FIXME: Add for every test object.
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

    // FIXME: Define necessary variables for height/weight/etc.
    int currHeight;

    // FIXME: Read features from sensors (ex: sonar.ping_cm())
    currHeight = sonar_height.ping_cm();

    // FIXME: Assign each of those features to inputObject
    inputObject.height = DISTANCE_SENSOR_HEIGHT_MAX - currHeight;
    
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
        
        //Feature Exteaction
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


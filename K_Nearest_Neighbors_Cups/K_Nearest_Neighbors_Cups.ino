#include <Arduino.h>
#include <math.h>
#include <LiquidCrystal.h>
#include "Object.h"

#include <NewPing.h>
#include <HX711.h>

// LCD Screen
const int rs = 41, en = 39, d4 = 37, d5 = 35, d6 = 33, d7 = 31;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
String lcd_current = "";

// IR Sensor
#define IR_SENSOR_PIN_1 6

// Height Sensor
#define TRIGGER_PIN_1 12
#define ECHO_PIN_1 11

// Width Sensor
#define TRIGGER_PIN_2 10
#define ECHO_PIN_2 9

#define TRIGGER_PIN_3 8
#define ECHO_PIN_3 7

// Weight Sensor
#define DOUT 3
#define CLK 2

// Category definitions
const int NUM_OF_CATEGORIES = 3;
String ObjectCategories[NUM_OF_CATEGORIES] = {"Travel Mug", "11oz Mug", "16oz Mug"};

bool wait = false;

// Used for test code
int test_num = 0;

// Sensor MIN/MAX Values
const int HEIGHT_MAX = 39;
const int HEIGHT_MIN = 2;

const int WIDTH_MAX = 46;
const int WIDTH_MIN = 1;

const float WEIGHT_MAX = 600.0;
const float WEIGHT_MIN = 100.0;

// Weight Sensor
// Use HX711_Calibration sketch to define
float calibration_factor = -204340.00;
HX711 scale(DOUT, CLK);

// Distance Sensor
NewPing sonar_height(TRIGGER_PIN_1, ECHO_PIN_1, HEIGHT_MAX);
NewPing sonar_width_1(TRIGGER_PIN_2, ECHO_PIN_2, WIDTH_MAX);
NewPing sonar_width_2(TRIGGER_PIN_3, ECHO_PIN_3, WIDTH_MAX);

// K Nearest Neighbors
#define K_Parameter 3;

// Setup knownObjects
#define NUM_OF_KNOWN_OBJECTS 9
Object knownObjects[NUM_OF_KNOWN_OBJECTS];

// Rescale a value to 0-1 range
float RescaleValue(float value, const float min, const float max) {
    return (value-min)/(max-min);
}

// Rescale object features
Object RescaleObject(Object object) {
    Object rescaledObject;
    rescaledObject.category = object.category;
    rescaledObject.height = RescaleValue(object.height, HEIGHT_MIN, HEIGHT_MAX);
    rescaledObject.width = RescaleValue(object.width, WIDTH_MIN, WIDTH_MAX);
    rescaledObject.weight = RescaleValue(object.weight, WEIGHT_MIN, WEIGHT_MAX);
    return rescaledObject;
}

// Add an object to the known objects array
// Change for each object (may need to add eight, colors, etc.)
void AddToKnownObjects(int i, String category, float height, float width, float weight) {
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

/* PHASE 1: FEATURE EXTRACTION */

// Takes the features from the current object and converts them to strings and integers
Object FeatureExtraction() {
    Object inputObject;
    
    int currHeight;
    int currWidth1;
    int currWidth2;

    // Get distance measurements
    // Get the median of 10 measurements
    // Convert the measurements from inch to cm
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

    inputObject.height = HEIGHT_MAX - currHeight;
    inputObject.width = WIDTH_MAX - currWidth1 - currWidth2;

    // Get weight feature (in lbs) to the nearest 100ths place
    inputObject.weight = (scale.get_units())*1000.0;
    
    Serial.print("Height: ");
    Serial.println(inputObject.height);
    Serial.print("Width: ");
    Serial.println(inputObject.width);
    Serial.print("Weight: ");
    Serial.println(inputObject.weight);

    return RescaleObject(inputObject);
}

/* PHASE 2: CLASSIFICATION */

// Computes the euclidean distance between the known and the current object's features
float ComputeDistanceofObjects(Object object1, Object object2) {
    float height = (object1.height - object2.height);
    float width = (object1.width - object2.width);
    float weight = (object1.weight - object2.weight);
    float dist = pow(height, 2) + pow(width, 2) + pow(weight, 2);
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


// Implementation of KNN algorithm
// It takes an input object and a list of known objects and predicts the category of the input object.
String ClassifyKNN(Object inputObject, Object knownObjects[]) {
    int count = 0;
    int max_count = 0;
    String most_frequent_category;
    
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
    
    // Find out which object type occurs most frequently
    for(int i = 0; i < NUM_OF_CATEGORIES; ++i) {
        count = 0;
        for(int j = 0; j < K_Parameter; ++j) {
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

/* PHASE 3: ACTUATION */

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
    
    // LCD Screen
    lcd.begin(16,2);

    // IR Sensor
    pinMode(IR_SENSOR_PIN_1, INPUT);
    digitalWrite(IR_SENSOR_PIN_1, HIGH);

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
        inputObject.height = 18.5;
        inputObject.width = 7.0;
        inputObject.weight = 219.5;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 1: {
        inputObject.height = 10.0;
        inputObject.width = 8.0;
        inputObject.weight = 351.3;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 2: {
        inputObject.height = 15.0;
        inputObject.width = 8.4;
        inputObject.weight = 517.4;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 3: {
        inputObject.height = 17.0;
        inputObject.width = 8.0;
        inputObject.weight = 250.0;
        inputObject = RescaleObject(inputObject);
        test_num = 0;
        break;
      }
      default:
        break;
    }

    return inputObject;
}
*/

void loop() {
    String closest_object_category;
    
    bool Detection_Sensor;
    Detection_Sensor = digitalRead(IR_SENSOR_PIN_1);

    // Sensor was tripped and wasn't right before.
    if(Detection_Sensor == LOW && !wait) {
        Serial.println("Cup Detected");

        delay(1000);

        // The built in LED is lit to show that the sensor was tripped.
        digitalWrite(LED_BUILTIN, HIGH);

        // Feature Exteaction
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
    
    Serial.println("Box Detected");
    
    delay(2000);

    Object inputObject = testCode(test_num);

    closest_object_category = ClassifyKNN(inputObject, knownObjects);

    Actuation(closest_object_category);

    delay(1000);
    */
}

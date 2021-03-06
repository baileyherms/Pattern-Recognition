#include <Arduino.h>
#include <math.h>
#include <LiquidCrystal.h>
#include "Object.h"

#include <HX711.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"

// LCD Screen
const int rs = 41, en = 39, d4 = 37, d5 = 35, d6 = 33, d7 = 31;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
String lcd_current = "";

// IR Sensor
#define IR_SENSOR_PIN_1 6

// Weight Sensor
#define DOUT 3
#define CLK 2

// Category definitions
const int NUM_OF_CATEGORIES = 4;
String ObjectCategories[NUM_OF_CATEGORIES] = {"Apple", "Orange", "Pear", "Lemon"};

bool wait = false;

// Used for test code
int test_num = 0;

// Sensor MIN/MAX Values
const float WEIGHT_MAX = 300.0;
const float WEIGHT_MIN = 100.0;

const int COLOR_MAX = 255;
const int COLOR_MIN = 0;

// Weight Sensor
// Use HX711_Calibration sketch to define
float calibration_factor = -204340.00;
HX711 scale(DOUT, CLK);

// Color Sensor
byte gammatable[256];
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_60X);

// K Nearest Neighbors
#define K_Parameter 3

// Setup knownObjects
#define NUM_OF_KNOWN_OBJECTS 12
Object knownObjects[NUM_OF_KNOWN_OBJECTS];

// Rescale a value to 0-1 range
float RescaleValue(float value, const float min, const float max) {
    return (value-min)/(max-min);
}

// Rescale object features
Object RescaleObject(Object object) {
    Object rescaledObject;
    rescaledObject.category = object.category;
    rescaledObject.weight = RescaleValue(object.weight, WEIGHT_MIN, WEIGHT_MAX);
    rescaledObject.r = RescaleValue(object.r, COLOR_MIN, COLOR_MAX);
    rescaledObject.g = RescaleValue(object.g, COLOR_MIN, COLOR_MAX);
    rescaledObject.b = RescaleValue(object.b, COLOR_MIN, COLOR_MAX);
    return rescaledObject;
}

// Add an object to the known objects array
void AddToKnownObjects(int i, String category, float weight, float r, float g, float b) {
    knownObjects[i].category = category;
    knownObjects[i].weight = weight;
    knownObjects[i].r = r;
    knownObjects[i].g = g;
    knownObjects[i].b = b;
    knownObjects[i] = RescaleObject(knownObjects[i]);
}

// Insert all Known objects into the array
void PopulateKnownObjects() {
    AddToKnownObjects(0, "Apple", 228.0, 118, 76, 62);
    AddToKnownObjects(1, "Apple", 216.0, 111, 80, 63);
    AddToKnownObjects(2, "Apple", 224.0, 124, 80, 65);

    AddToKnownObjects(3, "Orange", 270.0, 255, 139, 85);
    AddToKnownObjects(4, "Orange", 257.0, 255, 140, 88);
    AddToKnownObjects(5, "Orange", 250.0, 255, 128, 85);
    
    AddToKnownObjects(6, "Lemon", 154.0, 255, 229, 127);
    AddToKnownObjects(7, "Lemon", 148.0, 255, 242, 139);
    AddToKnownObjects(8, "Lemon", 141.0, 255, 255, 141);
    
    AddToKnownObjects(9, "Pear", 217.0, 198, 183, 110);
    AddToKnownObjects(10, "Pear", 230.0, 172, 150, 90);
    AddToKnownObjects(11, "Pear", 222.0, 207, 190, 109);
}

/* PHASE 1: FEATURE EXTRACTION */

// Takes the features from the current object and converts them to strings and integers
Object FeatureExtraction() {
    Object inputObject;
    
    // Color sensor definitions
    uint16_t r, g, b, c, colorTemp, lux;

    // Get weight measurement (in lbs) to the nearest 100ths place
    inputObject.weight = (scale.get_units())*1000.0;

    // Get color measurement
    tcs.getRawData(&r, &g, &b, &c);
    colorTemp = tcs.calculateColorTemperature(r, g, b);
    lux = tcs.calculateLux(r, g, b);
    
    // c is clear and it is the sum of the red, green, and blue values.
    uint32_t sum = c;
    float red, green, blue;

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

    inputObject.r = (int)red;
    inputObject.g = (int)green;
    inputObject.b = (int)blue;
    
    Serial.print("r: ");
    Serial.println(inputObject.r);
    Serial.print("g: ");
    Serial.println(inputObject.g);
    Serial.print("b: ");
    Serial.println(inputObject.b);
    Serial.print("Weight: ");
    Serial.println(inputObject.weight);
    
    return RescaleObject(inputObject);
}

/* PHASE 2: CLASSIFICATION */

//  Computes the euclidean distance between the known and the current object's rgb values
float ComputeDistanceofColors(Object object1, Object object2) {
    float red = object1.r - object2.r;
    float green = object1.g - object2.g;
    float blue = object1.b - object2.b;
    float dist = pow(red, 2) + pow(green, 2) + pow(blue, 2);
    dist = sqrt(dist);
    return dist;
}

// Computes the euclidean distance between the known and the current object's features
float ComputeDistanceofObjects(Object inputObject, Object knownObject) {
    float weight = inputObject.weight - knownObject.weight;
    float color_dist = ComputeDistanceofColors(inputObject, knownObject);
    float dist = pow(weight, 2) + pow(color_dist, 2);
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

    // Weight
    scale.set_scale(calibration_factor);
    scale.tare();

    // Color
    if (tcs.begin()) {
        Serial.println("Found sensor");
    }
    else {
        Serial.println("No Color Sensor found ... check your connections");
        while (1);
    }

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
        inputObject.weight = 228.0;
        inputObject.r = 118;
        inputObject.g = 76;
        inputObject.b = 62;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 1: {
        inputObject.weight = 270.0;
        inputObject.r = 255;
        inputObject.g = 139;
        inputObject.b = 85;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 2: {
        inputObject.weight = 154.0;
        inputObject.r = 255;
        inputObject.g = 229;
        inputObject.b = 127;
        inputObject = RescaleObject(inputObject);
        test_num++;
        break;
      }
      case 3: {
        inputObject.weight = 217.0;
        inputObject.r = 198;
        inputObject.g = 183;
        inputObject.b = 110;
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
    
    bool Detection_Sensor;
    Detection_Sensor = digitalRead(IR_SENSOR_PIN_1);

    // Sensor was tripped and wasn't right before.
    if(Detection_Sensor == LOW && !wait) {
        Serial.println("Fruit Detected");

        delay(1000);

        // The built in LED is lit to show that the sensor was tripped.
        digitalWrite(LED_BUILTIN, HIGH);
        
        // Feature Extraction
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

#include <Arduino.h>
#include <math.h>
#include <LiquidCrystal.h>
#include "Object.h"

#include <NewPing.h>

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

// Length Sensor
#define TRIGGER_PIN_4 16
#define ECHO_PIN_4 15

#define TRIGGER_PIN_5 5
#define ECHO_PIN_5 4

// Category definitions
const int NUM_OF_CATEGORIES = 3;
String ObjectCategories[NUM_OF_CATEGORIES] = {"Small", "Medium", "Large"};

bool wait = false;

// Used for test code
int test_num = 0;

// Sensor MIN/MAX Values
const int DISTANCE_SENSOR_HEIGHT_MAX = 39;
const int DISTANCE_SENSOR_WIDTH_MAX = 45;
const int DISTANCE_SENSOR_LENGTH_MAX = 82;

const int DISTANCE_SENSOR_HEIGHT_MIN = 2;
const int DISTANCE_SENSOR_WIDTH_MIN = 1;
const int DISTANCE_SENSOR_LENGTH_MIN = 1;

// Distance sensor
NewPing sonar_height(TRIGGER_PIN_1, ECHO_PIN_1, DISTANCE_SENSOR_HEIGHT_MAX);
NewPing sonar_width_1(TRIGGER_PIN_2, ECHO_PIN_2, DISTANCE_SENSOR_WIDTH_MAX);
NewPing sonar_width_2(TRIGGER_PIN_3, ECHO_PIN_3, DISTANCE_SENSOR_WIDTH_MAX);
NewPing sonar_length_1(TRIGGER_PIN_4, ECHO_PIN_4, DISTANCE_SENSOR_LENGTH_MAX);
NewPing sonar_length_2(TRIGGER_PIN_5, ECHO_PIN_5, DISTANCE_SENSOR_LENGTH_MAX);

// K Nearest Neighbors
const int K = 3;

// Setup knownObjects
const int NUM_OF_KNOWN_OBJECTS = 9;
Object knownObjects[NUM_OF_KNOWN_OBJECTS];

/*
    Known Objects Preperation
*/

// Rescale a value to 0-1 range
float RescaleValue(float value, const float min, const float max) {
    return (value-min)/(max-min);
}

// Rescale object features
Object RescaleObject(Object object) {
    Object rescaledObject;
    rescaledObject.category = object.category;
    rescaledObject.height = RescaleValue(object.height, DISTANCE_SENSOR_HEIGHT_MIN, DISTANCE_SENSOR_HEIGHT_MAX);
    rescaledObject.width = RescaleValue(object.width, DISTANCE_SENSOR_WIDTH_MIN, DISTANCE_SENSOR_WIDTH_MAX);
    rescaledObject.length = RescaleValue(object.length, DISTANCE_SENSOR_LENGTH_MIN, DISTANCE_SENSOR_LENGTH_MAX);
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
    AddToKnownObjects(1, "Small", 10.0, 15.0, 15.0);
    AddToKnownObjects(2, "Small", 6.0, 17.0, 21.0);
    
    AddToKnownObjects(3, "Medium", 16.0, 20.0, 20.0);
    AddToKnownObjects(4, "Medium", 12.0, 18.0, 28.0);
    AddToKnownObjects(5, "Medium", 18.0, 18.0, 18.0);
    
    AddToKnownObjects(6, "Large", 20.0, 24.0, 34.0);
    AddToKnownObjects(7, "Large", 22.0, 22.0, 32.0);
    AddToKnownObjects(8, "Large", 24.0, 26.0, 36.0);
}

/*
    Feature Extraction
*/

// Takes the features from the current object and converts them to strings and integers
Object FeatureExtraction() {
    Object inputObject;
    
    int currHeight;
    int currWidth1;
    int currWidth2;
    int currLength1;
    int currLength2;

    // Get distance measurements
    // Get the median of 10 measurements
    // Convert the measurements from inch to cm
    int temp;
    delay(50);
    temp =  sonar_height.ping_median(10);
    currHeight = sonar_height.convert_cm(temp);
    delay(50);
    temp =  sonar_width_1.ping_median(10);
    currWidth1 = sonar_width_1.convert_cm(temp);
    delay(50);
    temp =  sonar_width_2.ping_median(10);
    currWidth2 = sonar_width_2.convert_cm(temp);
    delay(50);
    temp =  sonar_length_1.ping_median(10);
    currLength1 = sonar_length_1.convert_cm(temp) - 1;
    delay(50);
    temp =  sonar_length_2.ping_median(10);
    currLength2 = sonar_length_2.convert_cm(temp) - 1;
    
    // Convert values to actual height/width/length
    inputObject.height = DISTANCE_SENSOR_HEIGHT_MAX - currHeight;
    inputObject.width = DISTANCE_SENSOR_WIDTH_MAX - currWidth1 - currWidth2;
    inputObject.length = DISTANCE_SENSOR_LENGTH_MAX - currLength1 - currLength2;
    
    Serial.print("Height: ");
    Serial.println(inputObject.height);
    Serial.print("Width: ");
    Serial.println(inputObject.width);
    Serial.print("Length: ");
    Serial.println(inputObject.length);

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
    
    // Length
    pinMode(TRIGGER_PIN_4, OUTPUT);
    pinMode(ECHO_PIN_4, INPUT);
    pinMode(TRIGGER_PIN_5, OUTPUT);
    pinMode(ECHO_PIN_5, INPUT);

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
    
    return inputObject;
}
*/

void loop() {
    String closest_object_category;
    
    bool Detection_Sensor;
    Detection_Sensor = digitalRead(IR_SENSOR_PIN_1);

    // Sensor was tripped and wasn't right before.
    if(Detection_Sensor == LOW && !wait) {
        Serial.println("Box Detected");

        delay(2000);

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
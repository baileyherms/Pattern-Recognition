#include <Arduino.h>
#include <math.h>
// FIXME: Add the libraries needed for each pin

#include "Object.h"

// Define Pins for each sensor
// Distance Sensor (Height)
#define BUTTON1 6
#define BUTTON2 7
#define BUTTON3 8

int test = 0;

// Define anything else needed for sensors

// K Nearest Neighbors
// FIXME: Change when necessary
const int K = 3;

// FIXME: Change when necessary
const size_t NUM_OF_KNOWN_OBJECTS = 5;
const size_t NUM_OF_OBJECT_TYPES = 5;

const int PATTERN_LENGTH = 5;

// FIXME: Change object types
String Known_Object_Types[NUM_OF_OBJECT_TYPES] = {"Pat1", "Pat2", "Pat3", "Pat4", "Pat5"};
Object knownObjects[NUM_OF_KNOWN_OBJECTS];

float ObjectPatternsSame(Object currObject, Object knownObject) {
 // If the pattern arrays are the same
 if(memcmp(currObject.pattern, knownObject.pattern, PATTERN_LENGTH) == 0) {
    return true;
 }
 return false;
}

// Takes the features from the current object and converts them to strings and integers
void ObjectFeatureExtraction(Object &currObject) {
  int counter = 0;
  // Define necessary variables for height/weight/etc.
  
  // Read features from sensors (ex: sonar.ping_cm())
  
  // Assign each of those features to currObject.
  // Up to PATTERN_LENGTH button presses for pattern
  while(counter < PATTERN_LENGTH) {
    // Read the button values
    int but1 = digitalRead(BUTTON1);
    int but2 = digitalRead(BUTTON2);
    int but3 = digitalRead(BUTTON3);
    // if a button is pressed
    if(but1 == LOW) {
      currObject.pattern[counter] = 1;
      counter++;
    }
    else if(but2 == LOW) {
      currObject.pattern[counter] = 2;
      counter++;
    }
    else if(but3 == LOW) {
      currObject.pattern[counter] = 3;
      counter++;
    }
  }
}

// Finds the patterns from the current object and compares them to patterns from the given objects
String ObjectPatternRecognition(Object currObject, Object knownObjects[]) {
  
  for(int i = 0; i < NUM_OF_KNOWN_OBJECTS; ++i) { // For each remaining known object
    if(ObjectPatternsSame(currObject, knownObjects[i])) {
      currObject.type = knownObjects[i].type;
      return currObject.type;
    }
  }
  
  return "No Pattern Detected";
}

// Prints the type of object that is being passed through the project
void Actuation(String object) {
  if(object != "") {
    Serial.print("\t");
    Serial.println(object);
  }
}

void PopulateKnownObjects() {
  knownObjects[0].type = "Pat1";
  knownObjects[0].pattern[0] = 1;
  knownObjects[0].pattern[1] = 1;
  knownObjects[0].pattern[2] = 1;
  knownObjects[0].pattern[3] = 2;
  knownObjects[0].pattern[4] = 2;
  
  knownObjects[1].type = "Pat2";
  knownObjects[1].pattern[0] = 1;
  knownObjects[1].pattern[1] = 2;
  knownObjects[1].pattern[2] = 3;
  knownObjects[1].pattern[3] = 2;
  knownObjects[1].pattern[4] = 1;
  
  knownObjects[2].type = "Pat3";
  knownObjects[2].pattern[0] = 3;
  knownObjects[2].pattern[1] = 2;
  knownObjects[2].pattern[2] = 1;
  knownObjects[2].pattern[3] = 2;
  knownObjects[2].pattern[4] = 3;
  
  knownObjects[3].type = "Pat4";
  knownObjects[3].pattern[0] = 1;
  knownObjects[3].pattern[1] = 2;
  knownObjects[3].pattern[2] = 2;
  knownObjects[3].pattern[3] = 3;
  knownObjects[3].pattern[4] = 3;
  
  knownObjects[4].type = "Pat5";
  knownObjects[4].pattern[0] = 1;
  knownObjects[4].pattern[1] = 1;
  knownObjects[4].pattern[2] = 1;
  knownObjects[4].pattern[3] = 1;
  knownObjects[4].pattern[4] = 1;
}


void setup() {
  
  PopulateKnownObjects();
  
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);
  
  String closestObject;
  
  Serial.begin(9600);
}

void loop() {
  
  String closestObject;
  
  Object currObject;
    
  ObjectFeatureExtraction(currObject);

  closestObject = ObjectPatternRecognition(currObject, knownObjects);

  Actuation(closestObject);

  delay(1000);
}

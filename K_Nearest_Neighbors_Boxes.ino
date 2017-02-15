//Distance Sensor
//Feature is height
//Pre-known object of different heights
//Pattern is see what the object is closest to.
#include <Arduino.h>
#include <NewPing.h>

#include "Known_Objects.h"

#define TRIGGER_PIN 12
#define ECHO_PIN 11

// Make these const
#define MAX_DISTANCE 50
//K Nearest Neighbors
#define K 3

const size_t NUM_OF_KNOWN = 9;
const size_t NUM_OF_TYPES = 3;
String Known_Types[3] = {"Small", "Medium", "Large"};
Known_Objects Boxes[NUM_OF_KNOWN] = {{"Small", 11},
                        {"Small", 15},
                        {"Small", 12},
                        {"Medium", 27},
                        {"Medium", 29},
                        {"Medium", 30},
                        {"Large", 40},
                        {"Large", 42},
                        {"Large", 42}};


NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

struct Features {
  int height;
};

Features Feature_Extraction() {
  Features features;
  //Get height

  int dist = sonar.ping_cm();
  
  features.height = MAX_DISTANCE - dist;
  
  return features; 
}

String Pattern_Recognition(Features features) {

  Known_Objects diff_Boxes[9];
  
  String closest_object;

  Serial.print("Object height: ");
  Serial.println(features.height);

  //k nearest neighbors

  //Checks the min difference between two points
  int min_difference = 100;
  int pos_min_difference[K];

  /*
  String temp_type;
  int temp_height;
  for(int i = 0; i < NUM_OF_KNOWN; ++i) {
    for(int j = 0; j < NUM_OF_KNOWN - i - 1; ++j) {
      if(Boxes[j].height > Boxes[j+1].height) {
        temp_type = Boxes[j].type;
        temp_height = Boxes[j].height;
        Boxes[j].type = Boxes[j+1].type;
        Boxes[j].height = Boxes[j+1].height;
        Boxes[j+1].type = temp_type;
        Boxes[j+1].height = temp_height;
      }
    }
  }
  */

  //MOST NEW CODE STARTS HERE
  //Find differences between distance and known box sizes and store in difference array
  for(int i = 0; i < NUM_OF_KNOWN; ++i) {
    /*
    if(abs(Boxes[i].height - features.height) <= min_difference) {
      //min_difference = abs(Boxes[i].height - features.height);
      //pos_min_difference[0] = i;
      
    }
    */
    diff_Boxes[i].type = Boxes[i].type;
    diff_Boxes[i].height = abs(Boxes[i].height - features.height);
  }

  String temp_type;
  int temp_height;
  //Sort the difference array
  for(int i = 0; i < NUM_OF_KNOWN; ++i) {
    for(int j = 0; j < NUM_OF_KNOWN - i - 1; ++j) {
      if(diff_Boxes[j].height > diff_Boxes[j+1].height) {
        temp_type = diff_Boxes[j].type;
        temp_height = diff_Boxes[j].height;
        diff_Boxes[j].type = diff_Boxes[j+1].type;
        diff_Boxes[j].height = diff_Boxes[j+1].height;
        diff_Boxes[j+1].type = temp_type;
        diff_Boxes[j+1].height = temp_height;
      }
    }
  }

  //Create new array holding only K elements
  Known_Objects k_Nearest_Boxes[K];
  for(int i = 0; i < K; ++i) {
    k_Nearest_Boxes[i].type = diff_Boxes[i].type;
    k_Nearest_Boxes[i].height = diff_Boxes[i].height;
  }

  int count = 0;
  int max_count = 0;
  String max_type;
  //Find out which box type occurs the most
  for(int i = 0; i < NUM_OF_TYPES; ++i) {
    for(int j = 0; j < NUM_OF_TYPES; ++j) {
      if(k_Nearest_Boxes[j].type == Known_Types[i]) {
        count++;
      }
    }
    if(count > max_count) {
      max_count = count;
      max_type = Known_Types[i];
    }
  }

  closest_object = max_type;
  
  return closest_object;
}

void Actuation(String object) {
  if(object != "") {
    Serial.print("\t");
    Serial.println(object);
  }
}



void setup() {
  
  Serial.begin(9600);
  
}

void loop() {
  //Turns both LEDs off before each reading

  //Should put feature extraction and pattern recognition in one multi-second for loop to
    //give the box time to go through the sensors.

    
  Features feature_states;
  feature_states = Feature_Extraction();

  String closest_object;
  closest_object = Pattern_Recognition(feature_states);

  Actuation(closest_object);

  delay(1000);
}

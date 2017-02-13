//Distance Sensor
//Feature is height
//Pre-known object of different heights
//Pattern is see what the object is closest to.

#include <NewPing.h>

#define TRIGGER_PIN 12
#define ECHO_PIN 11
#define MAX_DISTANCE 50
//K Nearest Neighbors
#define K 1

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

struct Features {
  int height;
};

//0 = small
//1 = medium
//2 = large

//Types of items being measured (rows)
const int boxes_x = 3;
//Amount of each item type that is given for k nearest neighbor (columns)
const int boxes_y = 3;
int Boxes[boxes_x][boxes_y] = {
  {11, 15, 12},
  {27, 29, 30},
  {40, 42, 42}
};

Features Feature_Extraction() {
  Features features;
  //Get height

  int dist = sonar.ping_cm();
  
  features.height = MAX_DISTANCE - dist;
  
  return features; 
}

String Pattern_Recognition(Features features) {
  //Banana is 3-7cm (3 <= x < 7)
  //Apple-Orange height is 7-11cm (7 <= x < 11)
  //Small box is 11-25cm (11 <= x < 25)
  //Medium box is 25-35cm (25 <= x < 35)
  //Big box is 35-45cm (35 <= x < 45)
  //Pre-known object of different heights
  //Pattern is see what the object is closest to.

  String closest_object;

  Serial.print("Object height: ");
  Serial.println(features.height);

  //k nearest neighbors

  //Checks the min difference between two points
  int min_difference = 100;

  //Position of min_difference
  int pos_min_difference[K];

  for(int i = 0; i < boxes_x; ++i) {
    for(int j = 0; j < boxes_y; ++j) {
      if(abs(Boxes[i][j] - features.height) <= min_difference) {
        min_difference = abs(Boxes[i][j] - features.height);
        pos_min_difference[0] = i;
      }
    }
  }

  //Serial.println(min_difference);
  //Serial.println(pos_min_difference);

  if(pos_min_difference[0] == 0) {
    closest_object = "Small Box";
  }
  else if(pos_min_difference[0] == 1) {
    closest_object = "Medium Box";
  }
  else if(pos_min_difference[0] == 2) {
    closest_object = "Large Box";
  }
  
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

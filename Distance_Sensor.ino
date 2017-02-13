//Distance Sensor
//Feature is height
//Pre-known object of different heights
//Pattern is see what the object is closest to.

#include <NewPing.h>

#define TRIGGER_PIN 12
#define ECHO_PIN 11
#define MAX_DISTANCE 50
#define K_NEAREST_NEIGHBORS 1

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

  if(features.height >= 3 && features.height < 7) {
    closest_object = "Banana";
  }
  else if(features.height >= 7 && features.height < 11) {
    closest_object = "Apple or Orange";
  }
  else if(features.height >= 11 && features.height < 25) {
    closest_object = "Small box";
  }
  else if(features.height >= 25 && features.height < 35) {
    closest_object = "Medium box";
  }
  else if(features.height >= 35 && features.height < 45) {
    closest_object = "Large box";
  }
  else {
    closest_object = "";
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

#include <Wire.h>
#include "Adafruit_TCS34725.h"

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

struct Features {
  bool lt_10in;
  uint16_t red;
  uint16_t green;
  uint16_t blue;
};

Features Feature_Extraction() {
  Features features;
  //Need a fourth value for getRawData()
  uint16_t hold;

  tcs.getRawData(&features.red, &features.green, &features.blue, &hold);
  //colorTemp = tcs.calculateColorTemperature(&features.red, &features.green, &features.blue);
  //lux = tcs.calculateLux(&features.red, &features.green, &features.blue);

  //Delete later, use for testing
  Serial.print("R: "); Serial.print(features.red, DEC); Serial.print(" ");
  Serial.print("G: "); Serial.print(features.green, DEC); Serial.print(" ");
  Serial.print("B: "); Serial.print(features.blue, DEC); Serial.print(" ");
  Serial.println(" ");
  
  return features; 
}

String Pattern_Recognition(Features features) {
  String fruit;

  //FIXME: k nearest neighbor algorithm

  return fruit;
}

void Actuation(String box_size) {

}

void setup() {

  Serial.begin(9600);

  if(tcs.begin()) {
    Serial.println("Found sensor");
  }
  else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }
}

void loop() {
  
  //Should put feature extraction and pattern recognition in one multi-second for loop to
    //give the box time to go through the sensors.
  Features feature_states;
  feature_states = Feature_Extraction();

  String fruit;
  fruit = Pattern_Recognition(feature_states);

  Actuation(fruit);

  delay(1000);
  
}

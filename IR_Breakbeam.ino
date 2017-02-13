#define LED_SMALL_PIN 14
#define LED_LARGE_PIN 15
#define IR_SENSOR_PIN_1 2
#define IR_SENSOR_PIN_2 3

bool IR_Sensor1;
bool IR_Sensor2;

struct Features {
  bool lt_10in;
};

Features Feature_Extraction() {
  Features features;
  
  IR_Sensor1 = digitalRead(IR_SENSOR_PIN_1);
  IR_Sensor2 = digitalRead(IR_SENSOR_PIN_2);

  //Do the state machine thing where once the first one is
    //broken see if the second one is broken at the same time.
    //Wait a few seconds.

  if(IR_Sensor1 == LOW && IR_Sensor2 == LOW) {
    features.lt_10in = false;
  }
  //If one beam is broken
  else if(IR_Sensor1 == LOW && IR_Sensor2 == HIGH) {
    features.lt_10in = true;
  }
  else if(IR_Sensor1 == HIGH && IR_Sensor2 == LOW) {
    features.lt_10in = true;
  }
  //If neither beam is broken
  else {
    //box_size = "None";
    features.lt_10in = true;
  }
  
  return features; 
}

String Pattern_Recognition(Features features) {
  String box_size;

  //If both beams are broken
  if(features.lt_10in) {
    box_size = "Less than 10 inches.";
  }
  else {
    box_size = "Greater than 10 inches.";
  }
  return box_size;
}

void Actuation(String box_size) {
  Serial.println(box_size);
  if(box_size == "Less than 10 inches.") {
    digitalWrite(LED_SMALL_PIN, HIGH);
  }
  else if(box_size == "Greater than 10 inches.") {
    digitalWrite(LED_LARGE_PIN, HIGH);
  }
  else if(box_size == "None") {
    //Both LEDs are off
  }
}

void setup() {
  pinMode(LED_SMALL_PIN, OUTPUT);
  pinMode(LED_LARGE_PIN, OUTPUT);
  
  pinMode(IR_SENSOR_PIN_1, INPUT);
  digitalWrite(IR_SENSOR_PIN_1, HIGH);
  pinMode(IR_SENSOR_PIN_2, INPUT);
  digitalWrite(IR_SENSOR_PIN_2, HIGH);

  Serial.begin(9600);
}

void loop() {
  //Turns both LEDs off before each reading
  digitalWrite(LED_SMALL_PIN, LOW);
  digitalWrite(LED_LARGE_PIN, LOW);

  //Should put feature extraction and pattern recognition in one multi-second for loop to
    //give the box time to go through the sensors.
  Features feature_states;
  feature_states = Feature_Extraction();

  String pattern_size;
  pattern_size = Pattern_Recognition(feature_states);

  Actuation(pattern_size);

  delay(1000);
}

#define LED_SMALL_PIN 14
#define LED_LARGE_PIN 15
#define IR_SENSOR_PIN_1 4
#define IR_SENSOR_PIN_2 3

struct Features {
  bool IR_Sensor1;
  bool IR_Sensor2;
};

Features Feature_Extraction() {
  Features features;
  
  features.IR_Sensor1 = digitalRead(IR_SENSOR_PIN_1);
  features.IR_Sensor2 = digitalRead(IR_SENSOR_PIN_2);

  return features; 
}

String Pattern_Recognition(Features features) {
  String box_size;

  //If both beams are broken
  if(features.IR_Sensor1 == LOW && features.IR_Sensor2 == LOW) {
    box_size = "Large";
  }
  //If one beam is broken
  else if(features.IR_Sensor1 == LOW && features.IR_Sensor2 == HIGH) {
    box_size = "Small";
  }
  else if(features.IR_Sensor1 == HIGH && features.IR_Sensor2 == LOW) {
    box_size = "Small";
  }
  //If neither beam is broken
  else {
    box_size = "None";
  }
  return box_size;
}

void Accuation(String box_size) {
  Serial.println(box_size);
  if(box_size == "Small") {
    digitalWrite(LED_SMALL_PIN, HIGH);
  }
  else if(box_size == "Large") {
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

  Accuation(pattern_size);

  delay(1000);
}

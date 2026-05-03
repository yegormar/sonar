// ============================================================
//  Ultrasonic Sensor Cone Test
//  Tests detection range and visibility cone for each sensor
//  Displays visible distance every 1 second
// ============================================================

const int TRIG_L = 9;
const int ECHO_L = 10;
const int TRIG_R = 7;
const int ECHO_R = 8;
const int RED_LED = 4;    // Right indicator
const int BLUE_LED = 5;   // Left indicator

unsigned long lastDisplay = 0;
bool testingLeft = true;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  Serial.println(F("Ultrasonic Sensor Cone Test"));
  Serial.println(F("========================================"));
  Serial.println(F("Testing LEFT sensor (BLUE LED lit)"));
  Serial.println(F("Press 'R' to switch to RIGHT sensor"));
  Serial.println(F("Distance updates every 1 second"));
  Serial.println();
}

void loop() {
  // Check for manual sensor switch commands
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    if (command == 'l' || command == 'L') {
      testingLeft = true;
      Serial.println();
      Serial.println(F("========================================"));
      Serial.println(F("Now testing LEFT sensor"));
      Serial.println(F("Press 'R' to switch to RIGHT sensor"));
      Serial.println();
    } else if (command == 'r' || command == 'R') {
      testingLeft = false;
      Serial.println();
      Serial.println(F("========================================"));
      Serial.println(F("Now testing RIGHT sensor"));
      Serial.println(F("Press 'L' to switch to LEFT sensor"));
      Serial.println();
    }
  }
  
  // Keep appropriate LED lit to show which sensor is active
  if (testingLeft) {
    digitalWrite(BLUE_LED, HIGH); // BLUE = LEFT sensor active
    digitalWrite(RED_LED, LOW);
  } else {
    digitalWrite(RED_LED, HIGH); // RED = RIGHT sensor active
    digitalWrite(BLUE_LED, LOW);
  }
  
  // Display visible distance every 1 second
  if (millis() - lastDisplay >= 1000) {
    lastDisplay = millis();
    
    float distance = -1.0;
    if (testingLeft) {
      distance = measureDistance(TRIG_L, ECHO_L);
    } else {
      distance = measureDistance(TRIG_R, ECHO_R);
    }
    
    if (distance > 0) {
      if (testingLeft) {
        Serial.print(F("LEFT SENSOR: "));
      } else {
        Serial.print(F("RIGHT SENSOR: "));
      }
      Serial.print(distance, 1);
      Serial.println(F(" cm visible"));
    } else {
      if (testingLeft) {
        Serial.println(F("LEFT SENSOR: No detection"));
      } else {
        Serial.println(F("RIGHT SENSOR: No detection"));
      }
    }
  }
}

float measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout ~5m
  if (duration == 0) return -1.0;
  
  return duration * 0.01716; // Convert to cm
}
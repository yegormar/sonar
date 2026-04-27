// ============================================================
//  Sensor Visibility Cone Test
//  Tests each ultrasonic sensor separately to determine:
//  - Maximum detection range
//  - Detection cone angle
//  - Reliability at different distances
// ============================================================

const int TRIG_L = 9;
const int ECHO_L = 10;
const int TRIG_R = 7;
const int ECHO_R = 8;
const int RED_LED = 4;
const int BLUE_LED = 5;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  Serial.println(F("Sensor Visibility Test"));
  Serial.println(F("======================="));
  Serial.println(F("Testing LEFT sensor first..."));
  Serial.println(F("Move object around to test detection range"));
  Serial.println(F("Press any key to switch to RIGHT sensor"));
}

void loop() {
  // Test LEFT sensor
  while (!Serial.available()) {
    digitalWrite(RED_LED, HIGH); // RED = LEFT sensor active
    float leftDist = measureDistance(TRIG_L, ECHO_L);
    digitalWrite(RED_LED, LOW);
    
    if (leftDist > 0) {
      Serial.print(F("LEFT: ")); Serial.print(leftDist, 1); Serial.println(F(" cm"));
    } else {
      Serial.println(F("LEFT: No detection"));
    }
    delay(200);
  }
  
  // Clear serial buffer
  while (Serial.available()) Serial.read();
  
  Serial.println(F("\nNow testing RIGHT sensor..."));
  Serial.println(F("Move object around to test detection range"));
  Serial.println(F("Press any key to restart test"));
  
  // Test RIGHT sensor
  while (!Serial.available()) {
    digitalWrite(BLUE_LED, HIGH); // BLUE = RIGHT sensor active
    float rightDist = measureDistance(TRIG_R, ECHO_R);
    digitalWrite(BLUE_LED, LOW);
    
    if (rightDist > 0) {
      Serial.print(F("RIGHT: ")); Serial.print(rightDist, 1); Serial.println(F(" cm"));
    } else {
      Serial.println(F("RIGHT: No detection"));
    }
    delay(200);
  }
  
  // Clear serial buffer
  while (Serial.available()) Serial.read();
  Serial.println(F("\nRestarting test..."));
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
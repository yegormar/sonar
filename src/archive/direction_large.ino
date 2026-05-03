// Direction Monitor - LARGE ROOM VERSION
// Increased timeout for larger spaces
// No distance cutoffs

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

const float SENSOR_SPACING = 12.0;
const unsigned long PULSE_TIMEOUT = 100000; // 100ms = ~17 meters max range

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for serial to initialize
  
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  Serial.println(F("ms, L, R, angle"));
}

float readSensor(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  // Increased timeout for large rooms
  long dur = pulseIn(echo, HIGH, PULSE_TIMEOUT);
  
  if (dur == 0) {
    // No echo detected - return maximum range
    return 1000.0; // Indicates no detection (beyond max range)
  }
  
  return dur * 0.01716; // Convert to cm
}

void loop() {
  float L = readSensor(TRIG_L, ECHO_L);
  float R = readSensor(TRIG_R, ECHO_R);

  // Output data regardless of distance
  float diff = L - R;
  float angle = diff / SENSOR_SPACING;

  // LED control
  if (L >= 1000.0 || R >= 1000.0) {
    // No detection - both LEDs off
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(RED_LED, LOW);
  } else if (angle < -10.0) {
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else if (angle > 10.0) {
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  } else {
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
  }

  // Always output data
  Serial.print(millis());
  Serial.print(F(","));
  Serial.print(L, 1);
  Serial.print(F(","));
  Serial.print(R, 1);
  Serial.print(F(","));
  Serial.println(angle, 1);

  delay(50);
}

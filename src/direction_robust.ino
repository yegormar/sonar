// Direction Monitor - ROBUST VERSION
// Simple, reliable output with proper formatting

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

const float PRINT_FLOOR = 200.0;
const float SENSOR_SPACING = 12.0;

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for serial to initialize
  
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  // Simple header
  Serial.println(F("ms, L, R, angle"));
}

float readSensor(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 35000);
  if (dur == 0) return -1.0;
  return dur * 0.01716;
}

void loop() {
  float L = readSensor(TRIG_L, ECHO_L);
  float R = readSensor(TRIG_R, ECHO_R);

  if (L < 0 || R < 0) {
    // Error - output special values
    Serial.println(F("-1,-1,-1"));
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    delay(50);
    return;
  }

  float diff = L - R;
  float angle = diff / SENSOR_SPACING;

  // LED control
  if (angle < -10.0) {
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else if (angle > 10.0) {
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  } else {
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
  }

  // Simple, reliable output
  Serial.print(millis());
  Serial.print(F(","));
  Serial.print(L, 1);
  Serial.print(F(","));
  Serial.print(R, 1);
  Serial.print(F(","));
  Serial.println(angle, 1);

  delay(50);
}

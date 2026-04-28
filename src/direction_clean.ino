// Direction Monitor - CLEAN OUTPUT VERSION
// Minimal output: timestamp, distances, angle in degrees
// Center = 0°, Left = negative°, Right = positive°

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

const float PRINT_FLOOR = 200.0;
const float SENSOR_SPACING = 12.0; // cm between sensors

// Smoothing
const int WINDOW_SIZE = 5;
float diffHistory[5] = {0};
int historyIndex = 0;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  Serial.println(F("ms,L,R,angle_deg"));
}

float readSensor(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 35000);
  if (dur == 0) return -1.0;
  return dur * 0.01716;
}

float smoothDiff(float newDiff) {
  diffHistory[historyIndex] = newDiff;
  historyIndex = (historyIndex + 1) % WINDOW_SIZE;
  
  float sum = 0;
  int count = min(historyIndex + 1, WINDOW_SIZE);
  for (int i = 0; i < count; i++) {
    sum += diffHistory[i];
  }
  return sum / count;
}

float calculateAngle(float diff) {
  // Simple angle approximation: diff / sensor_spacing
  // Returns degrees: negative = left, positive = right, 0 = center
  return diff / SENSOR_SPACING;
}

void loop() {
  float L = readSensor(TRIG_L, ECHO_L);
  float R = readSensor(TRIG_R, ECHO_R);

  if (L < 0 || R < 0) {
    // Error state - output special values
    Serial.println(F("ERROR"));
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    return;
  }

  float diff = L - R;
  float smoothedDiff = smoothDiff(diff);
  float angle = calculateAngle(smoothedDiff);

  // LED control based on angle
  if (angle < -10.0) {
    // Left region
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else if (angle > 10.0) {
    // Right region
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  } else {
    // Center region
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
  }

  // Clean output: timestamp, L, R, angle
  Serial.print(millis());
  Serial.print(F(","));
  Serial.print(L, 1);
  Serial.print(F(","));
  Serial.print(R, 1);
  Serial.print(F(","));
  Serial.println(angle, 1);

  delay(50);
}

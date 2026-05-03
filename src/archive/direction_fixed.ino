// Direction Monitor - FIXED WIDTH OUTPUT
// Each field has consistent width for vertical alignment

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

  // Header with fixed width
  Serial.println(F("   TIME    L    R  ANGLE"));
  Serial.println(F("  ------ ----- ----- -----"));
}

float readSensor(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 35000);
  if (dur == 0) return -1.0;
  return dur * 0.01716;
}

void printFixedWidth(long ms, float L, float R, float angle) {
  // Format: 7 chars for time, 6 for distances, 7 for angle
  char buffer[30];
  
  // Handle negative angles properly
  char angleSign = ' ';
  if (angle < 0) {
    angleSign = '-';
    angle = -angle;
  }
  
  snprintf(buffer, sizeof(buffer), 
           "%7lu %5.1f %5.1f %c%5.1f",
           ms, L, R, angleSign, angle);
  
  Serial.println(buffer);
}

void loop() {
  float L = readSensor(TRIG_L, ECHO_L);
  float R = readSensor(TRIG_R, ECHO_R);

  if (L < 0 || R < 0) {
    // Error - output special values with fixed width
    Serial.println(F("  ERROR  -1.0  -1.0  -1.0"));
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

  printFixedWidth(millis(), L, R, angle);

  delay(50);
}

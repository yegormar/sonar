// Direction Monitor - SMOOTH ANGLE TRACKING
// Continuous angle detection with smooth state transitions
// For gradual feedback from far left → center → far right
//
// Key Features:
//   - Continuous angle calculation (not just discrete states)
//   - Moving average for smooth angle changes
//   - Hysteresis zones for stable regions
//   - Gradual LED brightness transitions
//   - Time-based smoothing for all changes

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

const float PRINT_FLOOR = 200.0;

// Continuous angle tracking
const float SENSOR_SPACING = 12.0; // cm between sensors

// Smoothing parameters
const int ANGLE_WINDOW = 5; // Samples for angle averaging
float angleHistory[5] = {0};
int angleIndex = 0;
int angleCount = 0;

// State zones (in degrees from center)
const float LEFT_ZONE_START = 30.0;  // Entering left region
const float CENTER_ZONE = 15.0;     // Center dead zone
const float SMOOTH_TIME = 300;       // ms for smooth transitions

// Current state tracking
float currentAngle = 0.0;      // -90 (far left) to +90 (far right)
float targetAngle = 0.0;       // Where we're moving to
unsigned long transitionStart = 0;
bool inTransition = false;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  Serial.println(F("Direction Monitor - SMOOTH ANGLE"));
  Serial.println(F("ts(ms),L(cm),R(cm),diff,angle,zone,leds"));
}

float readSensor(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 35000);
  if (dur == 0) return -1.0;
  return dur * 0.01716;
}

float calculateAngle(float L, float R) {
  // Simple angle approximation based on difference
  // More negative = further left, more positive = further right
  float diff = L - R;
  
  // Convert difference to angle (-90 to +90 degrees)
  // Normalized to sensor spacing
  float angle = diff / SENSOR_SPACING;
  
  // Limit to realistic range
  if (angle < -90) angle = -90;
  if (angle > 90) angle = 90;
  
  return angle;
}

float smoothAngle(float newAngle) {
  // Add to history
  angleHistory[angleIndex] = newAngle;
  angleIndex = (angleIndex + 1) % ANGLE_WINDOW;
  if (angleCount < ANGLE_WINDOW) angleCount++;
  
  // Calculate moving average
  float sum = 0;
  int count = min(angleCount, ANGLE_WINDOW);
  for (int i = 0; i < count; i++) {
    sum += angleHistory[i];
  }
  return sum / count;
}

String getZone(float angle) {
  if (angle < -LEFT_ZONE_START) return "FAR_LEFT";
  if (angle < -CENTER_ZONE) return "LEFT";
  if (angle > CENTER_ZONE) return "RIGHT";
  if (angle > LEFT_ZONE_START) return "FAR_RIGHT";
  return "CENTER";
}

void smoothLEDTransition(float currentAngle) {
  // Calculate LED brightness based on angle
  // Left LED: bright when angle negative
  // Right LED: bright when angle positive
  
  int leftBrightness = 255;
  int rightBrightness = 255;
  
  if (currentAngle < 0) {
    // In left region
    leftBrightness = 255;
    rightBrightness = map(abs(currentAngle), 0, 90, 255, 50);
  } else if (currentAngle > 0) {
    // In right region
    leftBrightness = map(currentAngle, 0, 90, 255, 50);
    rightBrightness = 255;
  }
  
  // Constrain to valid PWM range
  leftBrightness = constrain(leftBrightness, 50, 255);
  rightBrightness = constrain(rightBrightness, 50, 255);
  
  analogWrite(RED_LED, rightBrightness);
  analogWrite(BLUE_LED, leftBrightness);
}

void loop() {
  float L = readSensor(TRIG_L, ECHO_L);
  float R = readSensor(TRIG_R, ECHO_R);

  if (L < 0 || R < 0) {
    Serial.println(F("ERROR"));
    analogWrite(RED_LED, 0);
    analogWrite(BLUE_LED, 0);
    return;
  }

  // Calculate current angle
  float rawAngle = calculateAngle(L, R);
  float smoothedAngle = smoothAngle(rawAngle);
  
  // Smooth transition to new angle
  if (!inTransition || abs(smoothedAngle - targetAngle) > 5.0) {
    targetAngle = smoothedAngle;
    transitionStart = millis();
    inTransition = true;
  }
  
  // Gradually move to target angle
  float elapsed = millis() - transitionStart;
  float progress = min(1.0, elapsed / SMOOTH_TIME);
  currentAngle = currentAngle + (targetAngle - currentAngle) * progress;
  
  if (progress >= 1.0) {
    inTransition = false;
  }
  
  // Update LEDs smoothly
  smoothLEDTransition(currentAngle);
  
  // Print data
  Serial.print(millis()); Serial.print(",");
  Serial.print(L, 1); Serial.print(",");
  Serial.print(R, 1); Serial.print(",");
  Serial.print(L - R, 1); Serial.print(",");
  Serial.print(currentAngle, 1); Serial.print(",");
  Serial.print(getZone(currentAngle)); Serial.print(",");
  Serial.println(currentAngle < 0 ? "BLUE" : "RED");
  
  delay(50); // Consistent sampling
}

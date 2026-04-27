// ============================================================
//  Sonar System — Main Sketch
//  Uses calibration constants for spatial feedback
//  Non-blocking buzzer for responsive detection
// ============================================================

// === CALIBRATION CONSTANTS — from calibration process ===
const float CAL_NEAR       = 59.92;  // cm — constant beep threshold
const float CAL_FAR        = 171.00;  // cm — beeping starts here
const float CAL_DIFF_LEFT  = -85.01;   // L-R cm difference at max left
const float CAL_DIFF_RIGHT = 70.49;   // L-R cm difference at max right
// =======================================================

// === SYMMETRICAL CALIBRATION ADJUSTMENT ===
// Use symmetrical range based on maximum observed difference
const float MAX_DIFF_MAGNITUDE = max(abs(CAL_DIFF_LEFT), abs(CAL_DIFF_RIGHT));
const float SYMMETRICAL_LEFT  = -MAX_DIFF_MAGNITUDE;  // Force symmetry
const float SYMMETRICAL_RIGHT = MAX_DIFF_MAGNITUDE;   // Force symmetry
// ===========================================

// --- Pin definitions ---
const int TRIG_L = 9;
const int ECHO_L = 10;
const int TRIG_R = 7;
const int ECHO_R = 8;
const int BUZZER = 6;
const int RED_LED = 4;    // Right indicator
const int BLUE_LED = 5;   // Left indicator

// --- System parameters ---
const int SAMPLES = 2;    // reduced samples for faster measurement
const float DEAD_ZONE_ANGLE = 15.0; // degrees for center LED activation

// --- Non-blocking buzzer state ---
unsigned long lastBeepTime = 0;
int currentBeepInterval = 0;
int currentFrequency = 0;
bool buzzerEnabled = false;

// ============================================================
void setup() {
  Serial.begin(9600);
  
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  // Start silent
  noTone(BUZZER);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
}

// ============================================================
//  Sensor Reading Functions
// ============================================================

float measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Use longer timeout for better reliability at 1.5m+ distances
  long duration = pulseIn(echoPin, HIGH, 35000); // 35ms timeout ~6m
  
  if (duration == 0) return -1.0;
  
  return duration * 0.01716; // Convert to cm
}

float getAverageDistance(int trigPin, int echoPin) {
  float sum = 0;
  int count = 0;
  
  for (int i = 0; i < SAMPLES; i++) {
    float distance = measureDistance(trigPin, echoPin);
    if (distance > 0) {
      sum += distance;
      count++;
    }
    delay(20); // Let echoes die out before next sample (enough for 170cm = ~10ms round trip)
  }
  
  return (count == 0) ? -1.0 : sum / count;
}

// ============================================================
//  Spatial Calculation Functions
// ============================================================

void calculateSpatialFeedback(float L, float R) {
  if (L < 0 || R < 0) {
    // No valid reading - turn off outputs
    noTone(BUZZER);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    buzzerEnabled = false;
    return;
  }

  // Calculate proximity (distance feedback)
  float proximity = min(L, R);

  // If proximity exceeds CAL_FAR, the target is outside the active zone.
  // Stay silent and turn off LEDs.
  if (proximity > CAL_FAR) {
    noTone(BUZZER);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    buzzerEnabled = false;
    return;
  }

  // Calculate differential for direction
  float diff = L - R;

  // Map differential to angle using forced symmetrical range
  float normalizedDiff = mapFloat(diff, SYMMETRICAL_LEFT, SYMMETRICAL_RIGHT, -1.0, 1.0);
  normalizedDiff = constrain(normalizedDiff, -1.0, 1.0);
  float angleDeg = normalizedDiff * -60.0; // INVERTED: +60° to -60° range

  // Fixed dead zone for predictable left/right/center behavior
  float dynamicDeadZone = DEAD_ZONE_ANGLE;
  
  // Map proximity to beep interval
  if (proximity <= CAL_NEAR) {
    currentBeepInterval = 100; // Constant beeping
  } else {
    // Linear mapping between CAL_NEAR and CAL_FAR
    float range = CAL_FAR - CAL_NEAR;
    float posInRange = proximity - CAL_NEAR;
    float pct = posInRange / range;
    currentBeepInterval = map(pct * 100, 0, 100, 100, 1000);
  }
  
  // Map angle to frequency
  currentFrequency = map(angleDeg, -60, 60, 200, 2000);
  buzzerEnabled = true;
  
  // Control LEDs based on direction (immediate, no blocking)
  if (angleDeg > dynamicDeadZone) {
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);
  } else if (angleDeg < -dynamicDeadZone) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
  } else {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, HIGH);
  }
}

// Non-blocking buzzer update
void updateBuzzer() {
  if (!buzzerEnabled) return;
  
  unsigned long now = millis();
  if (now - lastBeepTime >= (unsigned long)currentBeepInterval) {
    lastBeepTime = now;
    tone(BUZZER, currentFrequency, 50); // 50ms tone, non-blocking
  }
}

// Helper function for float mapping
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ============================================================
//  Main Loop
// ============================================================

void loop() {
  // Read sensors continuously — no blocking delay
  float leftDist = getAverageDistance(TRIG_L, ECHO_L);
  float rightDist = getAverageDistance(TRIG_R, ECHO_R);
    
  // Update LEDs and buzzer state immediately
  calculateSpatialFeedback(leftDist, rightDist);
  
  // Handle beeping in background without blocking
  updateBuzzer();
}

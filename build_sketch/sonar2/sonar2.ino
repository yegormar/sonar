// ============================================================
//  Sonar System v2 — Direction via BEEP DURATION, Proximity via RATE
//
//  Hardware reality: a piezo buzzer on a digital pin is ON/OFF only.
//  You cannot change its true volume/loudness.
//  What we CAN change is the duty cycle at a macro level:
//    - Centered object  = long beep, sounds "loud / present"
//    - Off-center object = short tick, sounds "quiet / faint"
//    - Proximity        = controls the interval between beeps (fast = close)
//
//  How to use:
//    1. Rotate left/right until beeps sound fullest (longest).
//    2. Beep speed tells you how close the object is.
// ============================================================

// === CALIBRATION CONSTANTS ===
const float CAL_NEAR       = 59.92;
const float CAL_FAR        = 171.00;
const float CAL_DIFF_LEFT  = -85.01;
const float CAL_DIFF_RIGHT = 70.49;
// ==============================

const float MAX_DIFF_MAGNITUDE = max(abs(CAL_DIFF_LEFT), abs(CAL_DIFF_RIGHT));
const float SYMMETRICAL_LEFT  = -MAX_DIFF_MAGNITUDE;
const float SYMMETRICAL_RIGHT = MAX_DIFF_MAGNITUDE;

// --- Pins ---
const int TRIG_L = 9;
const int ECHO_L = 10;
const int TRIG_R = 7;
const int ECHO_R = 8;
const int BUZZER = 6;
const int RED_LED = 4;
const int BLUE_LED = 5;

// --- Parameters ---
const int SAMPLES = 2;
const float DEAD_ZONE_ANGLE = 15.0;
const int BASE_FREQ = 1000;       // Constant pitch
const int MIN_BEEP_MS = 20;       // Shortest audible tick

// --- Non-blocking state ---
unsigned long lastBeepTime = 0;
int currentBeepInterval = 0;
int currentBeepDuration = 0;
bool buzzerEnabled = false;

// ============================================================
void setup() {
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  noTone(BUZZER);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
}

// ============================================================
float measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 35000);
  if (duration == 0) return -1.0;
  return duration * 0.01716;
}

float getAverageDistance(int trigPin, int echoPin) {
  float sum = 0; int count = 0;
  for (int i = 0; i < SAMPLES; i++) {
    float d = measureDistance(trigPin, echoPin);
    if (d > 0) { sum += d; count++; }
    delay(20);
  }
  return (count == 0) ? -1.0 : sum / count;
}

// ============================================================
void calculateFeedback(float L, float R) {
  if (L < 0 || R < 0) {
    noTone(BUZZER);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    buzzerEnabled = false;
    return;
  }

  float proximity = min(L, R);

  // Beyond calibrated zone = silent
  if (proximity > CAL_FAR) {
    noTone(BUZZER);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    buzzerEnabled = false;
    return;
  }

  float diff = L - R;
  float normalizedDiff = mapFloat(diff, SYMMETRICAL_LEFT, SYMMETRICAL_RIGHT, -1.0, 1.0);
  normalizedDiff = constrain(normalizedDiff, -1.0, 1.0);
  float angleDeg = normalizedDiff * -60.0; // + = LEFT, - = RIGHT

  // --- Proximity -> interval between beeps ---
  if (proximity <= CAL_NEAR) {
    currentBeepInterval = 100;
  } else {
    float range = CAL_FAR - CAL_NEAR;
    float pct   = (proximity - CAL_NEAR) / range;
    currentBeepInterval = map(pct * 100, 0, 100, 100, 1000);
  }

  // --- Direction -> beep duration (duty cycle) ---
  // Center (0 deg)   = long beep, sounds "loud / present"
  // Edge  (60 deg)   = short tick, sounds "quiet / faint"
  // We map the off-center magnitude into a percentage of the interval,
  // but clamp so even centered doesn't fill 100% (leaves a tiny gap).
  int dutyPercent = map(abs(angleDeg), 0, 60, 90, 10); // centered = 90% of interval
  dutyPercent = constrain(dutyPercent, 10, 90);

  currentBeepDuration = (long)currentBeepInterval * dutyPercent / 100;
  if (currentBeepDuration < MIN_BEEP_MS) currentBeepDuration = MIN_BEEP_MS;
  buzzerEnabled = true;

  // --- LEDs (immediate, non-blocking) ---
  if (angleDeg > DEAD_ZONE_ANGLE) {
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);     // object left
  } else if (angleDeg < -DEAD_ZONE_ANGLE) {
    digitalWrite(RED_LED, HIGH);      // object right
    digitalWrite(BLUE_LED, LOW);
  } else {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, HIGH);     // centered
  }
}

// --- Non-blocking beep scheduler ---
// Arduino tone(pin, freq, duration) is non-blocking (timer-driven)
void updateBuzzer() {
  if (!buzzerEnabled) return;

  unsigned long now = millis();
  if (now - lastBeepTime >= (unsigned long)currentBeepInterval) {
    lastBeepTime = now;
    tone(BUZZER, BASE_FREQ, currentBeepDuration); // non-blocking
  }
}

// Helper
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ============================================================
void loop() {
  float leftDist  = getAverageDistance(TRIG_L, ECHO_L);
  float rightDist = getAverageDistance(TRIG_R, ECHO_R);

  calculateFeedback(leftDist, rightDist);
  updateBuzzer();
}

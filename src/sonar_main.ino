// ============================================================
//  Sonar System — Main Sketch
//  Non-blocking buzzer + explicit diff-based LED mapping
//
//  LED mapping (direct from diff, no confusing angle sign flip):
//    diff = L − R
//    diff < −15  → L closer  → BLUE on  (person LEFT of center)
//    diff > +15  → R closer  → RED on   (person RIGHT of center)
//    |diff| ≤ 15  → centered  → BOTH on
//
//  Buzzer: pitch = direction, rate = proximity
//  Far wall (> CAL_FAR) = silent
// ============================================================

// === CALIBRATION CONSTANTS (UPDATE AFTER RE-CALIBRATING) ===
// Dev-room estimates; replace with actual calibration results:
const float CAL_NEAR       = 42.94;
const float CAL_FAR        = 130.0;   // set below back-wall distance
const float CAL_DIFF_LEFT  = -60.0;   // typical dev-room left-step diff
const float CAL_DIFF_RIGHT = 60.0;    // typical dev-room right-step diff
// ===========================================================

const float MAX_DIFF_MAGNITUDE = max(abs(CAL_DIFF_LEFT), abs(CAL_DIFF_RIGHT));
const float SYMMETRICAL_LEFT  = -MAX_DIFF_MAGNITUDE;
const float SYMMETRICAL_RIGHT =  MAX_DIFF_MAGNITUDE;

// --- Pins ---
const int TRIG_L = 9;
const int ECHO_L = 10;
const int TRIG_R = 7;
const int ECHO_R = 8;
const int BUZZER = 6;
const int RED_LED = 4;    // Right indicator
const int BLUE_LED = 5;   // Left indicator

// --- Parameters ---
const int SAMPLES = 2;
const float DEAD_ZONE_CM = 15.0;     // cm diff for "center"

// --- Non-blocking buzzer state ---
unsigned long lastBeepTime = 0;
int currentBeepInterval = 0;
int currentFrequency = 0;
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
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 35000);
  if (duration == 0) return -1.0;
  return duration * 0.01716;
}

float getAverageDistance(int trigPin, int echoPin) {
  float sum = 0;
  int count = 0;
  for (int i = 0; i < SAMPLES; i++) {
    float d = measureDistance(trigPin, echoPin);
    if (d > 0) { sum += d; count++; }
    delay(20); // echo settle
  }
  return (count == 0) ? -1.0 : sum / count;
}

// ============================================================
void calculateSpatialFeedback(float L, float R) {
  if (L < 0 || R < 0) {
    noTone(BUZZER);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    buzzerEnabled = false;
    return;
  }

  float proximity = min(L, R);

  // Beyond active zone → silent
  if (proximity > CAL_FAR) {
    noTone(BUZZER);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    buzzerEnabled = false;
    return;
  }

  // --- Proximity -> beep interval ---
  if (proximity <= CAL_NEAR) {
    currentBeepInterval = 100;
  } else {
    float range = CAL_FAR - CAL_NEAR;
    float pct   = (proximity - CAL_NEAR) / range;
    currentBeepInterval = map(pct * 100, 0, 100, 100, 1000);
  }
  buzzerEnabled = true;

  // --- Direction (pitch) using normalized angle ---
  float diff = L - R;
  float normalizedDiff = mapFloat(diff, SYMMETRICAL_LEFT, SYMMETRICAL_RIGHT, -1.0f, 1.0f);
  normalizedDiff = constrain(normalizedDiff, -1.0f, 1.0f);
  float angleDeg = normalizedDiff * 60.0f;   // +60 = LEFT, -60 = RIGHT
  currentFrequency = map(angleDeg, -60, 60, 300, 2000);

  // --- LEDs: explicit diff-based (no sign confusion) ---
  if (diff < -DEAD_ZONE_CM) {
    // L much closer → object on LEFT → BLUE
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);
  } else if (diff > DEAD_ZONE_CM) {
    // R much closer → object on RIGHT → RED
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
  } else {
    // Centered → BOTH ON
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
    tone(BUZZER, currentFrequency, 50);
  }
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ============================================================
void loop() {
  float leftDist  = getAverageDistance(TRIG_L, ECHO_L);
  float rightDist = getAverageDistance(TRIG_R, ECHO_R);

  calculateSpatialFeedback(leftDist, rightDist);
  updateBuzzer();
}

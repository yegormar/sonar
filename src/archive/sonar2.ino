// ============================================================
//  Sonar System v2 — Dual Buzzer (D3 + D6)
//  D3 = Distance  (beep rate = proximity, fixed pitch burst)
//  D6 = Direction (continuous pitch = deviation from center)
//
//  HC-SR04 ~15° cone: DEAD_ZONE_ANGLE = 15 degrees
//  D6 pitch: centered=high (2000Hz), extreme edge=low (300Hz)
//  D3 pitch: fixed burst at 1000Hz
//
//  Both buzzers driven by software square-wave so two tones
//  can sound simultaneously on Arduino Uno.
// ============================================================

// === CALIBRATION CONSTANTS (latest run) ===
const float CAL_NEAR       = 42.94;
const float CAL_FAR        = 215.93;
const float CAL_DIFF_LEFT  = 55.55;
const float CAL_DIFF_RIGHT = 52.53;
// ===========================================

const float MAX_DIFF_MAGNITUDE = max(abs(CAL_DIFF_LEFT), abs(CAL_DIFF_RIGHT));
const float SYMMETRICAL_LEFT  = -MAX_DIFF_MAGNITUDE;
const float SYMMETRICAL_RIGHT =  MAX_DIFF_MAGNITUDE;

// --- Pins ---
const int TRIG_L = 9;
const int ECHO_L = 10;
const int TRIG_R = 7;
const int ECHO_R = 8;
const int BUZZER_DIST = 3;   // D3: distance buzzer (burst rate)
const int BUZZER_DIR  = 6;   // D6: direction buzzer (continuous pitch)
const int RED_LED = 4;
const int BLUE_LED = 5;

// --- Parameters ---
const int SAMPLES = 2;
const float DEAD_ZONE_ANGLE = 15.0;   // HC-SR04 cone ~15°
const int DIST_BEEP_FREQ    = 1000;   // Hz burst tone
const int DIST_BEEP_MS      = 50;     // burst length
const int DIR_FREQ_MIN      = 300;    // Hz fully off-center
const int DIR_FREQ_MAX      = 2000;   // Hz centered

// --- Software square-wave state ---
unsigned long lastToggleDir  = 0;
unsigned long lastToggleDist = 0;
bool dirPinState  = LOW;
bool distPinState = LOW;

int dirFreq            = 0;
bool distBeepActive    = false;
unsigned long distBeepEndTime = 0;
unsigned long lastBeepTime    = 0;
int currentBeepInterval       = 1000;
bool buzzerEnabled            = false;

// ============================================================
void setup() {
  Serial.begin(9600);

  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(BUZZER_DIST, OUTPUT);
  pinMode(BUZZER_DIR,  OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  digitalWrite(BUZZER_DIST, LOW);
  digitalWrite(BUZZER_DIR,  LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);

  Serial.println(F("Sonar2 - Dual Buzzer"));
}

// ============================================================
//  Software non-blocking square wave for simultaneous buzzers
// ============================================================
void updateBuzzers() {
  unsigned long now = micros();

  // --- D6: direction tone (continuous) ---
  if (dirFreq > 0) {
    unsigned long halfPeriod = 500000UL / (unsigned long)dirFreq;
    if (now - lastToggleDir >= halfPeriod) {
      lastToggleDir = now;
      dirPinState = !dirPinState;
      digitalWrite(BUZZER_DIR, dirPinState);
    }
  } else {
    if (dirPinState) { digitalWrite(BUZZER_DIR, LOW); dirPinState = LOW; }
  }

  // --- D3: distance burst (only when active) ---
  if (distBeepActive && DIST_BEEP_FREQ > 0) {
    unsigned long halfPeriod = 500000UL / (unsigned long)DIST_BEEP_FREQ;
    if (now - lastToggleDist >= halfPeriod) {
      lastToggleDist = now;
      distPinState = !distPinState;
      digitalWrite(BUZZER_DIST, distPinState);
    }
  } else {
    if (distPinState) { digitalWrite(BUZZER_DIST, LOW); distPinState = LOW; }
  }
}

// ============================================================
//  Sensor reading that keeps buzzers alive during pulseIn wait
// ============================================================
float measureDistanceWithBuzzers(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  const unsigned long timeout = 35000UL;

  // Wait for any lingering HIGH to end (new pulse can't start until old one done)
  unsigned long start = micros();
  while (digitalRead(echoPin) == HIGH) {
    if (micros() - start > timeout) return -1.0;
    updateBuzzers();
  }

  // Wait for rising edge
  start = micros();
  while (digitalRead(echoPin) == LOW) {
    if (micros() - start > timeout) return -1.0;
    updateBuzzers();
  }

  // Measure HIGH duration
  unsigned long echoStart = micros();
  while (digitalRead(echoPin) == HIGH) {
    if (micros() - echoStart > timeout) return -1.0;
    updateBuzzers();
  }

  unsigned long duration = micros() - echoStart;
  return duration * 0.01716;
}

float getAverageDistance(int trigPin, int echoPin) {
  float sum = 0;
  int count = 0;
  for (int i = 0; i < SAMPLES; i++) {
    float d = measureDistanceWithBuzzers(trigPin, echoPin);
    if (d > 0) { sum += d; count++; }
    // Non-blocking settle delay (~20ms), keep buzzers running
    unsigned long t0 = millis();
    while (millis() - t0 < 20) { updateBuzzers(); }
  }
  return (count == 0) ? -1.0 : sum / count;
}

// ============================================================
//  Feedback logic
// ============================================================
void calculateFeedback(float L, float R) {
  if (L < 0 || R < 0) {
    dirFreq = 0;
    distBeepActive = false;
    buzzerEnabled = false;
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    return;
  }

  float proximity = min(L, R);

  // Out of range = silent
  if (proximity > CAL_FAR) {
    dirFreq = 0;
    distBeepActive = false;
    buzzerEnabled = false;
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    return;
  }

  // --- Proximity -> D3 beep interval ---
  if (proximity <= CAL_NEAR) {
    currentBeepInterval = 100;
  } else {
    float range = CAL_FAR - CAL_NEAR;
    float pct   = (proximity - CAL_NEAR) / range;
    currentBeepInterval = (int)mapFloat(pct, 0.0f, 1.0f, 100.0f, 1000.0f);
  }
  buzzerEnabled = true;

  // --- Direction (deviation from center) -> D6 continuous pitch ---
  float diff = L - R;
  float normalizedDiff = mapFloat(diff, SYMMETRICAL_LEFT, SYMMETRICAL_RIGHT, -1.0f, 1.0f);
  normalizedDiff = constrain(normalizedDiff, -1.0f, 1.0f);
  float angleDeg = normalizedDiff * -60.0f;   // + = LEFT, - = RIGHT
  float deviation = abs(angleDeg);              // 0..60, center = 0

  // Deviation 0°  -> highest pitch (centered)
  // Deviation 60° -> lowest pitch  (extreme edge)
  dirFreq = (int)(DIR_FREQ_MAX - (deviation / 60.0f) * (float)(DIR_FREQ_MAX - DIR_FREQ_MIN));
  dirFreq = constrain(dirFreq, DIR_FREQ_MIN, DIR_FREQ_MAX);

  // --- LEDs ---
  if (angleDeg > DEAD_ZONE_ANGLE) {
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, HIGH); // left
  } else if (angleDeg < -DEAD_ZONE_ANGLE) {
    digitalWrite(RED_LED, HIGH);  // right
    digitalWrite(BLUE_LED, LOW);
  } else {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, HIGH); // center
  }
}

// Scheduler: triggers a D3 burst every currentBeepInterval
void updateDistanceBeep() {
  if (!buzzerEnabled) {
    distBeepActive = false;
    return;
  }

  unsigned long now = millis();

  if (distBeepActive && now >= distBeepEndTime) {
    distBeepActive = false;
  }

  if (now - lastBeepTime >= (unsigned long)currentBeepInterval) {
    lastBeepTime = now;
    distBeepActive = true;
    distBeepEndTime = now + DIST_BEEP_MS;
  }
}

// Helper
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ============================================================
//  Main Loop
// ============================================================
void loop() {
  float leftDist  = getAverageDistance(TRIG_L, ECHO_L);
  float rightDist = getAverageDistance(TRIG_R, ECHO_R);

  calculateFeedback(leftDist, rightDist);
  updateDistanceBeep();
  updateBuzzers();
}

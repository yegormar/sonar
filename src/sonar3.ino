// ============================================================
//  Sonar System — Version 3 (Clean Implementation)
//  Dual buzzer system: active buzzer for proximity, passive for direction
//  Fast measurement cycle with 3-state direction logic and debouncing
// ============================================================

// === CALIBRATION CONSTANTS ===
const float CAL_NEAR       = 40.0;
const float CAL_FAR        = 200.0;   // set below back-wall distance
const float CENTER_THRESHOLD_PCT = 0.20; // 10% threshold for center detection

// === DEBUG CONFIGURATION ===
#define DEBUG_ENABLED 1  // Set to 0 to disable debug output
#define DEBUG_SERIAL_BAUD 115200

// --- Pins ---
const int TRIG_L = 9;
const int ECHO_L = 10;
const int TRIG_R = 7;
const int ECHO_R = 8;
const int BUZZER_ACTIVE = 3;   // Active buzzer for proximity
const int BUZZER_PASSIVE = 6; // Passive buzzer for direction
const int RED_LED = 4;         // Right indicator
const int BLUE_LED = 5;        // Left indicator

// --- Parameters ---
const int SAMPLES = 1;
const int DEBOUNCE_COUNT = 2;    // Require 3 consecutive consistent readings

// --- Active buzzer state (proximity beeping) ---
unsigned long lastActiveBeepTime = 0;
int currentActiveBeepInterval = 0;
bool activeBuzzerEnabled = false;

// --- Passive buzzer state (direction indication) ---
int currentPassiveFrequency = 0;
bool passiveBuzzerEnabled = false;

// --- Debouncing state tracking ---
enum DirectionState { STATE_UNKNOWN, STATE_CENTER, STATE_LEFT, STATE_RIGHT };
DirectionState currentState = STATE_UNKNOWN;
DirectionState pendingState = STATE_UNKNOWN;
int debounceCounter = 0;

// ============================================================
void setup() {
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(BUZZER_ACTIVE, OUTPUT);
  pinMode(BUZZER_PASSIVE, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  digitalWrite(BUZZER_ACTIVE, LOW);
  noTone(BUZZER_PASSIVE);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);

#if DEBUG_ENABLED
  Serial.begin(DEBUG_SERIAL_BAUD);
  // Short delay to allow serial to initialize, but don't block forever
  delay(100);
  Serial.println(F("Sonar3 Debug - Timestamp, DL, DR, Direction"));
#endif
}

// ============================================================
// Fast sensor reading (optimized from direction_v2_recommended.ino)
float readSensor(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 35000); // 35 ms timeout ~ 600 cm
  if (duration == 0) return -1.0;
  return duration * 0.01716; // cm
}

float getAverageDistance(int trigPin, int echoPin) {
  float sum = 0;
  int count = 0;
  for (int i = 0; i < SAMPLES; i++) {
    float d = readSensor(trigPin, echoPin);
    if (d > 0) { sum += d; count++; }
    delay(20); // echo settle
  }
  return (count == 0) ? -1.0 : sum / count;
}

// ============================================================
// Direction logic with 3 states
bool isObjectCentered(float L, float R) {
  if (L < 0 || R < 0) return false;
  float diff = abs(L - R);
  float avgDist = (L + R) / 2.0;
  float threshold = avgDist * CENTER_THRESHOLD_PCT;
  return diff <= threshold;
}

bool isObjectLeft(float L, float R) {
  if (L < 0 || R < 0) return false;
  float diff = L - R;
  float avgDist = (L + R) / 2.0;
  float threshold = avgDist * CENTER_THRESHOLD_PCT;
  return diff < -threshold; // L significantly smaller than R
}

bool isObjectRight(float L, float R) {
  if (L < 0 || R < 0) return false;
  float diff = L - R;
  float avgDist = (L + R) / 2.0;
  float threshold = avgDist * CENTER_THRESHOLD_PCT;
  return diff > threshold; // R significantly smaller than L
}

// ============================================================
void calculateSpatialFeedback(float L, float R) {
  // Reset everything if no valid readings
  if (L < 0 || R < 0) {
    digitalWrite(BUZZER_ACTIVE, LOW);
    noTone(BUZZER_PASSIVE);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    activeBuzzerEnabled = false;
    passiveBuzzerEnabled = false;
    currentState = STATE_UNKNOWN;
    pendingState = STATE_UNKNOWN;
    debounceCounter = 0;
    return;
  }

  float proximity = min(L, R);

  // Beyond active zone → silent
  if (proximity > CAL_FAR) {
    digitalWrite(BUZZER_ACTIVE, LOW);
    noTone(BUZZER_PASSIVE);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    activeBuzzerEnabled = false;
    passiveBuzzerEnabled = false;
    currentState = STATE_UNKNOWN;
    pendingState = STATE_UNKNOWN;
    debounceCounter = 0;
    return;
  }

  // --- Active Buzzer: Improved Proximity beeping (LINEAR RESPONSE) ---
  // Calculate beep interval based on proximity with better linear feel
  float normalizedProximity = constrain(proximity, CAL_NEAR, CAL_FAR);
  
  // Linear mapping: closer = faster beeping (100ms to 1000ms interval)
  // Using logarithmic scaling for more natural perception
  float proximityRatio = (CAL_FAR - normalizedProximity) / (CAL_FAR - CAL_NEAR);
  
  // Exponential mapping for more linear human perception
  // This makes the transition from fast to slow beeping feel more natural
  float perceivedIntensity = pow(proximityRatio, 1.5);
  
  // Map to beep interval (100ms = very close, 1000ms = far but still active)
  currentActiveBeepInterval = 100 + (900 * (1.0 - perceivedIntensity));
  
  // Ensure we stay within bounds
  currentActiveBeepInterval = constrain(currentActiveBeepInterval, 100, 1000);
  
  activeBuzzerEnabled = true;

  // --- Debounced Direction Detection ---
  DirectionState detectedState = STATE_UNKNOWN;
  
  if (isObjectCentered(L, R)) {
    detectedState = STATE_CENTER;
  } else if (isObjectLeft(L, R)) {
    detectedState = STATE_LEFT;
  } else if (isObjectRight(L, R)) {
    detectedState = STATE_RIGHT;
  }
  
  // Debouncing logic - require DEBOUNCE_COUNT consecutive identical readings
  if (detectedState == pendingState) {
    debounceCounter++;
    if (debounceCounter >= DEBOUNCE_COUNT) {
      currentState = detectedState;
      debounceCounter = 0;
    }
  } else {
    pendingState = detectedState;
    debounceCounter = 1;
  }
  
  // Apply the stable debounced state
  if (currentState == STATE_CENTER) {
    currentPassiveFrequency = 100; // Low pitch tone for center (100Hz)
    passiveBuzzerEnabled = true;
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, HIGH);
  } else if (currentState == STATE_LEFT) {
    // No tone for left/right, only LEDs
    noTone(BUZZER_PASSIVE);
    passiveBuzzerEnabled = false;
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);
  } else if (currentState == STATE_RIGHT) {
    // No tone for left/right, only LEDs
    noTone(BUZZER_PASSIVE);
    passiveBuzzerEnabled = false;
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
  } else {
    // Unknown/unstable state
    noTone(BUZZER_PASSIVE);
    passiveBuzzerEnabled = false;
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
  }
}

// ============================================================
// Non-blocking active buzzer update with precise timing
void updateActiveBuzzer() {
  if (!activeBuzzerEnabled) {
    digitalWrite(BUZZER_ACTIVE, LOW);
    return;
  }

  unsigned long now = millis();
  unsigned long timeSinceLastBeep = now - lastActiveBeepTime;
  
  // Beep cycle: ON for 30ms, then OFF until next interval
  if (timeSinceLastBeep < 30) {
    // Keep buzzer ON for first 30ms of each beep cycle
    digitalWrite(BUZZER_ACTIVE, HIGH);
  } else if (timeSinceLastBeep >= currentActiveBeepInterval) {
    // Time for next beep - reset timer
    lastActiveBeepTime = now;
    digitalWrite(BUZZER_ACTIVE, HIGH);
  } else {
    // Keep buzzer OFF between beeps
    digitalWrite(BUZZER_ACTIVE, LOW);
  }
}

// ============================================================
// Passive buzzer update (continuous tone for direction)
void updatePassiveBuzzer() {
  if (passiveBuzzerEnabled) {
    tone(BUZZER_PASSIVE, currentPassiveFrequency);
  } else {
    noTone(BUZZER_PASSIVE);
  }
}

// ============================================================
void loop() {
  unsigned long t0 = millis();
  
  float leftDist = getAverageDistance(TRIG_L, ECHO_L);
  float rightDist = getAverageDistance(TRIG_R, ECHO_R);

  calculateSpatialFeedback(leftDist, rightDist);
  
#if DEBUG_ENABLED
  // Debug output - show both raw and debounced data
  float proximity = min(leftDist, rightDist);
  if (leftDist > 0 && rightDist > 0 && proximity <= CAL_FAR) {
    // Raw measurement (every cycle)
    Serial.print(millis());
    Serial.print(F(","));
    Serial.print(leftDist, 1);
    Serial.print(F(","));
    Serial.print(rightDist, 1);
    Serial.print(F(","));
    
    // Show detected vs debounced state
    if (isObjectCentered(leftDist, rightDist)) {
      Serial.print(F("RAW_CENTER"));
    } else if (isObjectLeft(leftDist, rightDist)) {
      Serial.print(F("RAW_LEFT"));
    } else if (isObjectRight(leftDist, rightDist)) {
      Serial.print(F("RAW_RIGHT"));
    } else {
      Serial.print(F("RAW_UNKNOWN"));
    }
    
    Serial.print(F(","));
    
    // Debounced state (stable)
    if (currentState == STATE_CENTER) {
      Serial.println(F("DEBOUNCED_CENTER"));
    } else if (currentState == STATE_LEFT) {
      Serial.println(F("DEBOUNCED_LEFT"));
    } else if (currentState == STATE_RIGHT) {
      Serial.println(F("DEBOUNCED_RIGHT"));
    } else {
      Serial.println(F("DEBOUNCED_UNKNOWN"));
    }
  }
#endif
  
  updateActiveBuzzer();
  updatePassiveBuzzer();

  // Maintain fast measurement cycle (~50 ms loop period)
  long elapsed = (long)millis() - (long)t0;
  long delayMs = 50L - elapsed;
  if (delayMs > 0) {
    delay((unsigned long)delayMs);
  }
}
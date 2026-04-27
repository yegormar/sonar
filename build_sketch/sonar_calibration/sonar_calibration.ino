// ============================================================
//  Sonar System — Calibration Sketch
//  Upload this separately from the main sketch.
//  Same pin wiring — no hardware changes needed.
//  Open Serial Monitor at 9600 baud and follow the prompts.
// ============================================================

// --- Pin definitions (must match main sketch) ---
const int TRIG_L = 9;
const int ECHO_L = 10;
const int TRIG_R = 7;
const int ECHO_R = 8;
const int RED_LED = 4;    // Right indicator
const int BLUE_LED = 5;   // Left indicator

// --- Sampling ---
const int SAMPLES     = 20;   // readings averaged per capture
const int NOISE_CHECK = 10;   // extra reads used to check stability

// --- Calibration results ---
float cal_near  = 0;   // average of steps 1 & 2: closest center (both sensors)
float cal_far   = 0;   // step 3: farthest center (both sensors)
float cal_left_L  = 0, cal_left_R  = 0;  // step 4: max left (raw per sensor)
float cal_right_L = 0, cal_right_R = 0;  // step 5: max right (raw per sensor)

// --- Function prototypes ---
float measureOnce(int trigPin, int echoPin);
float avgDistance(int trigPin, int echoPin);

// ============================================================
void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  // Initialize LEDs off
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  
  Serial.println();
  Serial.println(F("============================================="));
  Serial.println(F("  SONAR CALIBRATION"));
  Serial.println(F("============================================="));
  Serial.println(F("You will place an object in 5 positions."));
  Serial.println(F("At each step: position the object, then"));
  Serial.println(F("send any character + Enter to capture."));
  Serial.println();
  Serial.println(F("Press Enter to begin."));
  waitForUser();
}

// ============================================================
void loop() {
  captureNear1();
  captureNear2();
  captureFar();
  captureLeft();
  captureRight();
  printResults();
  Serial.println(F("Calibration done. Upload your main sketch to continue."));
  while (true) {}
}

// ============================================================
//  Step 1 — Nearest position (first capture)
// ============================================================
void captureNear1() {
  Serial.println(F("----- STEP 1 of 5: Closest position -----"));
  Serial.println(F("Place the object as close as possible,"));
  Serial.println(F("directly in front, centered between sensors."));
  Serial.println(F("This will be your 'alarm' distance — constant beep."));
  Serial.println(F("Send any character when ready."));
  waitForUser();
  
  Serial.println(F("  Measuring... (LEDs will blink)"));
  float L = avgDistance(TRIG_L, ECHO_L);
  float R = avgDistance(TRIG_R, ECHO_R);
  
  if (L < 0 || R < 0) { 
    Serial.println(F("ERROR: No reading. Check wiring and try again."));
    // Flash both LEDs rapidly for error
    for(int i=0; i<5; i++) {
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BLUE_LED, HIGH);
      delay(100);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
      delay(100);
    }
    return; 
  }
  
  float avg = (L + R) / 2.0;
  checkStability(avg);
  
  cal_near = avg;  // will be averaged with step 2
  showStepComplete(1);
  
  Serial.print(F("  Captured: ")); Serial.print(avg, 1); Serial.println(F(" cm (raw)"));
  Serial.println();
}

// ============================================================
//  Step 2 — Nearest position (confirmation)
// ============================================================
void captureNear2() {
  Serial.println(F("----- STEP 2 of 5: Closest position again -----"));
  Serial.println(F("Keep the object in the same position."));
  Serial.println(F("This confirms the near reading and smooths noise."));
  Serial.println(F("Send any character when ready."));
  waitForUser();
  
  Serial.println(F("  Measuring... (LEDs will blink)"));
  float L = avgDistance(TRIG_L, ECHO_L);
  float R = avgDistance(TRIG_R, ECHO_R);
  
  if (L < 0 || R < 0) { 
    Serial.println(F("ERROR: No reading. Check wiring and try again."));
    // Flash both LEDs rapidly for error
    for(int i=0; i<5; i++) {
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BLUE_LED, HIGH);
      delay(100);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
      delay(100);
    }
    return; 
  }
  
  float avg = (L + R) / 2.0;
  checkStability(avg);
  
  cal_near = (cal_near + avg) / 2.0;  // average both near captures
  showStepComplete(2);
  
  Serial.print(F("  Captured: ")); Serial.print(avg, 1); Serial.println(F(" cm (raw)"));
  Serial.print(F("  Near threshold set to: ")); Serial.print(cal_near, 1); Serial.println(F(" cm"));
  Serial.println();
}

// ============================================================
//  Step 3 — Farthest center position
// ============================================================
void captureFar() {
  Serial.println(F("----- STEP 3 of 5: Farthest position -----"));
  Serial.println(F("Move the object back slowly, still centered,"));
  Serial.println(F("until you reach the distance where you'd want"));
  Serial.println(F("beeping to stop. This sets your detection range."));
  Serial.println(F("Send any character when ready."));
  waitForUser();
  
  Serial.println(F("  Measuring... (LEDs will blink)"));
  float L = avgDistance(TRIG_L, ECHO_L);
  float R = avgDistance(TRIG_R, ECHO_R);
  
  if (L < 0 || R < 0) { 
    Serial.println(F("ERROR: No reading — object may be too far. Move closer slightly."));
    // Flash both LEDs rapidly for error
    for(int i=0; i<5; i++) {
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BLUE_LED, HIGH);
      delay(100);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
      delay(100);
    }
    return; 
  }
  
  cal_far = (L + R) / 2.0;
  showStepComplete(3);
  
  Serial.print(F("  Captured: ")); Serial.print(cal_far, 1); Serial.println(F(" cm (raw)"));
  
  if (cal_far <= cal_near) {
    Serial.println(F("  WARNING: Far position reads closer than near. Did you move the object back?"));
  }
  Serial.println();
}

// ============================================================
//  Step 4 — Maximum left position
// ============================================================
void captureLeft() {
  Serial.println(F("----- STEP 4 of 5: Max left position -----"));
  Serial.println(F("Move the object sideways to the left,"));
  Serial.println(F("at roughly the same distance as Step 3."));
  Serial.println(F("Stop just before detection becomes unreliable."));
  Serial.println(F("Send any character when ready."));
  waitForUser();
  
  Serial.println(F("  Measuring... (LEDs will blink)"));
  cal_left_L = avgDistance(TRIG_L, ECHO_L);
  cal_left_R = avgDistance(TRIG_R, ECHO_R);
  
  if (cal_left_L < 0 || cal_left_R < 0) {
    Serial.println(F("ERROR: No reading — object may be out of range. Move it slightly closer to center."));
    // Flash both LEDs rapidly for error
    for(int i=0; i<5; i++) {
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BLUE_LED, HIGH);
      delay(100);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
      delay(100);
    }
    return;
  }
  showStepComplete(4);
  
  Serial.print(F("  Left sensor: "));  Serial.print(cal_left_L, 1); Serial.println(F(" cm"));
  Serial.print(F("  Right sensor: ")); Serial.print(cal_left_R, 1); Serial.println(F(" cm"));
  
  if (cal_left_L >= cal_left_R) {
    Serial.println(F("  NOTE: Left sensor reads farther than right — as expected for a left-side object. Good."));
  } else {
    Serial.println(F("  WARNING: Right sensor reads farther — object may be to the right of center. Check position."));
  }
  Serial.println();
}

// ============================================================
//  Step 5 — Maximum right position
// ============================================================
void captureRight() {
  Serial.println(F("----- STEP 5 of 5: Max right position -----"));
  Serial.println(F("Move the object to the far right side,"));
  Serial.println(F("mirroring Step 4 as closely as possible."));
  Serial.println(F("Send any character when ready."));
  waitForUser();
  
  Serial.println(F("  Measuring... (LEDs will blink)"));
  cal_right_L = avgDistance(TRIG_L, ECHO_L);
  cal_right_R = avgDistance(TRIG_R, ECHO_R);
  
  if (cal_right_L < 0 || cal_right_R < 0) {
    Serial.println(F("ERROR: No reading — object may be out of range. Move it slightly closer to center."));
    // Flash both LEDs rapidly for error
    for(int i=0; i<5; i++) {
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BLUE_LED, HIGH);
      delay(100);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
      delay(100);
    }
    return;
  }
  showStepComplete(5);
  
  Serial.print(F("  Left sensor: "));  Serial.print(cal_right_L, 1); Serial.println(F(" cm"));
  Serial.print(F("  Right sensor: ")); Serial.print(cal_right_R, 1); Serial.println(F(" cm"));
  
  if (cal_right_R >= cal_right_L) {
    Serial.println(F("  NOTE: Right sensor reads farther than left — as expected for a right-side object. Good."));
  } else {
    Serial.println(F("  WARNING: Left sensor reads farther — object may be to the left of center. Check position."));
  }
  Serial.println();
}

// ============================================================
//  Print final constants block
// ============================================================
void printResults() {
  // Derive the differential signatures for left and right
  // These are the L-R differences at each extreme — used by main sketch
  // to interpolate direction linearly between them.
  float diff_left  = cal_left_L  - cal_left_R;   // negative: L closer
  float diff_right = cal_right_L - cal_right_R;  // positive: R closer
  
  Serial.println(F("============================================="));
  Serial.println(F("  ALL STEPS COMPLETE"));
  Serial.println(F("  Copy the block below into your main sketch"));
  Serial.println(F("============================================="));
  Serial.println();
  Serial.println(F("// === CALIBRATION CONSTANTS — paste into main sketch ==="));
  Serial.print(F("const float CAL_NEAR       = ")); Serial.print(cal_near, 1);
  Serial.println(F(";  // cm — constant beep threshold"));
  Serial.print(F("const float CAL_FAR        = ")); Serial.print(cal_far, 1);
  Serial.println(F(";  // cm — beeping starts here"));
  Serial.print(F("const float CAL_DIFF_LEFT  = ")); Serial.print(diff_left, 2);
  Serial.println(F(";  // L-R cm difference at max left"));
  Serial.print(F("const float CAL_DIFF_RIGHT = ")); Serial.print(diff_right, 2);
  Serial.println(F(";  // L-R cm difference at max right"));
  Serial.println(F("// ======================================================="));
  Serial.println();
}

// ============================================================
//  Stability check — warns if readings are noisy
// ============================================================
void checkStability(float reference) {
  float maxDev = 0;
  for (int i = 0; i < NOISE_CHECK; i++) {
    float L = measureOnce(TRIG_L, ECHO_L);
    float R = measureOnce(TRIG_R, ECHO_R);
    if (L < 0 || R < 0) continue;
    float dev = abs(((L + R) / 2.0) - reference);
    if (dev > maxDev) maxDev = dev;
    delay(30);
  }
  if (maxDev > 2.0) {
    Serial.print(F("  NOTE: Readings vary by up to ")); Serial.print(maxDev, 1);
    Serial.println(F(" cm. Hold the object still, or move back slightly."));
  }
}

// ============================================================
//  LED Feedback Functions
// ============================================================

void showReadingInProgress() {
  // Blink both LEDs to show sensors are being read
  digitalWrite(RED_LED, HIGH);
  digitalWrite(BLUE_LED, HIGH);
  delay(100);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  delay(100);
}

void showStepComplete(int step) {
  // Quick blink pattern to indicate step completion
  for (int i = 0; i < step; i++) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, HIGH);
    delay(150);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    delay(150);
  }
  delay(500);
}

// ============================================================
//  Utilities
// ============================================================

void waitForUser() {
  while (Serial.available()) Serial.read();
  while (!Serial.available()) {
    // Pulse LEDs slowly while waiting for user input
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
    delay(200);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);
    delay(200);
  }
  while (Serial.available()) Serial.read();
  
  // Turn off LEDs after input received
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
}

float measureOnce(int trigPin, int echoPin) {
  // Indicate which sensor is being measured
  if (trigPin == TRIG_L) {
    digitalWrite(BLUE_LED, HIGH); // Left sensor measurement - BLUE LED
  } else {
    digitalWrite(RED_LED, HIGH); // Right sensor measurement - RED LED
  }
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long dur = pulseIn(echoPin, HIGH, 30000);
  
  // Turn off LED after measurement
  if (trigPin == TRIG_L) {
    digitalWrite(BLUE_LED, LOW);
  } else {
    digitalWrite(RED_LED, LOW);
  }
  
  if (dur == 0) return -1.0;
  float distance = dur * 0.01716;
  
  // For your setup: allow up to 160cm to handle 30cm side movement at 90cm distance
  // This gives us ~120-140cm diagonal measurements which should work reliably
  if (distance > 160.0) return -1.0;
  
  return distance;
}

float avgDistance(int trigPin, int echoPin) {
  float sum = 0; int count = 0;
  for (int i = 0; i < SAMPLES; i++) {
    float d = measureOnce(trigPin, echoPin);
    if (d > 0) { sum += d; count++; }
    delay(30);
  }
  return (count == 0) ? -1.0 : sum / count;
}
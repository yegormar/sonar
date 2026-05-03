// Simple reliable calibration - RED=RIGHT, BLUE=LEFT
// FIXED: Only retries the CURRENT failed step, not all previous ones.

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

enum State { ST_NEAR, ST_FAR, ST_LEFT, ST_RIGHT, ST_DONE };
State state = ST_NEAR;

float cal_near = 0, cal_far = 0;
float cal_left_L = 0, cal_left_R = 0;
float cal_right_L = 0, cal_right_R = 0;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT); pinMode(BLUE_LED, OUTPUT);
  Serial.println(F("=== CALIBRATION ==="));
  Serial.println(F("RED=RIGHT, BLUE=LEFT"));
}

void loop() {
  switch (state) {
    case ST_NEAR:  doStepNear();  break;
    case ST_FAR:   doStepFar();   break;
    case ST_LEFT:  doStepLeft();  break;
    case ST_RIGHT: doStepRight(); break;
    case ST_DONE:  /* idle */     break;
  }
}

// --- Helpers ---
float readSensor(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 40000);
  if (dur == 0) return -1;
  return dur * 0.01716;
}

bool readPair(float &L, float &R) {
  int tries = 0;
  while (tries < 3 && (L < 0 || R < 0)) {
    if (L < 0) L = readSensor(TRIG_L, ECHO_L);
    if (R < 0) R = readSensor(TRIG_R, ECHO_R);
    if (L < 0 || R < 0) delay(200);
    tries++;
  }
  return (L >= 0 && R >= 0);
}

void waitForEnter() {
  while (!Serial.available());
  while (Serial.available()) Serial.read();
}

// --- Steps ---
void doStepNear() {
  Serial.println(F("=== STEP 1/4 ==="));
  Serial.println(F("Object close and centered (30-60cm). Press Enter."));
  waitForEnter();

  float L = -1, R = -1;
  if (!readPair(L, R)) {
    Serial.println(F("Sensor read failed. Press Enter to retry."));
    return; // stays in ST_NEAR
  }

  if (abs(L - R) > 15) {
    Serial.print(F("NOT CENTERED: L=")); Serial.print(L);
    Serial.print(F(" R=")); Serial.println(R);
    Serial.println(F("Center the object and press Enter to retry."));
    return; // stays in ST_NEAR
  }

  cal_near = (L + R) / 2.0;
  Serial.print(F("NEAR = ")); Serial.print(cal_near); Serial.println(F("cm\n"));
  state = ST_FAR;
}

void doStepFar() {
  Serial.println(F("=== STEP 2/4 ==="));
  Serial.println(F("Object far and centered (1-2m). Press Enter."));
  waitForEnter();

  float L = -1, R = -1;
  if (!readPair(L, R)) {
    Serial.println(F("Sensor read failed. Press Enter to retry."));
    return; // stays in ST_FAR
  }

  if (abs(L - R) > 20) {
    Serial.print(F("NOT CENTERED: L=")); Serial.print(L);
    Serial.print(F(" R=")); Serial.println(R);
    Serial.println(F("Center the object and press Enter to retry."));
    return; // stays in ST_FAR
  }

  cal_far = (L + R) / 2.0;
  if (cal_far <= cal_near) {
    Serial.println(F("FAR distance must be farther than NEAR. Move back and press Enter."));
    return; // stays in ST_FAR
  }
  if (cal_far > 300) {
    Serial.println(F("FAR >300cm. Room too big or hitting back wall. Move closer."));
    return;
  }

  Serial.print(F("FAR = ")); Serial.print(cal_far); Serial.println(F("cm\n"));
  state = ST_LEFT;
}

void doStepLeft() {
  Serial.println(F("=== STEP 3/4 ==="));
  Serial.println(F("SAME distance, step LEFT as far as you can. Press Enter."));
  Serial.println(F("  Goal: right sensor loses you / hits wall."));
  waitForEnter();

  float L = -1, R = -1;
  if (!readPair(L, R)) {
    Serial.println(F("Sensor read failed. Press Enter to retry."));
    return; // stays in ST_LEFT
  }

  float diff = L - R;

  // Wall / background check (warn but don't reject unless both sensors dead)
  if (abs(L - cal_far) > 100 && abs(R - cal_far) > 100) {
    Serial.print(F("Both sensors lost you: L=")); Serial.print(L);
    Serial.print(F(" R=")); Serial.println(R);
    Serial.println(F("Make sure you are still at the same distance."));
    return; // stays in ST_LEFT
  }
  if (abs(L - cal_far) > 100) {
    Serial.println(F("Left sensor lost you (hitting wall?). Accepting if other sensor is good."));
  }
  if (abs(R - cal_far) > 100) {
    Serial.println(F("Right sensor lost you (hitting wall?). Accepting if other sensor is good."));
  }

  if (abs(diff) < 20) {
    Serial.print(F("Diff too small (")); Serial.print(diff, 1);
    Serial.println(F("cm). Step much farther LEFT and press Enter."));
    return; // stays in ST_LEFT
  }

  if (diff > 0) {
    Serial.println(F("Diff is POSITIVE — you may have stepped RIGHT instead of LEFT."));
    Serial.println(F("Entering anyway. If direction is wrong in main sketch, swap the constant sign."));
  }

  cal_left_L = L;
  cal_left_R = R;
  Serial.print(F("LEFT: L=")); Serial.print(L);
  Serial.print(F(" R=")); Serial.println(R);
  Serial.println(F("---\n"));
  state = ST_RIGHT;
}

void doStepRight() {
  Serial.println(F("=== STEP 4/4 ==="));
  Serial.println(F("SAME distance, step RIGHT as far as you can. Press Enter."));
  Serial.println(F("  Goal: left sensor loses you / hits wall."));
  waitForEnter();

  float L = -1, R = -1;
  if (!readPair(L, R)) {
    Serial.println(F("Sensor read failed. Press Enter to retry."));
    return; // stays in ST_RIGHT
  }

  float diff = L - R;

  if (abs(L - cal_far) > 100 && abs(R - cal_far) > 100) {
    Serial.print(F("Both sensors lost you: L=")); Serial.print(L);
    Serial.print(F(" R=")); Serial.println(R);
    Serial.println(F("Make sure you are still at the same distance."));
    return;
  }
  if (abs(L - cal_far) > 100) {
    Serial.println(F("Left sensor lost you (accepting if other is good)."));
  }
  if (abs(R - cal_far) > 100) {
    Serial.println(F("Right sensor lost you (accepting if other is good)."));
  }

  if (abs(diff) < 20) {
    Serial.print(F("Diff too small (")); Serial.print(diff, 1);
    Serial.println(F("cm). Step much farther RIGHT and press Enter."));
    return; // stays in ST_RIGHT
  }

  if (diff < 0) {
    Serial.println(F("Diff is NEGATIVE — you may have stepped LEFT instead of RIGHT."));
    Serial.println(F("Entering anyway. If direction is wrong in main sketch, swap the constant sign."));
  }

  cal_right_L = L;
  cal_right_R = R;
  Serial.print(F("RIGHT: L=")); Serial.print(L);
  Serial.print(F(" R=")); Serial.println(R);
  Serial.println(F("---\n"));
  state = ST_DONE;
  printResults();
}

void printResults() {
  float diff_left = cal_left_L - cal_left_R;
  float diff_right = cal_right_L - cal_right_R;

  Serial.println(F("=== SUCCESS ==="));
  Serial.print(F("CAL_NEAR = ")); Serial.println(cal_near);
  Serial.print(F("CAL_FAR = ")); Serial.println(cal_far);
  Serial.print(F("CAL_DIFF_LEFT = ")); Serial.println(diff_left, 2);
  Serial.print(F("CAL_DIFF_RIGHT = ")); Serial.println(diff_right, 2);
  Serial.println(F("Copy these into sonar_main.ino:"));
  Serial.println();
  Serial.print(F("const float CAL_NEAR       = ")); Serial.print(cal_near, 2); Serial.println(";  // cm");
  Serial.print(F("const float CAL_FAR        = ")); Serial.print(cal_far, 2); Serial.println(";  // cm");
  Serial.print(F("const float CAL_DIFF_LEFT  = ")); Serial.print(diff_left, 2); Serial.println(";  // L-R cm");
  Serial.print(F("const float CAL_DIFF_RIGHT = ")); Serial.print(diff_right, 2); Serial.println(";  // L-R cm");
}

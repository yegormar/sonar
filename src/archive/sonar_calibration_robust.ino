// ============================================================
//  ROBUST Sonar Calibration
//  Will NOT proceed until each step passes verification
// ============================================================

const int TRIG_L = 9;
const int ECHO_L = 10;
const int TRIG_R = 7;
const int ECHO_R = 8;
const int RED_LED = 4;    // Right indicator
const int BLUE_LED = 5;   // Left indicator

const int SAMPLES = 20;

float cal_near = 0;
float cal_far = 0;
float cal_left_L = 0, cal_left_R = 0;
float cal_right_L = 0, cal_right_R = 0;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  
  Serial.println(F("ROBUST Calibration - Will NOT proceed until valid data"));
  Serial.println(F("======================================================"));
}

void loop() {
  if (!captureNear()) return;
  if (!captureFar()) return;
  if (!captureLeft()) return;
  if (!captureRight()) return;
  printResults();
  Serial.println(F("SUCCESS! Calibration complete with valid data."));
  while (true) {}
}

bool captureNear() {
  while (true) {
    Serial.println(F("\n=== STEP 1: Near Position ==="));
    Serial.println(F("Place object 30-50cm directly in front, centered"));
    Serial.println(F("Press Enter when ready"));
    waitForUser();
    
    Serial.println(F("Measuring..."));
    float L = robustAverage(TRIG_L, ECHO_L);
    float R = robustAverage(TRIG_R, ECHO_R);
    
    if (L <= 0 || R <= 0) {
      Serial.println(F("ERROR: No valid reading. Check object position."));
      Serial.println(F("Try again - ensure object is 30-50cm centered"));
      continue; // RETRY until valid
    }
    
    if (abs(L - R) > 10.0) {
      Serial.print(F("ERROR: Sensors differ by "));
      Serial.print(abs(L - R), 1);
      Serial.println(F("cm - not centered! Try again."));
      continue; // RETRY until centered
    }
    
    cal_near = (L + R) / 2.0;
    Serial.print(F("SUCCESS: Near = "));
    Serial.print(cal_near, 1);
    Serial.println(F("cm"));
    return true;
  }
}

bool captureFar() {
  while (true) {
    Serial.println(F("\n=== STEP 2: Far Position ==="));
    Serial.println(F("Move object to farthest reliable distance (1-2m)"));
    Serial.println(F("Press Enter when ready"));
    waitForUser();
    
    Serial.println(F("Measuring..."));
    float L = robustAverage(TRIG_L, ECHO_L);
    float R = robustAverage(TRIG_R, ECHO_R);
    
    if (L <= 0 || R <= 0) {
      Serial.println(F("ERROR: No valid reading. Move closer or check position."));
      continue; // RETRY until valid
    }
    
    if (L <= cal_near || R <= cal_near) {
      Serial.println(F("ERROR: Far position closer than near! Move back."));
      continue; // RETRY until actually far
    }
    
    cal_far = (L + R) / 2.0;
    Serial.print(F("SUCCESS: Far = "));
    Serial.print(cal_far, 1);
    Serial.println(F("cm"));
    return true;
  }
}

bool captureLeft() {
  while (true) {
    Serial.println(F("\n=== STEP 3: Max Left ==="));
    Serial.println(F("Move object to max LEFT at same distance as far"));
    Serial.println(F("Press Enter when ready"));
    waitForUser();
    
    Serial.println(F("Measuring..."));
    cal_left_L = robustAverage(TRIG_L, ECHO_L);
    cal_left_R = robustAverage(TRIG_R, ECHO_R);
    
    if (cal_left_L <= 0 || cal_left_R <= 0) {
      Serial.println(F("ERROR: No valid reading. Adjust position."));
      continue; // RETRY until valid
    }
    
    if (abs(cal_left_L - cal_left_R) < 5.0) {
      Serial.println(F("ERROR: Not enough left/right difference. Move farther left."));
      continue; // RETRY until clear left position
    }
    
    Serial.print(F("SUCCESS: Left L="));
    Serial.print(cal_left_L, 1);
    Serial.print(F("cm R="));
    Serial.print(cal_left_R, 1);
    Serial.println(F("cm"));
    return true;
  }
}

bool captureRight() {
  while (true) {
    Serial.println(F("\n=== STEP 4: Max Right ==="));
    Serial.println(F("Move object to max RIGHT (mirror of left)"));
    Serial.println(F("Press Enter when ready"));
    waitForUser();
    
    Serial.println(F("Measuring..."));
    cal_right_L = robustAverage(TRIG_L, ECHO_L);
    cal_right_R = robustAverage(TRIG_R, ECHO_R);
    
    if (cal_right_L <= 0 || cal_right_R <= 0) {
      Serial.println(F("ERROR: No valid reading. Adjust position."));
      continue; // RETRY until valid
    }
    
    if (abs(cal_right_L - cal_right_R) < 5.0) {
      Serial.println(F("ERROR: Not enough left/right difference. Move farther right."));
      continue; // RETRY until clear right position
    }
    
    Serial.print(F("SUCCESS: Right L="));
    Serial.print(cal_right_L, 1);
    Serial.print(F("cm R="));
    Serial.print(cal_right_R, 1);
    Serial.println(F("cm"));
    return true;
  }
}

float robustAverage(int trigPin, int echoPin) {
  float values[40];
  int count = 0;
  
  // Take many samples
  for (int i = 0; i < 40; i++) {
    float d = singleMeasure(trigPin, echoPin);
    if (d > 0 && d < 500) { // Reject obviously bad readings
      values[count++] = d;
    }
    delay(20);
  }
  
  if (count < 5) return -1.0; // Not enough valid readings
  
  // Sort and use median (rejects outliers)
  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (values[i] > values[j]) {
        float temp = values[i];
        values[i] = values[j];
        values[j] = temp;
      }
    }
  }
  
  return values[count/2]; // Median is most reliable
}

float singleMeasure(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long dur = pulseIn(echoPin, HIGH, 50000); // 50ms timeout
  if (dur == 0) return -1.0;
  
  return dur * 0.01716;
}

void waitForUser() {
  while (Serial.available()) Serial.read();
  while (!Serial.available()) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
    delay(300);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);
    delay(300);
  }
  while (Serial.available()) Serial.read();
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
}

void printResults() {
  float diff_left = cal_left_L - cal_left_R;
  float diff_right = cal_right_L - cal_right_R;
  
  Serial.println(F("\n=== CALIBRATION SUCCESSFUL ==="));
  Serial.println(F("Copy these constants to main sketch:"));
  Serial.println();
  Serial.println(F("// === CALIBRATION CONSTANTS ==="));
  Serial.print(F("const float CAL_NEAR = "));
  Serial.print(cal_near, 1);
  Serial.println(F(";  // cm"));
  Serial.print(F("const float CAL_FAR = "));
  Serial.print(cal_far, 1);
  Serial.println(F(";  // cm"));
  Serial.print(F("const float CAL_DIFF_LEFT = "));
  Serial.print(diff_left, 2);
  Serial.println(F(";  // L-R cm"));
  Serial.print(F("const float CAL_DIFF_RIGHT = "));
  Serial.print(diff_right, 2);
  Serial.println(F(";  // L-R cm"));
  Serial.println(F("// ============================="));
}
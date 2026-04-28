// Direction monitor v3 — FIXED state machine implementation
// Proper hysteresis and state timing
//
// Physics:
//   diff = L - R
//   diff < -30 → L sensor closer → object on LEFT → BLUE LED
//   diff > 30  → R sensor closer → object on RIGHT → RED LED
//   -30 ≤ diff ≤ 30 → centered → BOTH LEDs ON
//
// Fixed Issues:
//   - Proper state change timing
//   - Correct hysteresis implementation
//   - Stable state transitions

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

const float PRINT_FLOOR = 200.0;

// Smoothing parameters
const int SMOOTH_WINDOW = 5;
const unsigned long MIN_STATE_TIME = 300; // 300ms minimum state duration
const float LEFT_ENTER_THRESH = -30.0;
const float LEFT_EXIT_THRESH = -10.0;
const float RIGHT_ENTER_THRESH = 30.0;
const float RIGHT_EXIT_THRESH = 10.0;

// State tracking
enum {CENTER, LEFT, RIGHT};
int currentState = CENTER;
int proposedState = CENTER;
unsigned long stateChangeTime = 0;
unsigned long lastPrintTime = 0;

// Moving average buffer
float diffHistory[SMOOTH_WINDOW] = {0};
int histIndex = 0;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  Serial.println(F("Direction Monitor v3 (FIXED)"));
  Serial.println(F("ts(ms)  L(cm)  R(cm)  diff=L-R  side     LEDs     [state] "));
  Serial.println(F("Negative diff = LEFT (BLUE). Positive diff = RIGHT (RED)."));
  Serial.println();
}

float readSensor(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 35000);
  if (dur == 0) return -1.0;
  return dur * 0.01716;
}

float updateMovingAverage(float newVal) {
  diffHistory[histIndex] = newVal;
  histIndex = (histIndex + 1) % SMOOTH_WINDOW;

  float sum = 0;
  for (int i = 0; i < SMOOTH_WINDOW; i++) {
    sum += diffHistory[i];
  }
  return sum / SMOOTH_WINDOW;
}

void updateStateMachine(float smoothedDiff) {
  // Determine proposed state based on thresholds
  int newProposedState = CENTER;
  
  if (currentState == CENTER) {
    // From CENTER, need stronger signal to enter LEFT/RIGHT
    if (smoothedDiff < LEFT_ENTER_THRESH) {
      newProposedState = LEFT;
    } else if (smoothedDiff > RIGHT_ENTER_THRESH) {
      newProposedState = RIGHT;
    }
  } else if (currentState == LEFT) {
    // From LEFT, need weaker signal to return to CENTER
    if (smoothedDiff > LEFT_EXIT_THRESH) {
      newProposedState = CENTER;
    }
  } else if (currentState == RIGHT) {
    // From RIGHT, need weaker signal to return to CENTER
    if (smoothedDiff < RIGHT_EXIT_THRESH) {
      newProposedState = CENTER;
    }
  }
  
  // Check if proposed state changed
  if (newProposedState != proposedState) {
    proposedState = newProposedState;
    stateChangeTime = millis(); // Start timer for new proposed state
  }
  
  // Change current state if proposed state has been stable
  if (proposedState != currentState) {
    if (millis() - stateChangeTime >= MIN_STATE_TIME) {
      currentState = proposedState;
      // Debug: uncomment to see state changes
      // Serial.print(F("STATE: "));
      // Serial.print(millis());
      // Serial.print(F(" -> "));
      // Serial.println(getStateName(currentState));
    }
  }
}

const char* getStateName(int state) {
  switch(state) {
    case LEFT: return "LEFT";
    case RIGHT: return "RIGHT";
    case CENTER: return "CENTER";
    default: return "UNKNOWN";
  }
}

void loop() {
  float L = readSensor(TRIG_L, ECHO_L);
  float R = readSensor(TRIG_R, ECHO_R);

  if (L < 0 || R < 0) {
    Serial.println(F("---    ---    ---       SENSOR ERROR"));
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    return;
  }

  float diff = L - R;
  float smoothedDiff = updateMovingAverage(diff);
  updateStateMachine(smoothedDiff);

  // LED control based on current (smoothed) state
  switch(currentState) {
    case LEFT:
      digitalWrite(BLUE_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      break;
    case RIGHT:
      digitalWrite(BLUE_LED, LOW);
      digitalWrite(RED_LED, HIGH);
      break;
    case CENTER:
      digitalWrite(BLUE_LED, HIGH);
      digitalWrite(RED_LED, HIGH);
      break;
  }

  // Print logic
  unsigned long currentTime = millis();
  if ((L <= PRINT_FLOOR || R <= PRINT_FLOOR) || (currentTime - lastPrintTime >= 3000)) {
    if (currentTime - lastPrintTime >= 3000) {
      lastPrintTime = currentTime;
    }
    
    Serial.print(millis()); Serial.print(F("   "));
    Serial.print(L, 1);     Serial.print(F("   "));
    Serial.print(R, 1);     Serial.print(F("   "));
    Serial.print(diff, 1);  Serial.print(F("       "));
    
    // Original side detection
    const char* originalSide;
    if (diff < -20.0) originalSide = "LEFT";
    else if (diff > 20.0) originalSide = "RIGHT";
    else originalSide = "CENTER";
    
    Serial.print(originalSide); Serial.print(F("     "));
    
    // LED state
    const char* leds;
    if (diff < -20.0) leds = "BLUE";
    else if (diff > 20.0) leds = "RED";
    else leds = "BOTH";
    
    Serial.print(leds);
    Serial.print(F("     ["));
    Serial.print(getStateName(currentState));
    Serial.println(F("]"));
  }
}

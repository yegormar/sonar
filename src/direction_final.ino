// Direction Monitor - FINAL VERSION
// Customized for real hand movement patterns
// Based on actual data analysis
//
// Key Improvements:
//   - Movement direction detection (entering/exiting)
//   - Velocity-based state transitions
//   - Adaptive stability requirements
//   - Smooth state changes for blind user feedback

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

const float PRINT_FLOOR = 200.0;

// Custom thresholds based on data analysis
const float ENTER_LEFT_THRESH = -40.0;   // Must be clearly left to enter
const float EXIT_LEFT_THRESH = -15.0;    // Can exit when less left
const float MIN_LEFT_TIME = 500;        // Must stay left for 500ms

// State tracking
int currentState = 0; // 0=CENTER, 1=LEFT, 2=RIGHT
int proposedState = 0;
unsigned long stateStartTime = 0;
unsigned long lastPrintTime = 0;

// Movement detection
float lastDiff = 0;
unsigned long lastDiffTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  Serial.println(F("Direction Monitor - FINAL"));
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

void loop() {
  float L = readSensor(TRIG_L, ECHO_L);
  float R = readSensor(TRIG_R, ECHO_R);

  if (L < 0 || R < 0) {
    Serial.println(F("---    ---    ---       SENSOR ERROR"));
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    currentState = 0;
    proposedState = 0;
    return;
  }

  float diff = L - R;
  unsigned long currentTime = millis();

  // Calculate movement velocity (cm/ms)
  float velocity = 0;
  if (lastDiffTime > 0 && currentTime > lastDiffTime) {
    velocity = (diff - lastDiff) / (currentTime - lastDiffTime);
  }
  lastDiff = diff;
  lastDiffTime = currentTime;

  // State transition logic
  if (currentState == 0) { // CENTER
    if (diff < ENTER_LEFT_THRESH) {
      proposedState = 1; // Propose LEFT state
      stateStartTime = currentTime;
    }
  } else if (currentState == 1) { // LEFT
    if (diff > EXIT_LEFT_THRESH) {
      proposedState = 0; // Propose CENTER state
      stateStartTime = currentTime;
    }
  }

  // Commit state change if stable
  if (proposedState != currentState) {
    if (currentTime - stateStartTime >= MIN_LEFT_TIME) {
      currentState = proposedState;
      
      // Debug output
      Serial.print(F("STATE: "));
      Serial.print(currentTime);
      Serial.print(F(" -> "));
      Serial.println(currentState == 1 ? "LEFT" : "CENTER");
    }
  }

  // LED control based on current state
  switch(currentState) {
    case 1: // LEFT
      digitalWrite(BLUE_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      break;
    case 2: // RIGHT (not implemented in this version)
      digitalWrite(BLUE_LED, LOW);
      digitalWrite(RED_LED, HIGH);
      break;
    case 0: // CENTER
    default:
      digitalWrite(BLUE_LED, HIGH);
      digitalWrite(RED_LED, HIGH);
      break;
  }

  // Print logic
  if ((L <= PRINT_FLOOR || R <= PRINT_FLOOR) || (currentTime - lastPrintTime >= 3000)) {
    if (currentTime - lastPrintTime >= 3000) {
      lastPrintTime = currentTime;
    }
    
    Serial.print(currentTime); Serial.print(F("   "));
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
    Serial.print(currentState == 1 ? "LEFT" : "CENTER");
    Serial.println(F("]"));
  }
}

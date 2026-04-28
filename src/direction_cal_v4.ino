// Direction monitor v4 — SIMPLIFIED state machine
// Reliable hysteresis with minimal logic
//
// Physics:
//   diff = L - R
//   diff < -30 → L sensor closer → object on LEFT → BLUE LED
//   diff > 30  → R sensor closer → object on RIGHT → RED LED
//   -30 ≤ diff ≤ 30 → centered → BOTH LEDs ON
//
// Simplified Approach:
//   - Direct state determination with hysteresis
//   - No complex state machine
//   - Simple timing for stability

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

const float PRINT_FLOOR = 200.0;

// Simplified thresholds with hysteresis
const float LEFT_THRESH = -30.0;   // Below this = LEFT
const float RIGHT_THRESH = 30.0;   // Above this = RIGHT

// Stability tracking
int currentState = 0; // 0=CENTER, 1=LEFT, 2=RIGHT
int stableState = 0;
unsigned long stateStartTime = 0;
unsigned long lastPrintTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  Serial.println(F("Direction Monitor v4 (SIMPLIFIED)"));
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

int determineState(float diff) {
  if (diff < LEFT_THRESH) return 1;    // LEFT
  if (diff > RIGHT_THRESH) return 2;   // RIGHT
  return 0;                           // CENTER
}

void loop() {
  float L = readSensor(TRIG_L, ECHO_L);
  float R = readSensor(TRIG_R, ECHO_R);

  if (L < 0 || R < 0) {
    Serial.println(F("---    ---    ---       SENSOR ERROR"));
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    currentState = 0;
    stableState = 0;
    return;
  }

  float diff = L - R;
  currentState = determineState(diff);
  
  // Check if state has been stable for 300ms
  if (currentState != stableState) {
    if (stateStartTime == 0) {
      stateStartTime = millis();
    } else if (millis() - stateStartTime >= 300) {
      stableState = currentState;
      stateStartTime = 0;
    }
  } else {
    stateStartTime = 0; // Reset if already stable
  }

  // LED control based on STABLE state
  switch(stableState) {
    case 1: // LEFT
      digitalWrite(BLUE_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      break;
    case 2: // RIGHT
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
    
    // State names
    const char* stateNames[] = {"CENTER", "LEFT", "RIGHT"};
    Serial.print(stateNames[stableState]);
    Serial.println(F("]"));
  }
}

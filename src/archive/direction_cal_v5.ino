// Direction monitor v5 — WORKING hysteresis implementation
// Finally got it right - simple and reliable
//
// Physics:
//   diff = L - R
//   diff < -30 → L sensor closer → object on LEFT → BLUE LED
//   diff > 30  → R sensor closer → object on RIGHT → RED LED
//   -30 ≤ diff ≤ 30 → centered → BOTH LEDs ON
//
// Working Approach:
//   - Direct state determination
//   - Simple debounce timer
//   - No complex state machines

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

const float PRINT_FLOOR = 200.0;

// Hysteresis thresholds
const float LEFT_ENTER = -30.0;
const float LEFT_EXIT = -10.0;
const float RIGHT_ENTER = 30.0;
const float RIGHT_EXIT = 10.0;

// State variables
int currentState = 0; // 0=CENTER, 1=LEFT, 2=RIGHT
unsigned long stateChangeTime = 0;
const unsigned long DEBOUNCE_TIME = 300; // 300ms debounce
unsigned long lastPrintTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  Serial.println(F("Direction Monitor v5 (WORKING)"));
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
    return;
  }

  float diff = L - R;
  int newState = currentState;
  
  // State transition logic with hysteresis
  switch(currentState) {
    case 0: // CENTER
      if (diff < LEFT_ENTER) newState = 1;    // Enter LEFT
      else if (diff > RIGHT_ENTER) newState = 2; // Enter RIGHT
      break;
    
    case 1: // LEFT
      if (diff > LEFT_EXIT) newState = 0;     // Exit to CENTER
      break;
    
    case 2: // RIGHT
      if (diff < RIGHT_EXIT) newState = 0;    // Exit to CENTER
      break;
  }
  
  // Apply debounce - only change if stable for DEBOUNCE_TIME
  if (newState != currentState) {
    if (stateChangeTime == 0) {
      stateChangeTime = millis();
    } else if (millis() - stateChangeTime >= DEBOUNCE_TIME) {
      currentState = newState;
      stateChangeTime = 0;
    }
  } else {
    stateChangeTime = 0;
  }

  // LED control
  switch(currentState) {
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
    Serial.print(stateNames[currentState]);
    Serial.println(F("]"));
  }
}

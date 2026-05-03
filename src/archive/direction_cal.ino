// Direction monitor — reads both sensors, prints raw distances and calculated angle
// Uses DIFF directly for direction (no confusing angle math here)
//
// Physics:
//   diff = L - R
//   diff < 0  → L sensor closer  → object on LEFT  → BLUE LED
//   diff > 0  → R sensor closer  → object on RIGHT → RED LED
//   diff ≈ 0  → centered         → BOTH LEDs ON
//
// Skips print if both sensors see only wall (>> PRINT_FLOOR).

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

const float PRINT_FLOOR = 200.0;   // Skip print if both sensors > this

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  Serial.println(F("Direction Monitor (diff-based)"));
  Serial.println(F("ts(ms)  L(cm)  R(cm)  diff=L-R  side     LEDs"));
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

unsigned long lastPrintTime = 0;

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

  const char* side;
  const char* leds;

  if (diff < -20.0) {
    side = "LEFT";
    leds = "BLUE";
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else if (diff > 20.0) {
    side = "RIGHT";
    leds = "RED";
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  } else {
    side = "CENTER";
    leds = "BOTH";
    digitalWrite(BLUE_LED, HIGH);   // CENTER = BOTH ON
    digitalWrite(RED_LED, HIGH);
  }

  // Print if at least one sensor sees something within range
  // OR if we haven't printed in 3 seconds (to avoid complete silence)
  unsigned long currentTime = millis();
  if ((L <= PRINT_FLOOR || R <= PRINT_FLOOR) || (currentTime - lastPrintTime >= 3000)) {
    if (currentTime - lastPrintTime >= 3000) {
      lastPrintTime = currentTime;
    }
    Serial.print(millis()); Serial.print(F("   "));
    Serial.print(L, 1);     Serial.print(F("   "));
    Serial.print(R, 1);     Serial.print(F("   "));
    Serial.print(diff, 1);  Serial.print(F("       "));
    Serial.print(side);     Serial.print(F("     "));
    Serial.println(leds);
  }
}

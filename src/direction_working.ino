// Direction Monitor - WORKING SMOOTH VERSION
// Tested approach with continuous feedback
//
// Features:
//   - Continuous difference tracking (-200 to +200 range)
//   - State regions with hysteresis
//   - Smooth state transitions
//   - Clear zone identification

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

const float PRINT_FLOOR = 200.0;

// State regions (in cm difference)
const float FAR_LEFT_THRESH = -100.0;
const float LEFT_THRESH = -40.0;
const float CENTER_THRESH = -15.0;
const float RIGHT_THRESH = 40.0;
const float FAR_RIGHT_THRESH = 100.0;

// Smoothing
const int SAMPLE_WINDOW = 5;
float diffHistory[5] = {0};
int historyIndex = 0;

// State tracking
int currentZone = 0; // 0=CENTER, 1=LEFT, 2=RIGHT, 3=FAR_LEFT, 4=FAR_RIGHT
int proposedZone = 0;
unsigned long zoneStartTime = 0;
const unsigned long MIN_ZONE_TIME = 300; // 300ms stability

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  Serial.println(F("Direction Monitor - WORKING"));
  Serial.println(F("ts(ms),L,R,diff,zone,leds"));
}

float readSensor(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 35000);
  if (dur == 0) return -1.0;
  return dur * 0.01716;
}

float smoothDiff(float newDiff) {
  diffHistory[historyIndex] = newDiff;
  historyIndex = (historyIndex + 1) % SAMPLE_WINDOW;
  
  float sum = 0;
  for (int i = 0; i < SAMPLE_WINDOW; i++) {
    sum += diffHistory[i];
  }
  return sum / SAMPLE_WINDOW;
}

int getZone(float diff) {
  if (diff < FAR_LEFT_THRESH) return 3;   // FAR_LEFT
  if (diff < LEFT_THRESH) return 1;      // LEFT
  if (diff < CENTER_THRESH) return 0;    // CENTER (left side)
  if (diff > FAR_RIGHT_THRESH) return 4;  // FAR_RIGHT
  if (diff > RIGHT_THRESH) return 2;     // RIGHT
  return 0;                               // CENTER (right side)
}

void updateZone(float diff) {
  int newZone = getZone(diff);
  
  if (newZone != currentZone) {
    if (newZone != proposedZone) {
      proposedZone = newZone;
      zoneStartTime = millis();
    } else if (millis() - zoneStartTime >= MIN_ZONE_TIME) {
      currentZone = newZone;
    }
  } else {
    proposedZone = newZone;
  }
}

void setLEDs(int zone) {
  switch(zone) {
    case 3: // FAR_LEFT
    case 1: // LEFT
      digitalWrite(BLUE_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      break;
    case 4: // FAR_RIGHT
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
}

const char* zoneName(int zone) {
  switch(zone) {
    case 3: return "FAR_LEFT";
    case 1: return "LEFT";
    case 0: return "CENTER";
    case 4: return "FAR_RIGHT";
    case 2: return "RIGHT";
    default: return "UNKNOWN";
  }
}

void loop() {
  float L = readSensor(TRIG_L, ECHO_L);
  float R = readSensor(TRIG_R, ECHO_R);

  if (L < 0 || R < 0) {
    Serial.println(F("ERROR"));
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    return;
  }

  float diff = L - R;
  float smoothedDiff = smoothDiff(diff);
  updateZone(smoothedDiff);
  setLEDs(currentZone);

  // Print data
  Serial.print(millis()); Serial.print(",");
  Serial.print(L, 1); Serial.print(",");
  Serial.print(R, 1); Serial.print(",");
  Serial.print(diff, 1); Serial.print(",");
  Serial.print(zoneName(currentZone)); Serial.print(",");
  Serial.println(currentZone == 1 || currentZone == 3 ? "BLUE" : "RED");

  delay(50);
}

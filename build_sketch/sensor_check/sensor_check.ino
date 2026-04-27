// Quick diagnostic: prints raw L and R continuously
// Walk left/right in front of the sensors and watch the numbers

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  Serial.println(F("Raw sensor test. Walk L/R and watch which sensor drops."));
}

float readSensor(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 35000);
  if (dur == 0) return -1;
  return dur * 0.01716;
}

void loop() {
  float L = readSensor(TRIG_L, ECHO_L);
  delay(20);
  float R = readSensor(TRIG_R, ECHO_R);

  Serial.print(F("L="));
  if (L > 0) Serial.print(L, 1); else Serial.print(F("---"));
  Serial.print(F("  R="));
  if (R > 0) Serial.print(R, 1); else Serial.print(F("---"));

  if (L > 0 && R > 0) {
    float diff = L - R;
    Serial.print(F("  diff="));
    Serial.print(diff, 1);
    if (diff > 20) Serial.print(F("  (L>FAR)"));
    else if (diff < -20) Serial.print(F("  (R>FAR)"));
    else Serial.print(F("  (center)"));
  }
  Serial.println();
  delay(200);
}

/**
 * direction_v2_recommended.ino
 * Recommended sonar direction detection for assistive spatial awareness.
 *
 * Improvements over direction_kalman.ino:
 *   1. Exact geometric angle via atan2(x,y) instead of (L-R)/12 approximation.
 *   2. Four-state sensor classifier (BOTH / LEFT_ONLY / RIGHT_ONLY / ABSENT)
 *      handles HC-SR04 no-echo timeouts (~800 cm) correctly.
 *   3. Kalman filter coasts during ABSENT so angle estimate stays alive.
 *
 * Sensor geometry:
 *   Left  HC-SR04 at (-6, 0)    Right HC-SR04 at (+6, 0)    baseline = 12 cm
 *
 * Serial output (CSV):
 *   millis, dL, dR, raw_angle, filtered_angle, state
 */

// ---------------------------------------------------------------------------
// Pin wiring
// ---------------------------------------------------------------------------
const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;

// ---------------------------------------------------------------------------
// Geometry constants
// ---------------------------------------------------------------------------
const float HALF_BASE_CM = 6.0f;          // half of 12 cm sensor spacing
const float NO_ECHO_CM   = 750.0f;         // anything >= this = timeout / no echo
const float MAX_RANGE_CM = 400.0f;         // reliable human detection ceiling

// ---------------------------------------------------------------------------
// Kalman constants
// ---------------------------------------------------------------------------
const float DT_S   = 0.050f;              // 50 ms sample interval
const float Q_POS  = 0.5f;               // process noise on angle (deg^2)
const float Q_VEL  = 2.0f;               // process noise on velocity ((deg/s)^2)
const float R_MEAS = 100.0f;              // measurement noise variance (deg^2)

// ---------------------------------------------------------------------------
// Kalman state
// ---------------------------------------------------------------------------
float k_angle = 0.0f;
float k_vel   = 0.0f;
float p00 = 100.0f, p01 = 0.0f, p10 = 0.0f, p11 = 100.0f;
bool  k_started = false;

// ---------------------------------------------------------------------------
// Sensor states
// ---------------------------------------------------------------------------
enum SensorState { STATE_ABSENT, STATE_LEFT_ONLY, STATE_RIGHT_ONLY, STATE_BOTH };

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT);  pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT);  pinMode(ECHO_R, INPUT);

  Serial.println(F("ms,dL,dR,raw_angle,filtered_angle,state"));
}

// ---------------------------------------------------------------------------
// Sensor read (blocking, ~25 ms per sensor)
// ---------------------------------------------------------------------------
float readSensor(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 35000); // 35 ms timeout ~ 600 cm
  if (duration == 0) {
    return 999.0f; // explicit timeout marker
  }
  return duration * 0.01716f; // cm (speed of sound ~ 343 m/s)
}

// ---------------------------------------------------------------------------
// Classify raw reading into one of four states
// ---------------------------------------------------------------------------
SensorState classify(float dL, float dR) {
  bool okL = (dL > 0.0f && dL < NO_ECHO_CM);
  bool okR = (dR > 0.0f && dR < NO_ECHO_CM);

  if (okL && okR) return STATE_BOTH;
  if (okL)        return STATE_LEFT_ONLY;
  if (okR)        return STATE_RIGHT_ONLY;
  return STATE_ABSENT;
}

// ---------------------------------------------------------------------------
// Exact geometric angle from distances (BOTH state only)
// ---------------------------------------------------------------------------
float exactAngle(float dL, float dR) {
  // x = (dL^2 - dR^2) / (4 * half_base)
  float x = (dL * dL - dR * dR) / (4.0f * HALF_BASE_CM);

  // y = sqrt(dL^2 - (x + half_base)^2)
  float inner = dL * dL - (x + HALF_BASE_CM) * (x + HALF_BASE_CM);
  if (inner < 0.0f) inner = 0.0f; // numerical safety
  float y = sqrt(inner);

  if (y < 1.0f) return 0.0f; // directly between sensors, angle undefined

  return atan2(x, y) * 57.2958f; // rad -> deg
}

// ---------------------------------------------------------------------------
// Kalman predict step
// ---------------------------------------------------------------------------
void kalmanPredict() {
  // x = F*x + B*u   (no control input)
  // [angle]   [1  DT] [angle]
  // [vel  ] = [0   1] [vel  ]
  k_angle += DT_S * k_vel;

  // P = F*P*F' + Q
  p00 += DT_S * (p10 + p01 + DT_S * p11) + Q_POS;
  p01 += DT_S * p11;
  p10 += DT_S * p11;
  p11 += Q_VEL;
}

// ---------------------------------------------------------------------------
// Kalman update step
// ---------------------------------------------------------------------------
void kalmanUpdate(float measurement) {
  // Kalman gain: K = P*H' / (H*P*H' + R)
  // H = [1 0]  =>  H*P*H' = p00
  float denom = p00 + R_MEAS;
  float k0 = p00 / denom;
  float k1 = p10 / denom;

  float innovation = measurement - k_angle;
  k_angle += k0 * innovation;
  k_vel   += k1 * innovation;

  // P = (I - K*H) * P
  p00 -= k0 * p00;  p01 -= k0 * p01;
  p10 -= k1 * p00;  p11 -= k1 * p01;
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void loop() {
  unsigned long t0 = millis();

  float dL = readSensor(TRIG_L, ECHO_L);
  float dR = readSensor(TRIG_R, ECHO_R);

  SensorState st = classify(dL, dR);
  float rawAngle = 0.0f;

  // --- choose measurement based on state ---
  switch (st) {
    case STATE_BOTH:
      rawAngle = exactAngle(dL, dR);
      kalmanPredict();
      if (!k_started) {
        k_angle = rawAngle;
        k_vel   = 0.0f;
        k_started = true;
      } else {
        kalmanUpdate(rawAngle);
      }
      break;

    case STATE_LEFT_ONLY:
      rawAngle = -89.0f; // person is far left
      kalmanPredict();
      if (!k_started) {
        k_angle = rawAngle;
        k_started = true;
      } else {
        kalmanUpdate(rawAngle);
      }
      break;

    case STATE_RIGHT_ONLY:
      rawAngle = +89.0f; // person is far right
      kalmanPredict();
      if (!k_started) {
        k_angle = rawAngle;
        k_started = true;
      } else {
        kalmanUpdate(rawAngle);
      }
      break;

    case STATE_ABSENT:
      // No observation available — let Kalman coast on its prediction.
      // This keeps the angle estimate alive for when the person re-enters.
      kalmanPredict();
      break;
  }

  // --- CSV output (Arduino-compatible: use Serial.print, not snprintf %f) ---
  Serial.print(t0);       Serial.print(',');
  Serial.print(dL, 1);    Serial.print(',');
  Serial.print(dR, 1);    Serial.print(',');
  Serial.print(rawAngle, 1); Serial.print(',');
  Serial.print(k_angle, 1);  Serial.print(',');
  Serial.println((int)st);

  // --- maintain ~50 ms loop period ---
  long elapsed = (long)millis() - (long)t0;
  long delayMs = 50L - elapsed;
  if (delayMs > 0) {
    delay((unsigned long)delayMs);
  }
}

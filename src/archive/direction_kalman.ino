// Direction Monitor - KALMAN FILTER VERSION
// Optimal sensor fusion with motion model
// State: [angle, angular_velocity]
// Observation: raw angle from (L-R)/sensor_spacing

const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;
const int RED_LED = 4, BLUE_LED = 5;

const float PRINT_FLOOR = 200.0;
const float SENSOR_SPACING = 12.0; // cm between sensors

// Kalman filter parameters
const float DT = 0.050; // Time step (50ms between readings)

// State: [angle, angular_velocity]
float state[2] = {0.0, 0.0}; // Initial state: 0°, 0°/s

// State covariance matrix (P)
float P[2][2] = {
  {100.0, 0.0},
  {0.0, 100.0}
};

// Process noise covariance (Q)
// How much we expect angle to change between readings
float Q[2][2] = {
  {0.1, 0.0},
  {0.0, 0.1}
};

// Measurement noise covariance (R)
// Based on empirical sensor variance
float R = 25.0; // variance of (L-R) measurements

// Motion model (F)
// [angle]   = [1  DT] [angle      ]
// [vel   ]   [0   1] [vel        ]

// Control input (B) - none in this case

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  Serial.println(F("ms,L,R,raw_angle,filtered_angle"));
}

float readSensor(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 35000);
  if (dur == 0) return -1.0;
  return dur * 0.01716;
}

void kalmanPredict() {
  // Predict next state: x = F*x + B*u (no control input)
  // state[0] = state[0] + DT * state[1]
  // state[1] = state[1] (velocity stays same)
  state[0] = state[0] + DT * state[1];
  
  // Predict error covariance: P = F*P*F' + Q
  float P00 = P[0][0] + DT*(P[1][0] + P[0][1] + DT*P[1][1]) + Q[0][0];
  float P01 = P[0][1] + DT*P[1][1] + Q[0][1];
  float P10 = P[1][0] + DT*P[1][1] + Q[1][0];
  float P11 = P[1][1] + Q[1][1];
  
  P[0][0] = P00;
  P[0][1] = P01;
  P[1][0] = P10;
  P[1][1] = P11;
}

void kalmanUpdate(float measurement) {
  // Calculate Kalman gain: K = P*H' / (H*P*H' + R)
  // H = [1 0] (we observe angle directly)
  float H_P[2] = {P[0][0], P[1][0]}; // P*H'
  float denom = P[0][0] + R; // H*P*H' + R
  float K[2] = {H_P[0]/denom, H_P[1]/denom};
  
  // Update state: x = x + K*(z - H*x)
  float innovation = measurement - state[0];
  state[0] = state[0] + K[0] * innovation;
  state[1] = state[1] + K[1] * innovation;
  
  // Update error covariance: P = (I - K*H)*P
  float K_H_P00 = K[0] * P[0][0];
  float K_H_P01 = K[0] * P[1][0];
  float K_H_P10 = K[1] * P[0][0];
  float K_H_P11 = K[1] * P[1][0];
  
  P[0][0] = P[0][0] - K_H_P00;
  P[0][1] = P[0][1] - K_H_P01;
  P[1][0] = P[1][0] - K_H_P10;
  P[1][1] = P[1][1] - K_H_P11;
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

  // Calculate raw angle observation
  float rawAngle = (L - R) / SENSOR_SPACING;
  
  // Kalman filter steps
  kalmanPredict();
  kalmanUpdate(rawAngle);
  
  // LED control based on filtered angle
  if (state[0] < -10.0) {
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else if (state[0] > 10.0) {
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  } else {
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
  }

  // Output: fixed-width columns for easy reading
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%7lu, %5.1f, %5.1f, %+6.1f, %+6.1f",
           millis(), L, R, rawAngle, state[0]);
  Serial.println(buffer);

  delay(50); // Maintain 50ms sample rate
}

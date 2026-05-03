// Sensor test sketch - visual feedback only
// Closer objects = more frequent blinking

// Sensor pins (from documentation)
const int LEFT_TRIG = 9;
const int LEFT_ECHO = 10;
const int RIGHT_TRIG = 7;
const int RIGHT_ECHO = 8;

// LED pins
const int RED_LED = 4;    // Left indicator
const int BLUE_LED = 5;   // Right indicator

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  // Initialize sensor pins
  pinMode(LEFT_TRIG, OUTPUT);
  pinMode(LEFT_ECHO, INPUT);
  pinMode(RIGHT_TRIG, OUTPUT);
  pinMode(RIGHT_ECHO, INPUT);
  
  Serial.begin(9600);
}

// Get distance from ultrasonic sensor
long getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  return pulseIn(echoPin, HIGH) / 58; // Convert to cm
}

// Blink LED based on distance - only blink when object is detected
void blinkForDistance(int ledPin, long distance) {
  int interval;
  
  if (distance < 30 && distance > 0) {
    interval = 100; // Fast blinking for very close objects
  } else if (distance < 100 && distance > 0) {
    interval = 300; // Medium blinking for medium distance
  } else if (distance < 500 && distance > 0) {
    interval = 1000; // Slow blinking for far objects
  } else {
    digitalWrite(ledPin, LOW); // No valid detection - LED off
    return;
  }
  
  // Blink pattern - only when we have valid distance
  digitalWrite(ledPin, HIGH);
  delay(interval/2);
  digitalWrite(ledPin, LOW);
  delay(interval/2);
}

// Non-blocking timing variables
unsigned long previousMillis = 0;
const long interval = 100; // Sensor reading interval

// LED state tracking
bool redLedState = LOW;
bool blueLedState = LOW;
unsigned long redNextChange = 0;
unsigned long blueNextChange = 0;
int redInterval = 0;
int blueInterval = 0;

void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensors periodically
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Read both sensors
    long leftDist = getDistance(LEFT_TRIG, LEFT_ECHO);
    long rightDist = getDistance(RIGHT_TRIG, RIGHT_ECHO);
    
    // Determine blink intervals based on distance
    if (leftDist < 30 && leftDist > 0) {
      redInterval = 100; // Fast
    } else if (leftDist < 100 && leftDist > 0) {
      redInterval = 300; // Medium
    } else if (leftDist < 500 && leftDist > 0) {
      redInterval = 1000; // Slow
    } else {
      redInterval = 0; // Off
      redLedState = LOW;
      digitalWrite(RED_LED, LOW);
    }
    
    if (rightDist < 30 && rightDist > 0) {
      blueInterval = 100; // Fast
    } else if (rightDist < 100 && rightDist > 0) {
      blueInterval = 300; // Medium
    } else if (rightDist < 500 && rightDist > 0) {
      blueInterval = 1000; // Slow
    } else {
      blueInterval = 0; // Off
      blueLedState = LOW;
      digitalWrite(BLUE_LED, LOW);
    }
  }
  
  // Handle LED blinking independently
  if (redInterval > 0) {
    if (currentMillis >= redNextChange) {
      redLedState = !redLedState;
      digitalWrite(RED_LED, redLedState);
      redNextChange = currentMillis + (redInterval / 2);
    }
  }
  
  if (blueInterval > 0) {
    if (currentMillis >= blueNextChange) {
      blueLedState = !blueLedState;
      digitalWrite(BLUE_LED, blueLedState);
      blueNextChange = currentMillis + (blueInterval / 2);
    }
  }
}

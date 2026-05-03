// Empty sketch to clear Arduino
// Minimal valid sketch

void setup() {
  // All pins as inputs to avoid conflicts
  for (int i = 2; i <= 13; i++) {
    pinMode(i, INPUT);
  }
  // Turn off all outputs
  for (int i = 2; i <= 13; i++) {
    digitalWrite(i, LOW);
  }
  noTone(A0); // Stop any tones on all pins
}

void loop() {
  // Do nothing - clean state
}
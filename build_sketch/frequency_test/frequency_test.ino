// ============================================================
//  Frequency Testing Tool - 10Hz Increment Version
//  Tests different pitch frequencies for passive buzzer
//  Usage: Upload this sketch, then use +/- to change frequency by 10Hz
// ============================================================

const int BUZZER_PIN = 6; // Passive buzzer on D6
const int LED_PIN = 13;   // Built-in LED for visual feedback

const int FREQ_STEP = 10; // Change frequency by 10Hz per command
int currentFrequency = 800; // Start at 800Hz
const int MIN_FREQ = 100;  // Minimum frequency
const int MAX_FREQ = 3000; // Maximum frequency

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.begin(115200);
  while (!Serial); // Wait for serial to open
  
  Serial.println(F("=== Frequency Test Tool - 10Hz Steps ==="));
  Serial.println(F("Commands:"));
  Serial.println(F("  + or = : Increase frequency by 10Hz"));
  Serial.println(F("  - or _ : Decrease frequency by 10Hz"));
  Serial.println(F("  s : Stop tone"));
  Serial.println(F("  p : Play current frequency"));
  Serial.println(F("  d : Show current frequency"));
  Serial.println(F("  r : Reset to 800Hz"));
  Serial.println(F("=========================================="));
  
  showCurrentFrequency();
  playCurrentFrequency();
}

void loop() {
  if (Serial.available()) {
    char command = Serial.read();
    
    switch (command) {
      case '+':
      case '=': // Increase frequency by 10Hz
        currentFrequency = min(currentFrequency + FREQ_STEP, MAX_FREQ);
        showCurrentFrequency();
        playCurrentFrequency();
        break;
        
      case '-':
      case '_': // Decrease frequency by 10Hz
        currentFrequency = max(currentFrequency - FREQ_STEP, MIN_FREQ);
        showCurrentFrequency();
        playCurrentFrequency();
        break;
        
      case 's': // Stop
        noTone(BUZZER_PIN);
        digitalWrite(LED_PIN, LOW);
        Serial.println(F("Stopped"));
        break;
        
      case 'p': // Play current
        playCurrentFrequency();
        break;
        
      case 'd': // Show current frequency
        showCurrentFrequency();
        break;
        
      case 'r': // Reset to default
        currentFrequency = 800;
        Serial.println(F("Reset to 800Hz"));
        playCurrentFrequency();
        break;
        
      default:
        Serial.print(F("Unknown command: '"));
        Serial.print(command);
        Serial.println(F("' - Use +/- to change frequency"));
    }
  }
}

void playCurrentFrequency() {
  tone(BUZZER_PIN, currentFrequency);
  digitalWrite(LED_PIN, HIGH);
  Serial.print(F("Playing: "));
  Serial.print(currentFrequency);
  Serial.println(F(" Hz"));
}

void showCurrentFrequency() {
  Serial.print(F("Current frequency: "));
  Serial.print(currentFrequency);
  Serial.println(F(" Hz"));
}
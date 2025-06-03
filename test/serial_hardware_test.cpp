// Simple hardware test to verify serial RX functionality
// This bypasses all complex logic and just echoes characters

void setup() {
  Serial.begin(115200);
  
  // Wait for serial with timeout
  unsigned long start = millis();
  while (!Serial && (millis() - start < 3000)) {
    delay(10);
  }
  
  Serial.println(F("\n=== SERIAL HARDWARE TEST ==="));
  Serial.println(F("Type any character - it should echo back immediately"));
  Serial.println(F("If you see this message but no echo, RX line is not working"));
  Serial.println(F("============================\n"));
}

void loop() {
  // Direct echo test - simplest possible serial test
  if (Serial.available()) {
    char c = Serial.read();
    Serial.print(F("Received: "));
    Serial.print(c);
    Serial.print(F(" (ASCII: "));
    Serial.print((int)c);
    Serial.println(F(")"));
  }
} 
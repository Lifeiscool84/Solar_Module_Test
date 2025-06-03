/**
 * MINIMAL SERIAL INPUT TEST for SparkFun RedBoard Artemis Nano
 * This bypasses all complex logic to test serial RX hardware directly
 * 
 * Expected behavior:
 * 1. Arduino outputs "Type any character:"
 * 2. When you type a character, it should echo back immediately
 * 3. If no echo appears, the RX hardware/driver has issues
 */

void setup() {
  // Minimal serial initialization
  Serial.begin(115200);
  
  // Wait briefly for serial to stabilize
  delay(2000);
  
  Serial.println("=== MINIMAL SERIAL RX TEST ===");
  Serial.println("Type any character - it should echo immediately:");
  Serial.println("If nothing echoes, RX hardware is broken.");
  Serial.println("===============================");
}

void loop() {
  // Most basic serial input test possible
  if (Serial.available()) {
    int incomingByte = Serial.read();
    
    // Echo the character immediately
    Serial.print("Received: '");
    Serial.print((char)incomingByte);
    Serial.print("' (ASCII: ");
    Serial.print(incomingByte);
    Serial.println(")");
    
    // Also blink built-in LED to show something was received
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
  }
  
  // Small delay to prevent flooding
  delay(10);
} 
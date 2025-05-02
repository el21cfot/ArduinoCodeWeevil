int counter = 0;  // Start at 0

void setup() {
    Serial.begin(115200);
    Serial.println("Counter test started...");
}

void loop() {
    Serial.print("Count: ");
    Serial.println(counter);
    counter++;  // Increment counter
    delay(1000);  // Wait 1 second
}

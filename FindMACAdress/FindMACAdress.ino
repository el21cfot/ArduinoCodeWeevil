#include <WiFi.h>


void setup() {
  Serial.begin(115200);
  delay(2000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(); // This ensures Wi-Fi initializes correctly, even without connecting to a network

  delay(1000); // Give Wi-Fi stack time to initialize
  
  String macAddress = WiFi.macAddress();
  Serial.print("Wi-Fi MAC Address (STA): ");
  Serial.println(macAddress);
  Serial.println(WiFi.macAddress());
}
void loop() {
  // Nothing needed here
}

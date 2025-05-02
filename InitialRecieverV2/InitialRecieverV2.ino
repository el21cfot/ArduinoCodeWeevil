#include <WiFi.h>
#include <esp_now.h>

void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len);

// Structure to store received data
typedef struct struct_message {
  uint8_t buttons[5];
  int X;
  int Y;
  int counter;
} struct_message;

struct_message receivedData;

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow time for Serial Monitor to initialize

  WiFi.mode(WIFI_STA); // Set ESP32 to station mode
  WiFi.disconnect();    // Disconnect from any previous Wi-Fi connection

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register receive callback function
  esp_now_register_recv_cb(onReceive);

  Serial.println("Receiver ready to receive messages!");
}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  Serial.println("Received Data:");
  Serial.print("Buttons: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(receivedData.buttons[i]);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("X: ");
  Serial.println(receivedData.X);

  Serial.print("Y: ");
  Serial.println(receivedData.Y);

  Serial.print("Counter: ");
  Serial.println(receivedData.counter);

  Serial.println("--------------------");
}

void loop() {
  // Nothing needed in loop, everything is handled in the callback
}

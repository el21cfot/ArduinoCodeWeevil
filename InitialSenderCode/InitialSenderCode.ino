#include <WiFi.h>
#include <esp_now.h>

// Replace with the MAC address of the receiver
uint8_t receiverMAC[] = {0xDC, 0x54, 0x75, 0xCD, 0xAC, 0x74}; 

// Structure for the data to send
typedef struct struct_message {
  uint8_t buttons[5];
    int X;
    int Y;
    int counter;
} struct_message;

struct_message dataToSend;
int counter = 0;

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

  // Register peer (Receiver)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Sender ready to send messages!");
}

void loop() {
  // Prepare message
  strcpy(dataToSend.message, "Hi boys");
  dataToSend.counter = counter++;

  // Send message
  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&dataToSend, sizeof(dataToSend));
  if (result == ESP_OK) {
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Error sending message");
  }

  delay(2000); // Send every 2 seconds
}

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

void DataSent(){
  // Set button values (for debugging, you can change these as needed)
  dataToSend.buttons[0] = 1;
  dataToSend.buttons[1] = 0;
  dataToSend.buttons[2] = 1;
  dataToSend.buttons[3] = 0;
  dataToSend.buttons[4] = 1;

  // Set fixed X and Y values for debugging
  dataToSend.X = random(0, 1024);  // Random value between 0 and 1023 (analog range)
  dataToSend.Y = random(0, 1024);  // Random value between 0 and 1023

  // Increment counter
  dataToSend.counter = counter++;
}

void loop() {
  //Call Function
  DataSent();

  // Send message
  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&dataToSend, sizeof(dataToSend));
  if (result == ESP_OK) {
    Serial.println("Message sent successfully");
    Serial.print("Buttons: ");
    for (int i = 0; i < 5; i++) {
      Serial.print(dataToSend.buttons[i]);
      Serial.print(" ");
    }
    Serial.println();

    Serial.print("X: ");
    Serial.println(dataToSend.X);

    Serial.print("Y: ");
    Serial.println(dataToSend.Y);

    Serial.print("Counter: ");
    Serial.println(dataToSend.counter);

    Serial.println("--------------------");
  } else {
    Serial.println("Error sending message");
  }

  delay(2000); // Send every 2 seconds
}

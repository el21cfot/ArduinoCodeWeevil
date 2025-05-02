#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Structure matching the sender
typedef struct struct_message {
    uint8_t buttons[5];
    int X;
    int Y;
    int counter;
} struct_message;

struct_message receivedData;
int waitingCounter = 0;  // Counter to indicate it's running

// Callback function for receiving data
void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    Serial.println("\nESP-NOW Data Received!");

    // Copy received data into the struct
    memcpy(&receivedData, incomingData, sizeof(receivedData));

    // Print sender's MAC address
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);
    Serial.print("Message received from: ");
    Serial.println(macStr);

    // Print received values
    Serial.print("Counter: ");
    Serial.println(receivedData.counter);
    
    Serial.print("X: ");
    Serial.println(receivedData.X);
    
    Serial.print("Y: ");
    Serial.println(receivedData.Y);
    
    Serial.print("Button States: ");
    for (int i = 0; i < 5; i++) {
        Serial.print(receivedData.buttons[i]);
        Serial.print(" ");
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Receiver starting...");

    WiFi.mode(WIFI_STA); // Set ESP32 to station mode
    WiFi.begin(); // Initialize Wi-Fi stack

    delay(1000); // Allow time for Wi-Fi to initialize
    
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // Set Wi-Fi channel

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register the receive callback
    esp_now_register_recv_cb(onReceive);
    
    Serial.println("Receiver ready!");
}

void loop() {
    waitingCounter++;  
    Serial.print("Waiting for message... Count: ");
    Serial.println(waitingCounter);
    delay(1000);  // Print message every second
}

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "Adafruit_seesaw.h"

uint8_t broadcastAddress[] = {0xDC, 0x54, 0x75, 0xCD, 0xB0, 0x18}; // Broadcast MAC address
int counter = 0;
int last_x = 0, last_y = 0;

//Featherwing stuff
  //motor control values
int selectedMotor = 1; // 1 = Forward 1, 2 = Backward 2, 3 = Right, 3 = Left

//button mapping
#define BUTTON_RIGHT 6
#define BUTTON_DOWN  7
#define BUTTON_LEFT  9
#define BUTTON_UP    10
#define BUTTON_SEL   14
uint32_t button_mask = (1 << BUTTON_RIGHT) | (1 << BUTTON_DOWN) | 
                (1 << BUTTON_LEFT) | (1 << BUTTON_UP) | (1 << BUTTON_SEL);

// Structure for the data to send
typedef struct struct_message {
  char message[10];
  int counter;
} struct_message;

struct_message dataToSend;

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA); // Initialize Wi-Fi in station mode
  WiFi.begin(); // Required to initialize the Wi-Fi stack

  delay(1000); // Allow time for Wi-Fi to initialize
  
  // Set Wi-Fi channel to 1 (or any other channel, 1–13)
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // Continue with ESP-NOW initialization
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Sender ready!");

  //this is where i start adding the joy in, potential errors
  if(!joy.begin(0x49)){
    Serial.println("ERROR! joy not found");
    while(1) delay(1);
  } else {
    Serial.println("connection started");
    Serial.print("version: ");
    Serial.println(joy.getVersion(), HEX);
  }
  joy.pinModeBulk(button_mask, INPUT_PULLUP);
  joy.setGPIOInterrupts(button_mask, 1);

  pinMode(IRQ_PIN, INPUT);
}

void ButtonControl(){
  int x = joy.analogRead(2);
  int y = joy.analogRead(3);
  
  if ( (abs(x - last_x) > 3)  ||  (abs(y - last_y) > 3)) {
    Serial.print(x); Serial.print(", "); Serial.println(y);
    last_x = x;
    last_y = y;
  }
  
  /* if(!digitalRead(IRQ_PIN)) {  // Uncomment to use IRQ */

    uint32_t buttons = joy.digitalReadBulk(button_mask);

    //Serial.println(buttons, BIN);

    if (! (buttons & (1 << BUTTON_RIGHT))) {
      Serial.println("Button A pressed");
      //NeoPixelRed();
      //right();
    }
    if (! (buttons & (1 << BUTTON_DOWN))) {
      Serial.println("Button B pressed");
      //NeoPixelGreen();
      //backward();
    }
    if (! (buttons & (1 << BUTTON_LEFT))) {
      Serial.println("Button Y pressed");
      //NeoPixelBlue();
      //left();
    }
    if (! (buttons & (1 << BUTTON_UP))) {
      Serial.println("Button X pressed");
      //NeoPixelPurple();
      //forward();
    }
    if (! (buttons & (1 << BUTTON_SEL))) {
      Serial.println("Button SEL pressed");
    }
  /* } // Uncomment to use IRQ */
  delay(10);
}

void loop() {
  // Prepare message
  strcpy(dataToSend.message, "Hi boys");
  dataToSend.counter = counter++;

  // Send message
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&dataToSend, sizeof(dataToSend));
  if (result == ESP_OK) {
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Error sending message");
  }

  delay(2000); // Send every 2 seconds
}

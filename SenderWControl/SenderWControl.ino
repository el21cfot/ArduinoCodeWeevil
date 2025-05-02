#include <WiFi.h>
#include <esp_now.h>
#include "Adafruit_seesaw.h"

//JoyStick Stuff
Adafruit_seesaw joy;
// Button mapping
#define BUTTON_RIGHT 6
#define BUTTON_DOWN  7
#define BUTTON_LEFT  9
#define BUTTON_UP    10
#define BUTTON_SEL   14

//IRcue stuff
#if defined(ESP8266)
  #define IRQ_PIN   2
#elif defined(ESP32) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
  #define IRQ_PIN   14
#elif defined(ARDUINO_NRF52832_FEATHER)
  #define IRQ_PIN   27
#elif defined(TEENSYDUINO)
  #define IRQ_PIN   8
#elif defined(ARDUINO_ARCH_WICED)
  #define IRQ_PIN   PC5
#else
  #define IRQ_PIN   5
#endif

uint32_t button_mask = (1 << BUTTON_RIGHT) | (1 << BUTTON_DOWN) | 
                       (1 << BUTTON_LEFT) | (1 << BUTTON_UP) | (1 << BUTTON_SEL);
uint8_t button_status[5] = {0, 0, 0, 0, 0}; 

// MAC address of the receiver
uint8_t receiverMAC[] = {0xDC, 0x54, 0x75, 0xC1, 0x05, 0x64}; 

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

  // Initialize joystick
  if(!joy.begin(0x49)){
      Serial.println("ERROR! joystick not found");
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

void DataSent(){
  // Set button values as zero to make sure no previous presses carrying
  button_status[0] = 0;
  button_status[1] = 0;
  button_status[2] = 0;
  button_status[3] = 0;
  button_status[4] = 0;

  //create button object
  uint32_t buttons = joy.digitalReadBulk(button_mask);
  //set button status
  if (!(buttons & (1 << BUTTON_RIGHT))) {
        //Serial.println("Button A pressed");
        button_status[0] = 1;
  } else {
      button_status[0] = 0;
  }

  if (!(buttons & (1 << BUTTON_DOWN))) {
      //Serial.println("Button B pressed");
      button_status[1] = 1;
  } else {
      button_status[1] = 0;
  }

  if (!(buttons & (1 << BUTTON_LEFT))) {
      //Serial.println("Button Y pressed");
      button_status[2] = 1;
  } else {
      button_status[2] = 0;
  }

  if (!(buttons & (1 << BUTTON_UP))) {
      //Serial.println("Button X pressed");
      button_status[3] = 1;
  } else {
      button_status[3] = 0;
  }

  if (!(buttons & (1 << BUTTON_SEL))) {
      //Serial.println("Button SEL pressed");
      button_status[4] = 1;
  } else {
      button_status[4] = 0;
  }
  //Set button message to values of buttons
  for (int i = 0; i<5; i++){
    dataToSend.buttons[i] = button_status[i];
  }

  // Set fixed X and Y values for debugging
  dataToSend.X = 0;
  dataToSend.Y = 0;

  int x = joy.analogRead(3);
  int y = joy.analogRead(2);
  //send values to other board
  dataToSend.X = x;
  dataToSend.Y = y;


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
  delay(10);
  //delay(2000); // Send every 2 seconds
}

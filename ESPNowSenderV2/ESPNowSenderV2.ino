#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "Adafruit_seesaw.h"

Adafruit_seesaw joy;

uint8_t broadcastAddress[] = {0xDC, 0x54, 0x75, 0xCD, 0xB0, 0x18}; // Broadcast MAC address
int counter = 0;
int last_x = 0, last_y = 0;

// Featherwing stuff
int selectedMotor = 1; // 1 = Forward, 2 = Backward, 3 = Right, 4 = Left

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
uint8_t button_status[5] = {0, 0, 0, 0, 0}; // Example button status array

// Structure for the data to send
typedef struct struct_message {
    uint8_t buttons[5];
    int X;
    int Y;
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

void ButtonControl(){
    int x = joy.analogRead(3);
    int y = joy.analogRead(2);
    
    if ((abs(x - last_x) > 3) || (abs(y - last_y) > 3)) {
        //.print(x); Serial.print(", "); Serial.println(y);
        last_x = x;
        last_y = y;
    }

    uint32_t buttons = joy.digitalReadBulk(button_mask);

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
    
    delay(10);
}

void loop() {
    ButtonControl(); // Update button statuses and joystick values
    // Prepare message
    memcpy(dataToSend.buttons, button_status, sizeof(button_status));
    dataToSend.X = last_x;
    dataToSend.Y = last_y;
    dataToSend.counter = counter++;

    // Send message
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&dataToSend, sizeof(dataToSend));
    if (result == ESP_OK) {
        Serial.println("Message sent successfully");
        for(int i = 0; i < 5; i++)
        {
          Serial.println(dataToSend.buttons[i]);
        }
        Serial.print(dataToSend.X);
        Serial.print(dataToSend.Y);
        Serial.print(dataToSend.counter);
    } else {
        Serial.println("Error sending message");
    }

    delay(2000); // Send every 2 seconds
}

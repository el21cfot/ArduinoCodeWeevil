#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Adafruit_NeoPixel.h>

// Initial X and Y values
int last_x = 0, last_y = 0;

// Define pins for motors out
#define MotorA1 6      
#define MotorA2 5 
#define MotorB1 12
#define MotorB2 11
#define MotorDriverOn 9
#define MotorDriverFlag 10

// NeoPixel Stuff
#define LED_PIN PIN_NEOPIXEL // Built-in NeoPixel is typically on pin 8
#define NUM_LEDS 1 // Only one NeoPixel for built-
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
#define LED_PIN2 13

// Button stuff             
uint8_t button_status[5] = {0, 0, 0, 0, 0}; // Example button status array
int current_x = 0, current_y = 0;

void NeoPixelGreen() {
  strip.setPixelColor(0, strip.Color(0, 255, 0)); // Set to green
  strip.setBrightness(50); // Brightness level (0-255)
  strip.show(); // Display the color
}

void NeoPixelBlue() {
  strip.setPixelColor(0, strip.Color(0, 0, 255)); // Set to blue
  strip.setBrightness(50); // Brightness level (0-255)
  strip.show(); // Display the color
}

void NeoPixelRed() {
  strip.setPixelColor(0, strip.Color(255, 0, 0)); // Set to red
  strip.setBrightness(50); // Brightness level (0-255)
  strip.show(); // Display the color
}

void NeoPixelPurple() {
  strip.setPixelColor(0, strip.Color(128, 0, 128)); // Set to purple
  strip.setBrightness(50); // Brightness level (0-255)
  strip.show(); // Display the color
}

// Structure for received data
typedef struct struct_message {
  uint8_t buttons[5];
  int X;
  int Y;
  int counter;
} struct_message;

struct_message receivedData;

// Callback function for receiving data
void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  Serial.println("ESP-NOW Data Received!");
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  
  // Print sender's MAC address
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           info->src_addr[0], info->src_addr[1], info->src_addr[2],
           info->src_addr[3], info->src_addr[4], info->src_addr[5]);
  Serial.print("Message received from: ");
  Serial.println(macStr);

  // Print counter
  Serial.print("Counter: ");
  Serial.println(receivedData.counter);
  NeoPixelRed();
  memcpy(button_status, receivedData.buttons, sizeof(receivedData.buttons));
  current_x = receivedData.X;
  current_y = receivedData.Y;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Serial working");
  // Set up WiFi in STA mode
  WiFi.mode(WIFI_STA); // Initialize Wi-Fi in station mode
  WiFi.begin(); // Required to initialize the Wi-Fi stack

  delay(1000); // Allow time for Wi-Fi to initialize
  
  // Set Wi-Fi channel to 1 (or any other channel, 1–13)
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    NeoPixelBlue();
    return;
  }

  // Register the receive callback
  NeoPixelRed();
  esp_now_register_recv_cb(onReceive);
  Serial.println("Receiver ready!");
  //NeoPixelGreen();

  // Motor setup
  pinMode(MotorA1, OUTPUT);
  pinMode(MotorA2, OUTPUT);
  pinMode(MotorDriverOn, OUTPUT);
  pinMode(MotorDriverFlag, INPUT_PULLUP); // Configure pin as input with internal pull-up resistor

  // NeoPixel setup
  pinMode(LED_PIN, OUTPUT);
  strip.begin(); // Initialize the NeoPixel
  strip.show(); // Ensure it's off at the start

  // Extra pins
  pinMode(LED_PIN2, OUTPUT); // Set the pin as an output
}

void ButtonControl(){
  int x = current_x;
  int y = current_y;
  
  if ((abs(x - last_x) > 3) || (abs(y - last_y) > 3)) {
    Serial.print(x); Serial.print(", "); Serial.println(y);
    last_x = x;
    last_y = y;
  }

  if (button_status[0] == 1) {
    Serial.println("Button A pressed");
    NeoPixelRed();
    right();
  }
  if (button_status[1] == 1) {
    Serial.println("Button B pressed");
    NeoPixelGreen();
    backward();
  }
  if (button_status[2] == 1) {
    Serial.println("Button Y pressed");
    NeoPixelBlue();
    left();
  }
  if (button_status[3] == 1) {
    Serial.println("Button X pressed");
    NeoPixelPurple();
    forward();
  }
  if (button_status[4] == 1) {
    Serial.println("Button SEL pressed");
  }
  
  delay(10);

}

void forward() {
  digitalWrite(MotorA1, HIGH);  // Set PWM value
  digitalWrite(MotorA2, LOW);  // Set direction
  digitalWrite(MotorB1, HIGH);  // Set PWM value
  digitalWrite(MotorB2, LOW);  // Set direction
  digitalWrite(MotorDriverOn, HIGH);
}

void backward() {
  digitalWrite(MotorA1, LOW);  // Set PWM value
  digitalWrite(MotorA2, HIGH);  // Set direction
  digitalWrite(MotorB1, LOW);  // Set PWM value
  digitalWrite(MotorB2, HIGH);  // Set direction
  digitalWrite(MotorDriverOn, HIGH);
}

void right() {
  digitalWrite(MotorA1, HIGH);  // Set PWM value
  digitalWrite(MotorA2, LOW);  // Set direction
  digitalWrite(MotorB1, LOW);  // Set PWM value
  digitalWrite(MotorB2, HIGH);  // Set direction
  digitalWrite(MotorDriverOn, HIGH);
}

void left() {
  digitalWrite(MotorA1, HIGH);  // Set PWM value
  digitalWrite(MotorA2, LOW);  // Set direction
  digitalWrite(MotorB1, HIGH);  // Set PWM value
  digitalWrite(MotorB2, LOW);  // Set direction
  digitalWrite(MotorDriverOn, HIGH);
}

void loop() {
  // Nothing to do here, as data reception is handled by the callback
  //ButtonControl();
}

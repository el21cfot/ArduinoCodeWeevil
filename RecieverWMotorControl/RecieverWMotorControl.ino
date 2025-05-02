#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>

void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len);

//Latest Recieved values
int RecievedX = 0;
int RecievedY = 0;
int RecievedCounter = 0;
uint8_t RecievedButtons[5] = {0, 0, 0, 0, 0};

// Structure to store received data
typedef struct struct_message {
  uint8_t buttons[5];
  int X;
  int Y;
  int counter;
} struct_message;

struct_message receivedData;

// NeoPixel Stuff
#define LED_PIN PIN_NEOPIXEL // Built-in NeoPixel is typically on pin 8
#define NUM_LEDS 1 // Only one NeoPixel for built-
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
#define LED_PIN2 13

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

//Motor stuff
// Define pins for motors out
#define MotorA1 6      
#define MotorA2 5 
#define MotorB1 12
#define MotorB2 11
#define MotorDriverOn 9
#define MotorDriverFlag 10

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
  
  // Motor setup
  pinMode(MotorA1, OUTPUT);
  pinMode(MotorA2, OUTPUT);
  pinMode(MotorDriverOn, OUTPUT);
  pinMode(MotorDriverFlag, INPUT_PULLUP); // Configure pin as input with internal pull-up resistor

  // NeoPixel setup
  pinMode(LED_PIN, OUTPUT);
  strip.begin(); // Initialize the NeoPixel
  strip.show(); // Ensure it's off at the start
}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  //Recieved data printing to serial for debugging
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

  //Setting recieved values to correct variables
  RecievedX = receivedData.X;
  RecievedY = receivedData.Y;
  RecievedCounter = receivedData.counter;
  
  for (int i = 0; i < 5; i++){
    RecievedButtons[i] = receivedData.buttons[i];
  }
  
}

//movement functions
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

void MotorControl(){
  while(RecievedButtons[0]==1){
    //Buttons A
    NeoPixelRed();
  }
  while(RecievedButtons[1]==1){
    //Buttons B
    NeoPixelGreen();
    backward();
  }
  while(RecievedButtons[2]==1){
    //Buttons Y
    NeoPixelPurple();
  }
  while(RecievedButtons[3]==1){
    //Buttons X
    NeoPixelBlue();
    forward();
  }
}

void loop() {
  // Nothing needed in loop, everything is handled in the callback
  MotorControl();
}

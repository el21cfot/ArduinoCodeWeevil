#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PWMServoDriver.h>

//constants
float decayFactor = 0.8;
float baseAmplitude = 45.0;
float timeStep = 0.05;
float TimeVar = 0;
int NumServos = 4;


void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len);

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

// Latest Received values
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

// Servo Driver Setup
Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40);
#define SERVOMIN  125
#define SERVOMAX  625

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(onReceive);

  Serial.println("Receiver ready to receive messages!");

  // NeoPixel setup
  pinMode(LED_PIN, OUTPUT);
  strip.begin();
  strip.show();

  // Servo driver setup
  board1.begin();
  board1.setPWMFreq(60);
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

int angleToPulse(int ang) {
  return map(ang, 0, 180, SERVOMIN, SERVOMAX);
}

void moveServos(bool forward) {
  int phaseShift = 90;
  for (int angle = 0; angle <= 180; angle += 10) {
    for (int i = 0; i < 4; i++) {
      int adjustedAngleLeft = forward ? angle : 180 - angle;
      int adjustedAngleRight = forward ? (angle + phaseShift) % 180 : 180 - ((angle + phaseShift) % 180);
      board1.setPWM(i, 0, angleToPulse(adjustedAngleLeft));
      board1.setPWM(i + 4, 0, angleToPulse(adjustedAngleRight));
    }
    delay(100);
  }
}

void forward() {

  for(int i = 0; i < NumServos; i++){
    float phaseShift = i * 0.5;
    float amplitude = baseAmplitude * pow(decayFactor, i);
    float angle = 90 + amplitude * sin(TimeVar + phaseShift);
    board1.setPWM(i, 0, angleToPulse(angle));
    board1.setPWM(i + 4, 0, angleToPulse(angle));
  }
  TimeVar += timeStep;
}

void backward() {
  for(int i = 0; i < NumServos; i++){
    float phaseShift = i * 0.5;
    float amplitude = baseAmplitude * pow(decayFactor, i);
    float angle = 90 + amplitude * sin(TimeVar - phaseShift);
    board1.setPWM(i, 0, angleToPulse(angle));
    board1.setPWM(i + 4, 0, angleToPulse(angle));
  }
  TimeVar += timeStep;
}

void MotorControl() {
  if (RecievedButtons[0] == 1) {
    NeoPixelGreen();
    //right();
    LeftSweepServos();
    //sweepServos();
  }
  if (RecievedButtons[1] == 1) {
    NeoPixelRed();
    //backward();
    RevSweepServos();
  }
  if (RecievedButtons[2] == 1) {
    NeoPixelBlue();
    //left();
    RightSweepServos();
    //centerServos();
  }
  if (RecievedButtons[3] == 1) {
    NeoPixelBlue();
    //forward();
    sweepServos();
  }
  if (RecievedButtons[4] == 1) {
    NeoPixelPurple();
    //forward();
    //centerServos();
    centerServos();
  }
}

void centerServos() {
  for (int i = 0; i < 8; i++) { // 4 servos per side (8 total)
    board1.setPWM(i, 0, angleToPulse(90)); // Move to center position
  }
}

void sweepServos() {
  int sweepAngle = 45;   // Maximum sweep range from center (90 ± 45)
  int delayTime = 50;    // Delay between updates
  float phaseShift = PI / 4;  // Phase shift between each servo in radians

  for (float t = 0; t < 2 * PI; t += 0.1) {  // Full cycle for smooth motion
    for (int i = 0; i < 4; i++) { 
      // Compute angle with phase shift
      float leftAngle = 90 + sweepAngle * sin(t - (i * phaseShift));  
      float rightAngle = 90 - sweepAngle * sin(t - (i * phaseShift)); // Mirror motion

      // Set servo positions
      board1.setPWM(i, 0, angleToPulse(leftAngle));
      board1.setPWM(i + 4, 0, angleToPulse(rightAngle));
    }
    delay(delayTime);
  }
}

void RevSweepServos() {
  int sweepAngle = 45;   // Maximum sweep range from center (90 ± 45)
  int delayTime = 50;    // Delay between updates
  float phaseShift = PI / 4;  // Phase shift between each servo in radians

  for (float t = 0; t < 2 * PI; t += 0.1) {  // Full cycle for smooth motion
    for (int i = 0; i < 4; i++) { 
      // Compute angle with phase shift
      float leftAngle = 90 - sweepAngle * sin(t - (i * phaseShift));  
      float rightAngle = 90 + sweepAngle * sin(t - (i * phaseShift)); // Mirror motion

      // Set servo positions
      board1.setPWM(i, 0, angleToPulse(rightAngle));
      board1.setPWM(i + 4, 0, angleToPulse(leftAngle));
    }
    delay(delayTime);
  }
}

void LeftSweepServos() {
  int sweepAngle = 45;   // Maximum sweep range from center (90 ± 45)
  int delayTime = 50;    // Delay between updates
  float phaseShift = PI / 4;  // Phase shift between each servo in radians

  for (float t = 0; t < 2 * PI; t += 0.1) {  // Full cycle for smooth motion
    for (int i = 0; i < 4; i++) { 
      // Compute angle with phase shift
      float leftAngle = 90 + sweepAngle * sin(t - (i * phaseShift));  
      // Set servo positions
      board1.setPWM(i, 0, angleToPulse(leftAngle));
    }
    delay(delayTime);
  }
}

void RightSweepServos() {
  int sweepAngle = 45;   // Maximum sweep range from center (90 ± 45)
  int delayTime = 50;    // Delay between updates
  float phaseShift = PI / 4;  // Phase shift between each servo in radians

  for (float t = 0; t < 2 * PI; t += 0.1) {  // Full cycle for smooth motion
    for (int i = 0; i < 4; i++) { 
      // Compute angle with phase shift
      float rightAngle = 90 + sweepAngle * sin(t - (i * phaseShift));  
      // Set servo positions
      board1.setPWM(i+4, 0, angleToPulse(rightAngle));
    }
    delay(delayTime);
  }
}



void loop() {
  MotorControl();
}

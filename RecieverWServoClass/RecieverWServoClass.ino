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

//SEt up pin on boost convertor
#define Boost_PIN 9


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

// Servo Driver Setup
Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40);
#define SERVOMIN  125
#define SERVOMAX  625

//Grabber open close logic
bool grabberOpen      = false;
bool lastButton4State = false;
// motor pins (DRV8833)
const int motorPin1 = 5;
const int motorPin2 = 6;
//Servo constants
int openAngle  = 180;
int closeAngle = 20;
int currentAngle;


void clkwsSpin() {
  int d = 10;
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  delay(d);
  // digitalWrite(motorPin1, LOW);
  // digitalWrite(motorPin2, LOW);
  // delay(500);
}

void antclkwsSpin() {
  int d = 10;
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin1, LOW);
  delay(d);
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  delay(500);
}

void motorSpin() {
  int d = 250;
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  delay(d);
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  delay(500);
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  delay(d);
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
}

// maps 0–180° → SERVOMIN–SERVOMAX and writes to PCA9685
void setServoAngleJay(int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  board1.setPWM(9, 0, pulse);
  currentAngle = angle;
}

void servoOpen() {
  setServoAngleJay(openAngle);
}

void servoClose() {
  setServoAngleJay(closeAngle);
}

//Class for servos
class ServoGroup {
private:
    Adafruit_PWMServoDriver* pwm;
    int servoIDs[4]; // Store IDs of the 4 servos in the group
    float baseAmplitude;
    float decayFactor;
    float timeVar;
    float timeStep;
    float phaseShift;

public:
    ServoGroup(Adafruit_PWMServoDriver* pwmDriver, int ids[4], float amplitude = 45.0, float decay = 0.8, float step = 0.05, float shift = 0.5)
        : pwm(pwmDriver), baseAmplitude(amplitude), decayFactor(decay), timeVar(0), timeStep(step), phaseShift(shift) {
        for (int i = 0; i < 4; i++) {
            servoIDs[i] = ids[i];
        }
    }

    int angleToPulse(int ang) {
        return map(ang, 0, 180, SERVOMIN, SERVOMAX);
    }

    void moveServos(float directionMultiplier) {
        for (int i = 0; i < 4; i++) {
            float amplitude = baseAmplitude * pow(decayFactor, i);
            float angle = 90 + directionMultiplier * amplitude * sin(timeVar + (i * phaseShift));
            pwm->setPWM(servoIDs[i], 0, angleToPulse(angle));
        }
        timeVar += timeStep;
    }

    void centerServos() {
        for (int i = 0; i < 4; i++) {
            pwm->setPWM(servoIDs[i], 0, angleToPulse(90)); // Move to center
        }
    }
};

struct_message receivedData;

int leftServos[] = {0, 1, 2, 3};  // PWM channels for left servos
int rightServos[] = {4, 5, 6, 7}; // PWM channels for right servos

ServoGroup leftGroup(&board1, leftServos);
ServoGroup rightGroup(&board1, rightServos);

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

  //Boost setup
  pinMode(Boost_PIN, OUTPUT); // Configure LED_PIN2 as an output
  digitalWrite(Boost_PIN, HIGH); // Set LED_PIN2 to HIGH

  // --- motor driver pins
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);

  //ServoGrabberSetup
  currentAngle = openAngle;
  setServoAngleJay(currentAngle);
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

void ManualControl() {
  while (RecievedButtons[0] == 1) {
        leftGroup.moveServos(1);  // Move left side forward
        rightGroup.moveServos(1); // Move right side forward
        NeoPixelGreen();
    }
  while (RecievedButtons[1] == 1) {
      leftGroup.moveServos(-1);  // Move left side backward
      rightGroup.moveServos(-1); // Move right side backward
      NeoPixelRed();
  }
  while (RecievedButtons[2] == 1) {
      leftGroup.moveServos(1);   // Left side moves forward
      rightGroup.moveServos(-1); // Right side moves backward (turn left)
      NeoPixelBlue();
  }
  while (RecievedButtons[3] == 1) {
      leftGroup.moveServos(-1);  // Left side moves backward
      rightGroup.moveServos(1);  // Right side moves forward (turn right)
      NeoPixelPurple();
  }
  bool btn4 = (RecievedButtons[4] == 1);
  if(btn4 && !lastButton4State) {
    motorSpin();
  }
  lastButton4State = btn4;
  //Center servos
  leftGroup.centerServos();
  rightGroup.centerServos();
}



void SpinControl() {
  // if Y low and grabber still open → close it
  if (RecievedY <= 400 && grabberOpen) {
    int pulse = map(closeAngle, 0, 180, SERVOMIN, SERVOMAX);
    board1.setPWM(9, 0, pulse);
    grabberOpen = false;
    Serial.println("SpinControl → grabber CLOSED");
  }
  // else if Y high and grabber still closed → open it
  else if (RecievedY >= 600 && !grabberOpen) {
    int pulse = map(openAngle, 0, 180, SERVOMIN, SERVOMAX);
    board1.setPWM(9, 0, pulse);
    grabberOpen = true;
    Serial.println("SpinControl → grabber OPENED");
  }
}



void loop() {
  ManualControl();
  SpinControl();
}

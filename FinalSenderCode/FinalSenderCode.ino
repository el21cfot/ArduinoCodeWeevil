#include <WiFi.h>
#include <esp_now.h>
#include "Adafruit_seesaw.h"

// Joystick (Seesaw) definitions
Adafruit_seesaw joy;
#define BUTTON_RIGHT 6
#define BUTTON_DOWN  7
#define BUTTON_LEFT  9
#define BUTTON_UP    10
#define BUTTON_SEL   14

// IRQ pin for Seesaw
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

uint32_t button_mask = (1 << BUTTON_RIGHT) | (1 << BUTTON_DOWN) \
                       | (1 << BUTTON_LEFT) | (1 << BUTTON_UP)  \
                       | (1 << BUTTON_SEL);
uint8_t button_status[5] = {0, 0, 0, 0, 0};

// Receiver MAC address
uint8_t receiverMAC[] = {0xDC, 0x54, 0x75, 0xC1, 0x05, 0x64};

// Structure matching the receiver's definition
typedef struct struct_message {
  bool    manualMode;
  bool    returnHome;
  double  lat;
  double  lon;
  uint8_t buttons[5];
  int     X;
  int     Y;
  int     counter;
} struct_message;

// Data buffer to send
struct_message dataToSend;
int counter = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Wi-Fi and ESP-NOW init
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register peer (receiver)
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
  if (!joy.begin(0x49)) {
    Serial.println("ERROR! joystick not found");
    while (1) delay(1);
  }
  joy.pinModeBulk(button_mask, INPUT_PULLUP);
  joy.setGPIOInterrupts(button_mask, 1);
  pinMode(IRQ_PIN, INPUT);
}

// Reads joystick buttons and axes, updates dataToSend.buttons, X, Y, counter
void prepareMessage() {
  uint32_t buttons = joy.digitalReadBulk(button_mask);
  button_status[0] = !(buttons & (1 << BUTTON_RIGHT));
  button_status[1] = !(buttons & (1 << BUTTON_DOWN));
  button_status[2] = !(buttons & (1 << BUTTON_LEFT));
  button_status[3] = !(buttons & (1 << BUTTON_UP));
  button_status[4] = !(buttons & (1 << BUTTON_SEL));
  memcpy(dataToSend.buttons, button_status, sizeof(button_status));

  dataToSend.X = joy.analogRead(3);
  dataToSend.Y = joy.analogRead(2);

  dataToSend.counter = counter++;
}

// Sends dataToSend via ESP-NOW
void sendMessage() {
  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&dataToSend, sizeof(dataToSend));
  if (result == ESP_OK) {
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Error sending message");
  }
}

void loop() {
  // Prompt for control mode
  Serial.println("Enter command: m = Manual, a = Autonomous, r = Return Home, s = Set Target, x = Reset");
  while (!Serial.available()) {
    delay(10);
  }
  char cmd = Serial.read();

  switch (cmd) {
    case 'm':
      // Switch to manual mode
      dataToSend.manualMode = true;
      dataToSend.returnHome = false;
      prepareMessage();
      sendMessage();
      break;

    case 'a':
      // Switch to autonomous live target
      dataToSend.manualMode = false;
      dataToSend.returnHome = false;
      prepareMessage();
      sendMessage();
      break;

    case 'r':
      // Initiate return-home
      dataToSend.manualMode = false;
      dataToSend.returnHome = true;
      prepareMessage();
      sendMessage();
      break;

    case 's':
      // Prompt user for new target coordinates
      dataToSend.manualMode = false;
      dataToSend.returnHome = false;
      Serial.println("Enter target latitude:");
      while (!Serial.available()) delay(10);
      dataToSend.lat = Serial.parseFloat();
      Serial.read(); // consume newline

      Serial.println("Enter target longitude:");
      while (!Serial.available()) delay(10);
      dataToSend.lon = Serial.parseFloat();
      Serial.read(); // consume newline

      prepareMessage();
      sendMessage();
      break;

    case 'x':
      // Reset message to default values
      dataToSend.manualMode = true;
      dataToSend.returnHome = false;
      dataToSend.lat = 0.0;
      dataToSend.lon = 0.0;
      memset(dataToSend.buttons, 0, sizeof(dataToSend.buttons));
      dataToSend.X = 0;
      dataToSend.Y = 0;
      dataToSend.counter = 0;
      Serial.println("Message reset to standard defaults");
      prepareMessage();
      sendMessage();
      break;

    default:
      Serial.println("Invalid command");
      break;
  }

  // Small pause before next prompt
  delay(10);
}

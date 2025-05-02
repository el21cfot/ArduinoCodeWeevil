#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_MAX1704X.h>
#include <cmath>
#include <Adafruit_GPS.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MMC56x3.h>

// ==================== CONSTANTS & CONFIGURATION ====================
#define GPS_RX       4       // GPS TX -> ESP32 RX
#define GPS_TX      -1       // Unused GPS TX pin
#define GPS_BAUD  9600       // GPS baud rate
#define BOOST_PIN    9       // Boost converter enable pin
#define NUM_LEDS     1       // Only one NeoPixel used
#define LED_PIN      PIN_NEOPIXEL
#define SERVOMIN    125      // PWM pulse for 0°
#define SERVOMAX    625      // PWM pulse for 180°
constexpr double ALIGN_THRESHOLD   = 15.0;    // Degrees tolerance for "on target"
constexpr unsigned long FAST_INTERVAL = 1000; // ms between updates when off-course
constexpr unsigned long SLOW_INTERVAL = 5000; // ms between updates when aligned
constexpr int HISTORY_SIZE = 20;              // Buffer size for past GPS fixes

// ==================== GLOBALS ====================
HardwareSerial gpsSerial(1);
Adafruit_GPS GPS(&gpsSerial);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40);

// Circular buffer to store past latitude/longitude
struct LatLon { double lat, lon; };
LatLon history[HISTORY_SIZE];
int histIndex = 0;  // Next buffer slot to write
int histCount = 0;  // Number of stored entries (<= HISTORY_SIZE)

// Return-home state
bool returningHome = false;
int returnIndex   = -1;

// Control flags and targets
bool ManualControlBool = true;
double targetLat = 0.0;
double targetLon = 0.0;

// Received data structure over ESP-NOW
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
struct_message receivedData;

// Latest raw inputs from the controller
int RecievedX = 0;
int RecievedY = 0;
int RecievedCounter = 0;
uint8_t RecievedButtons[5] = {0};

// Pin definitions for BLDC driver (DRV8833)
const int motorPin1 = 5;
const int motorPin2 = 6;

// Grabber state
bool grabberOpen = false;
bool lastButton4State = false;
int openAngle  = 180;
int closeAngle = 20;
int currentAngle;

// ==================== FUNCTION DECLARATIONS ====================
void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len);
void startReturnHome();
void recordPos(double lat, double lon);
void motorSpin();
void moveServos(bool forward);
void ManualControl();
void ManualGrabber();
double toDecimalDegrees(double raw);
double calculateBearing(double lat1, double lon1, double lat2, double lon2);
double wrap180(double d);
double haversineDist(double lat1, double lon1, double lat2, double lon2);
void updateMovement(double currentBearing, double targetBearing);
void AutonomousControl();

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize Wi-Fi in station mode and ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(onReceive);

  Serial.println("Receiver ready to receive messages!");

  // Initialize PWM servo driver
  board1.begin();
  board1.setPWMFreq(60);

  // Initialize boost convertor control pin
  pinMode(BOOST_PIN, OUTPUT);
  digitalWrite(BOOST_PIN, HIGH);

  // Initialize motor driver pins
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);

  // Set grabber to open
  currentAngle = openAngle;

  // Initialize GPS serial and settings
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
  GPS.begin(GPS_BAUD);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
}

// ==================== ESP-NOW CALLBACK ====================
// Copies incoming data into `receivedData` and triggers return-home
void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  RecievedX = receivedData.X;
  RecievedY = receivedData.Y;
  RecievedCounter = receivedData.counter;
  for (int i = 0; i < 5; i++) RecievedButtons[i] = receivedData.buttons[i];

  // Update live target coordinates
  targetLat = receivedData.lat;
  targetLon = receivedData.lon;

  // If returnHome flag set and not already returning, initialize
  if (receivedData.returnHome && !returningHome) {
    startReturnHome();
  }
}

// ==================== HISTORY MANAGEMENT ====================
// Pushes the current position into the circular buffer
void recordPos(double lat, double lon) {
  history[histIndex] = { lat, lon };
  histIndex = (histIndex + 1) % HISTORY_SIZE;
  if (histCount < HISTORY_SIZE) histCount++;
}

// Sets up indices to walk the history in reverse
void startReturnHome() {
  if (histCount == 0) return;
  returningHome = true;
  returnIndex = (histIndex + HISTORY_SIZE - 1) % HISTORY_SIZE;
}

// ==================== MOVEMENT HELPERS ====================
// Spins the DC motor back and forth for the grabber
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

// Moves each servo in a group in a sine-wave gait
void moveServos(bool forward) {
  int phaseShift = 90;
  for (int angle = 0; angle <= 180; angle += 10) {
    for (int i = 0; i < 4; i++) {
      int leftA  = forward ? angle : 180 - angle;
      int rightA = forward ? (angle + phaseShift) % 180 : 180 - ((angle + phaseShift) % 180);
      board1.setPWM(i,     0, map(leftA,  0, 180, SERVOMIN, SERVOMAX));
      board1.setPWM(i + 4, 0, map(rightA, 0, 180, SERVOMIN, SERVOMAX));
    }
    delay(100);
  }
}

// ==================== MANUAL CONTROL LOOP ====================
// Reads controller inputs and drives servos & grabber
void ManualControl() {
  for (int i = 0; i < 5; i++) RecievedButtons[i] = receivedData.buttons[i];

  // Forward/backward/turn logic based on buttons
  while (RecievedButtons[0] == 1) { leftGroup.moveServos(1);  rightGroup.moveServos(1);  strip.Color(0,255,0); }
  while (RecievedButtons[1] == 1) { leftGroup.moveServos(-1); rightGroup.moveServos(-1); strip.Color(255,0,0); }
  while (RecievedButtons[2] == 1) { leftGroup.moveServos(1);  rightGroup.moveServos(-1); strip.Color(0,0,255); }
  while (RecievedButtons[3] == 1) { leftGroup.moveServos(-1); rightGroup.moveServos(1);  strip.Color(128,0,128); }

  // Button 4 toggles grabber spin
  bool btn4 = (RecievedButtons[4] == 1);
  if (btn4 && !lastButton4State) motorSpin();
  lastButton4State = btn4;

  // Center legs when no button held
  leftGroup.centerServos();
  rightGroup.centerServos();
}

// ==================== MANUAL GRABBER CONTROL ====================
// Opens or closes grabber based on Y axis
void ManualGrabber() {
  if (RecievedY <= 400 && grabberOpen) {
    board1.setPWM(9, 0, map(closeAngle, 0, 180, SERVOMIN, SERVOMAX));
    grabberOpen = false;
  } else if (RecievedY >= 600 && !grabberOpen) {
    board1.setPWM(9, 0, map(openAngle, 0, 180, SERVOMIN, SERVOMAX));
    grabberOpen = true;
  }
}

// ==================== GPS & NAVIGATION HELPERS ====================
// Convert raw NMEA ddmm.mmmm to degrees
double toDecimalDegrees(double raw) {
  int deg = int(raw) / 100;
  double mins = fmod(raw, 100);
  return deg + mins / 60.0;
}

// Compute initial bearing between two geo points
double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
  lat1 *= M_PI / 180;
  lon1 *= M_PI / 180;
  lat2 *= M_PI / 180;
  lon2 *= M_PI / 180;
  double dLon = lon2 - lon1;
  double x = sin(dLon) * cos(lat2);
  double y = cos(lat1)*sin(lat2) - sin(lat1)*cos(lat2)*cos(dLon);
  double brng = atan2(x, y) * 180.0 / M_PI;
  return fmod(brng + 360.0, 360.0);
}

// Normalize angle difference to -180..+180
double wrap180(double d) {
  if (d > 180)  d -= 360;
  if (d < -180) d += 360;
  return d;
}

// Haversine formula for distance in meters
double haversineDist(double lat1, double lon1, double lat2, double lon2) {
  constexpr double R = 6371000.0;
  double φ1 = lat1 * M_PI/180;
  double φ2 = lat2 * M_PI/180;
  double dφ = (lat2 - lat1) * M_PI/180;
  double dλ = (lon2 - lon1) * M_PI/180;
  double a = sin(dφ/2)*sin(dφ/2) + cos(φ1)*cos(φ2)*sin(dλ/2)*sin(dλ/2);
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

// ==================== MOVEMENT FEEDBACK ====================
// Chooses servo movement direction based on heading error
void updateMovement(double currentBearing, double targetBearing) {
  double diff = wrap180(targetBearing - currentBearing);
  if (fabs(diff) <= ALIGN_THRESHOLD) {
    leftGroup.moveServos(1);    rightGroup.moveServos(1);
  } else if (diff > 0) {
    leftGroup.moveServos(1);
  } else {
    rightGroup.moveServos(1);
  }
  strip.setBrightness(50);
  strip.show();
}

// ==================== AUTONOMOUS CONTROL ====================
// Guides robot toward target or along return path
void AutonomousControl() {
  GPS.read();
  if (GPS.newNMEAreceived() && !GPS.parse(GPS.lastNMEA())) return;
  if (!GPS.fix) return;

  unsigned long now = millis();
  if (now - lastCheckTime < interval) return;
  lastCheckTime = now;

  // Fetch and convert current position
  double lat = toDecimalDegrees(GPS.latitude);
  double lon = toDecimalDegrees(GPS.longitude);
  if (GPS.lat=='S') lat = -lat;
  if (GPS.lon=='W') lon = -lon;

  // Record outward track when not returning
  if (!returningHome) recordPos(lat, lon);

  // Override target if in return-home mode
  if (returningHome) {
    targetLat = history[returnIndex].lat;
    targetLon = history[returnIndex].lon;
  }

  // Compute distance to current target
  double dist = haversineDist(lat, lon, targetLat, targetLon);

  // If within 2m, either record or step return index
  if (returningHome) {
    if (dist <= 2.0) {
      returnIndex--;
      if (returnIndex < 0) {
        returningHome = false;
        receivedData.manualMode = true;
        leftGroup.centerServos();
        rightGroup.centerServos();
      }
      return;
    }
  } else if (dist <= 2.0) {
    // Arrived at live target: stop and record
    leftGroup.centerServos();
    rightGroup.centerServos();
    recordPos(lat, lon);
    return;
  }

  // Compute heading error and move
  double tgtBrg = calculateBearing(lat, lon, targetLat, targetLon);
  double curBrg = GPS.angle;
  updateMovement(curBrg, tgtBrg);

  // Adjust update rate based on alignment
  double diff = wrap180(tgtBrg - curBrg);
  interval = (fabs(diff) > ALIGN_THRESHOLD) ? FAST_INTERVAL : SLOW_INTERVAL;
}

// ==================== MAIN LOOP ====================
void loop() {
  if (ManualControlBool) {
    ManualControl();
    ManualGrabber();
  } else {
    AutonomousControl();
  }
}

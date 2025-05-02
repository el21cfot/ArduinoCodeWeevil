#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MMC56x3.h>
#include <Adafruit_GPS.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <cmath>

// ----- PINS & HARDWARE -----
#define GPS_RX       4        // GPS TX → ESP32 RX
#define GPS_TX      -1        // unused
#define LED_PIN    PIN_NEOPIXEL
#define NUM_LEDS       1
#define GPS_BAUD    9600

Adafruit_MMC5603 mag = Adafruit_MMC5603(12345);
HardwareSerial gpsSerial(1);
Adafruit_GPS GPS(&gpsSerial);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB+NEO_KHZ800);

// ----- TIMING & THRESHOLDS -----
unsigned long lastCheckTime = 0;
unsigned long interval     = 5000;  // ms between checks
constexpr double ALIGN_THRESHOLD = 15.0;     // degrees
constexpr unsigned long FAST_INTERVAL = 1000;
constexpr unsigned long SLOW_INTERVAL = 5000;

// ----- CALIBRATION & OFFSETS -----
float MagMinX, MagMaxX, MagMinY, MagMaxY, MagMinZ, MagMaxZ;
float xOffset = 0, yOffset = 0;
float northOffset = 0;
float declinationDeg = 0.0;  // set yours here

// ----- TARGET LOCATION -----
const double targetLat = 51.500729;
const double targetLon = -0.124625;

//–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

double ConvertToDecimalDegrees(double RawValue) {
  int deg = int(RawValue) / 100;
  double mins = fmod(RawValue, 100);
  return deg + mins/60.0;
}

double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
  // great-circle bearing
  lat1*=M_PI/180; lon1*=M_PI/180;
  lat2*=M_PI/180; lon2*=M_PI/180;
  double dLon = lon2-lon1;
  double x = sin(dLon)*cos(lat2);
  double y = cos(lat1)*sin(lat2)
           - sin(lat1)*cos(lat2)*cos(dLon);
  double brng = atan2(x,y)*180.0/M_PI;
  return fmod(brng+360.0,360.0);
}

double wrap180(double diff) {
  if (diff > 180)  diff -= 360;
  if (diff < -180) diff += 360;
  return diff;
}

void updateLED(double currentBearing, double targetBearing) {
  double diff = wrap180(targetBearing - currentBearing);
  if (fabs(diff) <= ALIGN_THRESHOLD) {
    strip.setPixelColor(0, strip.Color(0,255,0));        // green
  } else if (diff > 0) {
    strip.setPixelColor(0, strip.Color(0,0,255));        // blue → turn right
  } else {
    strip.setPixelColor(0, strip.Color(128,0,128));      // purple → turn left
  }
  strip.show();
}

void setup() {
  Serial.begin(115200);
  while(!Serial) delay(10);

  // NeoPixel
  strip.begin();
  strip.setBrightness(50);
  strip.show();

  // GPS init
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
  GPS.begin(GPS_BAUD);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  // Magnetometer init
  if (!mag.begin(MMC56X3_DEFAULT_ADDRESS, &Wire)) {
    Serial.println("ERROR: MMC5603 not found!");
    while(1) delay(10);
  }
  mag.setDataRate(50);
  mag.setContinuousMode(true);

  Serial.println("Setup complete.");
}

void loop() {
  // 1) Read GPS so we can get our position for computing targetBearing
  GPS.read();
  if (GPS.newNMEAreceived() && !GPS.parse(GPS.lastNMEA())) {
    // invalid sentence
  }

  // 2) Only proceed when we have a fix & interval elapsed
  unsigned long now = millis();
  if (!GPS.fix) {
    Serial.println("Waiting for GPS fix...");
    delay(500);
    return;
  }
  if (now - lastCheckTime < interval) return;
  lastCheckTime = now;

  // 3) Get own lat/lon
  double lat = ConvertToDecimalDegrees(GPS.latitude);
  double lon = ConvertToDecimalDegrees(GPS.longitude);
  if (GPS.lat=='S') lat = -lat;
  if (GPS.lon=='W') lon = -lon;

  // 4) Compute target bearing
  double targetBearing = calculateBearing(lat, lon, targetLat, targetLon);

  // 5) Read the compass for current bearing
  sensors_event_t evt;
  if (mag.getEvent(&evt)) {
    float cx = evt.magnetic.x - xOffset;
    float cy = evt.magnetic.y - yOffset;
    double heading = -atan2(cy,cx)*180.0/M_PI;
    heading += declinationDeg - northOffset;
    heading = fmod(heading+360.0,360.0);

    // 6) LED & logging
    Serial.print("Mag Bearing: "); Serial.println(heading);
    Serial.print("Target Brg : "); Serial.println(targetBearing);
    updateLED(heading, targetBearing);

    // 7) Adjust interval if off by > threshold
    double diff = wrap180(targetBearing - heading);
    interval = (fabs(diff) > ALIGN_THRESHOLD) ? FAST_INTERVAL : SLOW_INTERVAL;
  } else {
    Serial.println("Mag read error");
  }
}

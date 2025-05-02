#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MMC56x3.h>

Adafruit_MMC5603 mag = Adafruit_MMC5603(12345);

// Calibration extrema & offsets
float MagMinX, MagMaxX, MagMinY, MagMaxY, MagMinZ, MagMaxZ;
float xOffset = 0, yOffset = 0;

// “North” orientation offset
float northOffset = 0;

// Streaming control
bool streaming       = false;
unsigned long lastStreamTime = 0;
const unsigned long STREAM_INTERVAL = 1000;  // ms

// Your local declination (° east +, west –)
float declinationDeg = 0.0;

//==============================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  if (!mag.begin(MMC56X3_DEFAULT_ADDRESS, &Wire)) {
    Serial.println("ERROR: MMC5603 not found!");
    while (1) delay(10);
  }
  mag.setDataRate(50);
  mag.setContinuousMode(true);

  printMenu();
}

//==============================================================================
void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'c':  runCalibration();     break;
      case 'r':  readCompass();        break;
      case 's':  startStreaming();     break;
      case 'x':  stopStreaming();      break;
      case 'o':  setNorthReference();  break;
      default:   /* ignore */          break;
    }
  }

  if (streaming && (millis() - lastStreamTime >= STREAM_INTERVAL)) {
    readCompass();
    lastStreamTime = millis();
  }
}

//==============================================================================
// Print the user menu
void printMenu() {
  Serial.println("\n🧭 MMC5603 Compass Console");
  Serial.println("  c → calibrate (10 s)");
  Serial.println("  r → single heading");
  Serial.println("  s → start streaming 1 Hz");
  Serial.println("  x → stop streaming");
  Serial.println("  o → orient north here");
  Serial.println();
}

//==============================================================================
// start continuous 1 Hz streaming
void startStreaming() {
  streaming = true;
  lastStreamTime = millis();
  Serial.println("→ Streaming ON");
}

//==============================================================================
// stop streaming and show menu
void stopStreaming() {
  streaming = false;
  Serial.println("→ Streaming OFF");
  printMenu();
}

//==============================================================================
// 10 s hard-iron calibration
void runCalibration() {
  MagMinX = MagMinY = MagMinZ =  1e6;
  MagMaxX = MagMaxY = MagMaxZ = -1e6;

  Serial.println("🔄 Calibrating for 10 s… rotate on all axes now");
  unsigned long start = millis();
  sensors_event_t evt;
  while (millis() - start < 10000) {
    if (mag.getEvent(&evt)) {
      MagMinX = min(MagMinX, evt.magnetic.x);
      MagMaxX = max(MagMaxX, evt.magnetic.x);
      MagMinY = min(MagMinY, evt.magnetic.y);
      MagMaxY = max(MagMaxY, evt.magnetic.y);
      MagMinZ = min(MagMinZ, evt.magnetic.z);
      MagMaxZ = max(MagMaxZ, evt.magnetic.z);
    }
    delay(20);
  }

  xOffset = (MagMinX + MagMaxX) * 0.5;
  yOffset = (MagMinY + MagMaxY) * 0.5;

  Serial.println("✅ Calibration complete!");
  Serial.print(" Xmin=");    Serial.print(MagMinX,2);
  Serial.print(" Xmax=");    Serial.print(MagMaxX,2);
  Serial.print(" xOffset="); Serial.println(xOffset,2);
  Serial.print(" Ymin=");    Serial.print(MagMinY,2);
  Serial.print(" Ymax=");    Serial.print(MagMaxY,2);
  Serial.print(" yOffset="); Serial.println(yOffset,2);
  Serial.println();
}

//==============================================================================
// Record current heading as “north”
void setNorthReference() {
  sensors_event_t evt;
  if (!mag.getEvent(&evt)) {
    Serial.println("Read error");
    return;
  }
  // compute raw heading
  float cx = evt.magnetic.x - xOffset;
  float cy = evt.magnetic.y - yOffset;
  float h = -atan2(cy, cx) * 180.0 / PI + declinationDeg;
  if (h < 0)       h += 360;
  else if (h >=360) h -= 360;

  northOffset = h;
  Serial.print("📌 North reference set at ");
  Serial.print(northOffset,1);
  Serial.println("°");
  printMenu();
}

//==============================================================================
// Single compass read + print
void readCompass() {
  sensors_event_t evt;
  if (!mag.getEvent(&evt)) {
    Serial.println("Read error");
    return;
  }

  float cx = evt.magnetic.x - xOffset;
  float cy = evt.magnetic.y - yOffset;

  // compute heading (CW positive) + declination
  float heading = -atan2(cy, cx) * 180.0 / PI;
  heading += declinationDeg;

  // subtract your north reference
  heading -= northOffset;

  // wrap into 0…360
  if (heading < 0)       heading += 360.0;
  else if (heading >=360) heading -= 360.0;

  Serial.print("Heading: ");
  Serial.print(heading, 1);
  Serial.println("°");
}

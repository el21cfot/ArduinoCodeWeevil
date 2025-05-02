#include <Adafruit_GPS.h>
#include <HardwareSerial.h>
#include <iostream>
#include <cmath>

// Pins
#define GPS_RX 4     // GPS TX → ESP32 RX
#define GPS_TX -1    // Not used (only reading from GPS)
#define FIX_PIN 10   // Connected to GPS FIX output
#define GPS_BAUD 9600

// Setup GPS object using UART1
HardwareSerial gpsSerial(1);
Adafruit_GPS GPS(&gpsSerial);

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("Adafruit Ultimate GPS Test (via Adafruit_GPS library)");

  // Start GPS serial
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);

  // Configure the GPS module
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);  // Get basic location info
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);     // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);                // Enable antenna status messages

  pinMode(FIX_PIN, INPUT);
  
  Serial.println("Type any key in the Serial Monitor to get current GPS data.");
  Serial.println("Waiting for GPS data and FIX...");
}

//Function to turn the gps lat/long from degrees and minutes to decimal degrees
//RawValue in form dddmm.mmm...
//where d is degrees in form 0-360
//m is minutes in form 00-59.99999...
double ConvertToDecimalDegrees(double RawValue){
  //Extract degrees and minutes from raw data
  int degrees = static_cast<int>(RawValue)/100; //shifts decimal point left by two, leaving only the degrees wihtin int
  double minutes = fmod(RawValue, 100);
  //convert to decimal degrees
  double decimalDegrees = degrees + (minutes/60.0);
  return decimalDegrees;
}

//Function to find bearing between two GPS Co-ords
double calculateBearing(double lat1, double long1, double lat2, double long2) {
    // Convert latitude and longitude to radians
    double Radianlat1 = lat1 * M_PI / 180.0;
    double Radianlong1 = long1 * M_PI / 180.0;
    double Radianlat2 = lat2 * M_PI / 180.0;
    double Radianlong2 = long2 * M_PI / 180.0;

    // Calculate the bearing
    double deltaLong = Radianlong2 - Radianlong1;
    double x = sin(deltaLong) * cos(Radianlat2);
    double y = cos(Radianlat1) * sin(Radianlat2) - sin(Radianlat1) * cos(Radianlat2) * cos(deltaLong);
    double bearing = atan2(x, y);

    // Convert the bearing to degrees
    bearing = bearing * 180.0 / M_PI;

    // Normalize the bearing to 0°-360°
    bearing = fmod((bearing + 360.0), 360.0);
    
    return bearing;
}


void loop() {
  // Continuously read GPS characters
  GPS.read();

  // Handle complete NMEA sentences
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) {
      return; // Invalid sentence
    }
  }

  // Print on user request
  if (Serial.available()) {
    Serial.read();  // Clear the incoming char

    Serial.println("========== GPS REPORT ==========");

    // Handle FIX pin (interpreting behavior per datasheet)
    bool fixPinState = digitalRead(FIX_PIN);
    Serial.print("FIX pin state: ");
    Serial.println(fixPinState ? "Pulse (NO FIX)" : "Low (FIX OK)");

    // GPS fix status from NMEA sentence
    Serial.print("GPS Fix: ");
    Serial.println(GPS.fix ? "Yes" : "No");
    Serial.print("Satellites: ");
    Serial.println(GPS.satellites);

    if (GPS.fix) {
      double ConvertedLat = ConvertToDecimalDegrees(GPS.latitude);
      double ConvertedLong = ConvertToDecimalDegrees(GPS.longitude);
      Serial.print("Latitude: "); Serial.println(ConvertedLat, 6); Serial.println(GPS.lat);
      Serial.print("Longitude: "); Serial.println(ConvertedLong, 6); Serial.println(GPS.lon);
      Serial.print("Speed (knots): "); Serial.println(GPS.speed);
      Serial.print("Speed (km/h): "); Serial.println(GPS.speed * 1.852);
      Serial.print("Altitude (m): "); Serial.println(GPS.altitude);
      Serial.print("Course (deg): "); Serial.println(GPS.angle);

      Serial.print("Date (DDMMYY): ");
      Serial.print(GPS.day); Serial.print("/");
      Serial.print(GPS.month); Serial.print("/");
      Serial.println(GPS.year);

      Serial.print("Time (GMT): ");
      Serial.print((GPS.hour)+1); Serial.print(":");
      Serial.print(GPS.minute); Serial.print(":");
      Serial.println(GPS.seconds);

      double Lat2 =51.439250; 
      double Lon2 =-3.148754;
      if(GPS.lat == 'S'){ConvertedLat = -ConvertedLat;}
      if(GPS.lon == 'W'){ConvertedLong = -ConvertedLong;}
      double Bearing = calculateBearing(ConvertedLat, ConvertedLong, Lat2, Lon2);
      Serial.print("Bearing: "); Serial.println(Bearing); 
    } else {
      Serial.println("No valid GPS fix yet.");
    }

    Serial.println("================================");
  }
}

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <WiFi.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "thingProperties.h"

// Wi-Fi Credentials (Stored in arduino_secrets.h)
const char SSID[] = "21HessleMountMesh";
const char PASS[] = "University2425";

// Cloud Variables
CloudLocation location;  // For the Arduino Cloud Map Widget
float heading_deg;
float latitude;
float longitude;
float speed_kmph;
int satellites;

// GPS Configuration
#define RX_PIN 4  // GPS TX → ESP32 RX
#define TX_PIN -1 // TX not needed
#define GPS_BAUD 9600

TinyGPSPlus gps;
HardwareSerial gpsSerial(1); // Use UART1 for GPS

// Function Prototypes for Cloud Updates
//void onHeadingDegChange();
void onLatitudeChange();
void onLongitudeChange();
//void onSpeedKmphChange();
void onSatellitesChange();
void onLocationChange(); // New function for CloudLocation

void initProperties() {
    ArduinoCloud.addProperty(location, READWRITE, ON_CHANGE, onLocationChange);
    //ArduinoCloud.addProperty(heading_deg, READWRITE, ON_CHANGE, onHeadingDegChange);
    ArduinoCloud.addProperty(latitude, READWRITE, ON_CHANGE, onLatitudeChange);
    ArduinoCloud.addProperty(longitude, READWRITE, ON_CHANGE, onLongitudeChange);
    //ArduinoCloud.addProperty(speed_kmph, READWRITE, ON_CHANGE, onSpeedKmphChange);
    ArduinoCloud.addProperty(satellites, READWRITE, ON_CHANGE, onSatellitesChange);
}

// Wi-Fi Connection Handler
WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

void setup() {
    Serial.begin(115200);
    delay(1000); // Wait for Serial Monitor
    
    Serial.println("GPS Module Test Starting...");
    
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.println("GPS Serial started on RX pin 4.");

    // Start Arduino IoT Cloud
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);
}

void loop() {
    ArduinoCloud.update();  // Update Cloud Data
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status());
    Serial.print("Arduino Cloud Connection Status: ");
    Serial.println(ArduinoCloud.connected() ? "Connected" : "Disconnected");

    delay(2000); // Print status every 2 seconds

    while (gpsSerial.available()) {
        char c = gpsSerial.read();
        gps.encode(c); // Parse GPS data

        if (gps.location.isUpdated()) {
            latitude = gps.location.lat();
            longitude = gps.location.lng();
            location = {latitude, longitude};
            Serial.print("Location: ");
            Serial.print(latitude, 6);
            Serial.print(", ");
            Serial.println(longitude, 6);
        }
/*
        if (gps.speed.isUpdated()) {
            speed_kmph = gps.speed.kmph();
            Serial.print("Speed: ");
            Serial.print(speed_kmph);
            Serial.println(" km/h");
        }

        if (gps.course.isUpdated()) {
            heading_deg = gps.course.deg();
            Serial.print("Heading: ");
            Serial.print(heading_deg);
            Serial.println("°");
        }
*/
        if (gps.satellites.isUpdated()) {
            satellites = gps.satellites.value();
            Serial.print("Satellites: ");
            Serial.println(satellites);
        }
/*
        if (gps.altitude.isUpdated()) {
            Serial.print("Altitude: ");
            Serial.print(gps.altitude.meters());
            Serial.println(" m");
        }*/
    }
}
/*
// Cloud Variable Callbacks (Optional)
void onLocationChange() {}
void onHeadingDegChange() {}
void onLatitudeChange() {}
void onLongitudeChange() {}
void onSpeedKmphChange() {}
void onSatellitesChange() {}*/

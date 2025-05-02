#include <TinyGPS++.h>
#include <HardwareSerial.h>

#define RX_PIN 4   // GPS TX → ESP32 RX (working pin)
#define TX_PIN -1  // TX not needed
#define GPS_BAUD 9600

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);  // Use UART1 for GPS

void setup() {
    Serial.begin(115200);
    delay(1000);  // Wait for Serial Monitor

    Serial.println("GPS Module Test Starting...");
    
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.println("GPS Serial started on RX pin 2.");
}


void loop() {
    while (gpsSerial.available()) {
        char c = gpsSerial.read();
        gps.encode(c);  // Parse GPS data

        if (gps.location.isUpdated()) {
            Serial.print("Location: ");
            Serial.print(gps.location.lat(), 6);
            Serial.print(", ");
            Serial.println(gps.location.lng(), 6);
        }

        if (gps.speed.isUpdated()) {
            Serial.print("Speed: ");
            Serial.print(gps.speed.kmph());
            Serial.println(" km/h");
        }

        if (gps.course.isUpdated()) {
            Serial.print("Heading: ");
            Serial.print(gps.course.deg());
            Serial.println("°");
        }

        if (gps.satellites.isUpdated()) {
            Serial.print("Satellites: ");
            Serial.println(gps.satellites.value());
        }

        if (gps.altitude.isUpdated()) {
            Serial.print("Altitude: ");
            Serial.print(gps.altitude.meters());
            Serial.println(" m");
        }
    }
}

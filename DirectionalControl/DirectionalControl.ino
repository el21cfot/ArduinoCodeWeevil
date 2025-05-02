#include <Adafruit_GPS.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <cmath>

// Pins
#define GPS_RX 4     // GPS TX -> ESP32 RX
#define GPS_TX -1    // Not used (only reading from GPS)
#define LED_PIN PIN_NEOPIXEL  // Built-in NeoPixel
#define NUM_LEDS 1  // Only one NeoPixel LED
#define GPS_BAUD 9600

// Setup GPS object using UART1
HardwareSerial gpsSerial(1);
Adafruit_GPS GPS(&gpsSerial);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Timing intervals
unsigned long lastCheckTime = 0;
unsigned long interval = 5000; // Default to periodic checks (5 seconds)

// Function to convert raw GPS degrees+minutes to decimal degrees
double ConvertToDecimalDegrees(double RawValue) {
    int degrees = static_cast<int>(RawValue) / 100; // Extract degrees
    double minutes = fmod(RawValue, 100);          // Extract minutes
    return degrees + (minutes / 60.0);             // Convert to decimal degrees
}

// Function to calculate the bearing between two GPS coordinates
double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
    lat1 = lat1 * M_PI / 180.0; // Convert to radians
    lon1 = lon1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;
    lon2 = lon2 * M_PI / 180.0;

    double deltaLon = lon2 - lon1;
    double x = sin(deltaLon) * cos(lat2);
    double y = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(deltaLon);
    double bearing = atan2(x, y) * 180.0 / M_PI; // Convert to degrees
    return fmod((bearing + 360.0), 360.0);       // Normalize to 0-360
}

// Function to control the LED based on the direction
void updateLED(double currentBearing, double targetBearing) {
    double difference = targetBearing - currentBearing;

    // Normalize the difference to -180 to 180
    if (difference > 180) difference -= 360;
    if (difference < -180) difference += 360;

    // Check if within the 30-degree band (±15 degrees)
    if (fabs(difference) <= 15) {
        // In the correct direction
        strip.setPixelColor(0, strip.Color(0, 255, 0)); // Green
    } else if (difference > 0) {
        // Turn right
        strip.setPixelColor(0, strip.Color(0, 0, 255)); // Blue
    } else {
        // Turn left
        strip.setPixelColor(0, strip.Color(128, 0, 128)); // Purple
    }

    strip.show(); // Update the LED
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    // Initialize NeoPixel
    strip.begin();
    strip.setBrightness(50);
    strip.show(); // Clear LEDs

    Serial.println("Initializing GPS...");
    // Start GPS serial
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
    GPS.begin(9600);
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);  // Basic location info
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);     // 1 Hz update rate
    GPS.sendCommand(PGCMD_ANTENNA);                // Enable antenna status messages
}

void loop() {
    // Continuously read GPS data
    GPS.read();

    if (GPS.newNMEAreceived()) {
        if (!GPS.parse(GPS.lastNMEA())) {
            return; // Skip invalid sentences
        }
    }

    unsigned long currentMillis = millis();

    if (GPS.fix && currentMillis - lastCheckTime >= interval) {
        lastCheckTime = currentMillis;

        // Get current GPS position and convert to decimal degrees
        double currentLat = ConvertToDecimalDegrees(GPS.latitude);
        double currentLon = ConvertToDecimalDegrees(GPS.longitude);

        if (GPS.lat == 'S') currentLat = -currentLat; // Adjust hemisphere
        if (GPS.lon == 'W') currentLon = -currentLon;

        // Get current bearing
        double currentBearing = GPS.angle;

        // Manually entered target GPS position
        double targetLat = 51.500729;  // Example target latitude
        double targetLon = -0.124625; // Example target longitude

        // Calculate bearing to target position
        double targetBearing = calculateBearing(currentLat, currentLon, targetLat, targetLon);

        // Compare current bearing to target bearing
        double difference = targetBearing - currentBearing;

        // Normalize the difference to -180 to 180
        if (difference > 180) difference -= 360;
        if (difference < -180) difference += 360;

        // If out of bounds, update more frequently
        if (fabs(difference) > 15) {
            interval = 1000; // Check every second
            Serial.println("Out of bounds! Updating more frequently...");
        } else {
            interval = 5000; // Check every 5 seconds
        }

        // Update the LED
        Serial.print("Current Bearing: "); Serial.println(currentBearing);
        Serial.print("Target Bearing: "); Serial.println(targetBearing);
        Serial.print("Difference: "); Serial.println(difference);
        updateLED(currentBearing, targetBearing);
    } else if (!GPS.fix) {
        Serial.println("No GPS Fix.");
    }
}
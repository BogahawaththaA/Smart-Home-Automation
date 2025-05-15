#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>

// Pin Definitions
#define PIR_PIN 13
#define BUZZER_PIN 8
#define DHT_PIN 4
#define RELAY_DOOR_PIN 27  // Solenoid lock control
#define RELAY_LIGHT_PIN 12  // Relay for light

// DHT Settings
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// RFID Settings
#define SS_PIN 21
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

// WiFi Access Point Settings
const char* ssid = "ESP32_ApplianceControl";
const char* password = "12345678";

// WebServer Declaration
WebServer server(80);

// Variables
bool motionDetected = false;
byte authorizedUID[] = {0x93, 0x5D, 0x54, 0x27};  // Example RFID UID, replace with actual UID

// HTML Webpage for appliance control
String webpage = "";

// Function declarations
void handleRoot();
void handleLightOn();
void handleLightOff();

void setup() {
  // Initialize Serial port
  Serial.begin(115200);

  // Initialize components
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_DOOR_PIN, OUTPUT);
  pinMode(RELAY_LIGHT_PIN, OUTPUT);

  // Turn off buzzer and relays initially
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RELAY_DOOR_PIN, LOW);   // Lock the door initially
  digitalWrite(RELAY_LIGHT_PIN, LOW);

  // Initialize DHT sensor
  dht.begin();

  // Initialize RFID module
  SPI.begin();
  rfid.PCD_Init();

  // Start ESP32 Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Create the appliance control webpage
  webpage += "<h1>Appliance Control</h1>";
  webpage += "<p>Light Control: <a href=\"/lighton\"><button>ON</button></a>&nbsp;<a href=\"/lightoff\"><button>OFF</button></a></p>";

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/lighton", handleLightOn);
  server.on("/lightoff", handleLightOff);

  // Start the server
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  // Handle incoming web client requests
  server.handleClient();

  // Motion Detection
  motionDetected = digitalRead(PIR_PIN);
  if (motionDetected) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // RFID Module - Unlock the door if a valid RFID tag is detected
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (checkAuthorizedRFID(rfid.uid.uidByte, rfid.uid.size)) {
      Serial.println("Authorized RFID Detected! Unlocking Door...");
      digitalWrite(RELAY_DOOR_PIN, HIGH);  // Unlock Door (Solenoid)
      delay(5000);  // Keep door unlocked for 5 seconds
      digitalWrite(RELAY_DOOR_PIN, LOW);   // Lock Door again
    } else {
      Serial.println("Unauthorized RFID Detected!");
    }
    rfid.PICC_HaltA();  // Stop reading the card
  }

  // DHT Sensor readings
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Print temperature and humidity to Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" °C  Humidity: ");
  Serial.print(h);
  Serial.println(" %");

  delay(2000);
}

// Helper function to check if the detected RFID UID matches an authorized UID
bool checkAuthorizedRFID(byte *uid, byte uidLength) {
  for (byte i = 0; i < uidLength; i++) {
    if (uid[i] != authorizedUID[i]) {
      return false;  // If any byte doesn't match, return false
    }
  }
  return true;  // Return true if UID matches
}


#include <WiFi.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"

// Wi-Fi credentials
const char* ssid = "Ramakrishna";
const char* password = "12345677654321";

// ThingSpeak settings
const char* server = "api.thingspeak.com";
unsigned long channelID = 2972407;
const char* writeAPIKey = "1KVOIY3IGY2J2RHA";

WiFiClient client;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Pin configuration
const int ldrPin = 39;         // Analog pin connected to LDR
const int ledPin1 = 15;        // GPIO for LED 1
const int ledPin2 = 2;         // GPIO for LED 2
const int trigPin = 18;        // Ultrasonic Trigger pin
const int echoPin = 19;        // Ultrasonic Echo pin
int threshold = 450;

long duration;
float distance;

void setup() {
  Serial.begin(115200);

  pinMode(ldrPin, INPUT);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startAttemptTime >= 15000) {
      Serial.println("\nConnection failed! 
        
        Please check your Wi-Fi settings.");
      return;
    }
  }

  Serial.println("\nWi-Fi connected!");
  ThingSpeak.begin(client);

  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31 sensor!");
  } else {
    Serial.println("SHT31 sensor found!");
  }
}

void loop() {
  int ldrValue = analogRead(ldrPin);
  int ledBrightness = 0;

  // Read ultrasonic distance
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 10000);
  distance = (duration > 0) ? (duration * 0.0343 / 2.0) : -1;

  Serial.print("LDR Value: ");
  Serial.println(ldrValue);

  if (ldrValue > threshold) {
    Serial.println("Light is Dim");
    ledBrightness = 50;
  } else {
    Serial.println("Light is Bright");
    ledBrightness = 0;
  }

  if (distance > 0 && distance < 10.0) {
    Serial.print("Object Detected at: ");
    Serial.print(distance);
    Serial.println(" cm");
    ledBrightness = 255;
  } else if (distance >= 10.0) {
    Serial.println("No Object Detected");
  } else {
    Serial.println("Out of Range or Echo Failed");
  }

  float temp = sht31.readTemperature();
  float humidity = sht31.readHumidity();

  if (!isnan(temp) && !isnan(humidity)) {
    Serial.print("Temp *C = ");
    Serial.print(temp);
    Serial.print("\tHumidity % = ");
    Serial.println(humidity);

    // Shut down light if humidity > 80
    if (humidity > 80.0) {
      ledBrightness = 0;
      Serial.println("Humidity above 80%. Lights turned OFF.");
    }

    ThingSpeak.setField(4, temp);
    ThingSpeak.setField(5, humidity);
  } else {
    Serial.println("Failed to read from SHT31 sensor");
  }

  analogWrite(ledPin1, ledBrightness);
  analogWrite(ledPin2, ledBrightness);

  ThingSpeak.setField(1, ldrValue);
  ThingSpeak.setField(2, distance);
  ThingSpeak.setField(3, ledBrightness);

  int response = ThingSpeak.writeFields(channelID, writeAPIKey);
  if (response == 200) {
    Serial.println("Data sent to ThingSpeak successfully!");
  } else {
    Serial.print("ThingSpeak Error: ");
    Serial.println(response);
  }

  delay(10);
}
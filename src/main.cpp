#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Provide the RTDB payload printing info and other helper functions.
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Network credentials
const char* ssid = "UW MPSK";
const char* password = "5kK_e!L3;d";

// Firebase project credentials
#define DATABASE_URL "https://lab5-8a917-default-rtdb.firebaseio.com"
#define API_KEY "AIzaSyCvPg6cQ20Dtb-NDJuDqePviW62ztHV02w"

// HC-SR04 Pins
const int trigPin = 2;
const int echoPin = 3;

// Sound speed in cm/usec
const float soundSpeed = 0.034;

// Updated deep sleep and measurement intervals
#define DEEP_SLEEP_INTERVAL 40 // Sleep for 40 seconds if no object detected
#define MEASUREMENT_INTERVAL 1000 // Check every second

// Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signedIn = false;

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Firebase connected");

  // Initial deep sleep
  Serial.println("Going to deep sleep initially.");
  goToDeepSleep(DEEP_SLEEP_INTERVAL);
}

void loop() {
  float distance = measureDistance();

  // If an object is detected within 60cm, send data to Firebase
  if (distance < 60.0) {
    if (!signedIn) {
      signedIn = true;
      WiFi.reconnect();
    }
    sendDataToFirebase(distance);
  } else {
    // If no object detected within 60cm, go to deep sleep for 40 seconds
    goToDeepSleep(DEEP_SLEEP_INTERVAL);
  }
}

float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  return distance;
}

void sendDataToFirebase(float distance) {
  if (millis() - sendDataPrevMillis > MEASUREMENT_INTERVAL) {
    sendDataPrevMillis = millis();
    
    if (Firebase.RTDB.setFloat(&fbdo, "/distance", distance)) {
      Serial.println("Distance data sent to Firebase");
    } else {
      Serial.println("Failed to send data to Firebase");
      Serial.println(fbdo.errorReason());
    }
  }
}

void goToDeepSleep(long timeInSeconds) {
  Serial.printf("Entering deep sleep for %ld seconds...\n", timeInSeconds);
  WiFi.disconnect(true);
  esp_sleep_enable_timer_wakeup(timeInSeconds * 1000000ULL);
  esp_deep_sleep_start();
}

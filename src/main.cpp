#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Provide the RTDB payload printing info and other helper functions.
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Provide the methods to sign in with email and password.
const char* ssid = "UW MPSK";
const char* password = "5kK_e!L3;d";

// Define the Firebase Realtime Database URL and API Key
#define DATABASE_URL "https://lab5-8a917-default-rtdb.firebaseio.com"
#define API_KEY "AIzaSyCvPg6cQ20Dtb-NDJuDqePviW62ztHV02w"

// HC-SR04 Pins
const int trigPin = 2;
const int echoPin = 3;

// Define sound speed in cm/usec
const float soundSpeed = 0.034;

// Define the deep sleep interval
#define DEEP_SLEEP_INTERVAL 30 // Sleep for 30 seconds if no object is detected

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
  // This will never be called as we're using deep sleep
}

void goToDeepSleep(long timeInSeconds) {
  Serial.printf("Entering deep sleep for %ld seconds...\n", timeInSeconds);
  WiFi.disconnect(true);
  esp_sleep_enable_timer_wakeup(timeInSeconds * 1000000ULL); // time in microseconds
  esp_deep_sleep_start();
}

void wakeUpRoutine() {
  float distance = measureDistance();

  // If an object is detected within 60cm, send data to Firebase
  if (distance < 60.0) {
    if (!signedIn) {
      signedIn = true;
      WiFi.reconnect();
    }
    sendDataToFirebase(distance);
  }
  
  // Go back to deep sleep to save power
  goToDeepSleep(DEEP_SLEEP_INTERVAL);
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
  if (millis() - sendDataPrevMillis > uploadInterval) {
    sendDataPrevMillis = millis();
    
    if (Firebase.RTDB.setFloat(&fbdo, "/distance", distance)) {
      Serial.println("Distance data sent to Firebase");
    } else {
      Serial.println("Failed to send data to Firebase");
      Serial.println(fbdo.errorReason());
    }
  }
}

// The ISR for the wake stub
void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
  esp_default_wake_deep_sleep();
}

extern "C" void app_main() {
  // Setup the wake stub function
  esp_set_deep_sleep_wake_stub(&esp_wake_deep_sleep);

  // Call the Arduino setup
  setup();

  // Call the wake up routine
  wakeUpRoutine();

  // This is our "loop"
  while (1) {
    goToDeepSleep(DEEP_SLEEP_INTERVAL);
  }
}

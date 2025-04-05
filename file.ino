#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h> // <<< HTTPS এর জন্য এটি যোগ করুন
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// --- আপনার সেটিংস এখানে পরিবর্তন করুন ---
const char* ssid = "আপনার_ওয়াইফাই_নাম";         // আপনার Wi-Fi SSID
const char* password = "আপনার_ওয়াইফাই_পাসওয়ার্ড"; // আপনার Wi-Fi Password

// <<< নতুন HTTPS API URL >>>
const char* serverUrl = "https://esp8266-0jd1.onrender.com/api/esptempu";

#define DHT11_PIN D1 // DHT11 পিন
#define DHT22_PIN D2 // DHT22 পিন

#define DHTTYPE11 DHT11
#define DHTTYPE22 DHT22

const long utcOffsetInSeconds = 6 * 3600; // বাংলাদেশ সময় UTC+6
const unsigned long dataSendInterval = 60 * 1000; // প্রতি মিনিট
// --- সেটিংস পরিবর্তন শেষ ---

DHT dht11(DHT11_PIN, DHTTYPE11);
DHT dht22(DHT22_PIN, DHTTYPE22);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// <<< HTTPS এর জন্য WiFiClientSecure অবজেক্ট >>>
WiFiClientSecure clientSecure;

unsigned long lastSendTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nNodeMCU Starting (HTTPS Mode)...");

  dht11.begin();
  dht22.begin();
  Serial.println("DHT sensors initialized.");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  Serial.println("NTP client started.");

  // <<< নিরাপত্তা সতর্কতা: HTTPS সার্টিফিকেট যাচাইকরণ এড়িয়ে যাওয়া হচ্ছে >>>
  // এটি ম্যান-ইন-দ্য-মিডল অ্যাটাকের ঝুঁকি বাড়ায়।
  // প্রোডাকশন বা সংবেদনশীল ডেটার জন্য ফিঙ্গারপ্রিন্ট বা রুট CA ব্যবহার করুন।
  clientSecure.setInsecure();
  // বিকল্প (কম ব্যবহৃত): clientSecure.setFingerprint(fingerprint); // fingerprint খালি রাখুন যদি যাচাই না চান
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastSendTime >= dataSendInterval) {
    lastSendTime = currentTime;

    Serial.println("\nInterval reached. Reading sensors and sending data via HTTPS...");

    if (!timeClient.update()) {
       Serial.println("Failed to update time from NTP server.");
    }
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = localtime(&epochTime);
    char timeBuffer[20];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", ptm);
    String currentTimestamp(timeBuffer);

    float h1 = dht11.readHumidity();
    float t1 = dht11.readTemperature();
    float h2 = dht22.readHumidity();
    float t2 = dht22.readTemperature();

    if (isnan(h1) || isnan(t1) || isnan(h2) || isnan(t2)) {
      Serial.println("Error: Failed to read from DHT sensor!");
      return;
    }

    Serial.println("--- Sensor Readings ---");
    Serial.print("DHT11: Humi: "); Serial.print(h1); Serial.print(" %\tTemp: "); Serial.print(t1); Serial.println(" *C");
    Serial.print("DHT22: Humi: "); Serial.print(h2); Serial.print(" %\tTemp: "); Serial.print(t2); Serial.println(" *C");
    Serial.print("Timestamp: "); Serial.println(currentTimestamp);

    // DynamicJsonDocument ব্যবহার (যেমন আগের কোডে ছিল)
    DynamicJsonDocument jsonDoc(256);

    JsonObject sensor1 = jsonDoc.createNestedObject("sensor1");
    sensor1["temperature"] = t1;
    sensor1["humidity"] = h1;

    JsonObject sensor2 = jsonDoc.createNestedObject("sensor2");
    sensor2["temperature"] = t2;
    sensor2["humidity"] = h2;

    jsonDoc["timestamp"] = currentTimestamp;

    String jsonPayload;
    serializeJson(jsonDoc, jsonPayload);

    Serial.println("--- Sending Data (HTTPS) ---");
    Serial.println("JSON Payload: " + jsonPayload);

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      Serial.print("Connecting to server (HTTPS): ");
      Serial.println(serverUrl);

      // <<< http.begin এ clientSecure ব্যবহার করুন >>>
      http.begin(clientSecure, serverUrl);
      http.addHeader("Content-Type", "application/json");

      int httpResponseCode = http.POST(jsonPayload);

      if (httpResponseCode > 0) {
        Serial.print("HTTPS Response code: ");
        Serial.println(httpResponseCode);
        // String responsePayload = http.getString();
        // Serial.println("Server Response: " + responsePayload);
      } else {
        Serial.print("Error sending POST request (HTTPS). HTTP Code: ");
        Serial.println(httpResponseCode);
        Serial.printf("[HTTPS] POST... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
      }

      http.end();
    } else {
      Serial.println("Error: WiFi Disconnected. Cannot send data.");
    }
     Serial.println("-----------------------\n");
  }
}
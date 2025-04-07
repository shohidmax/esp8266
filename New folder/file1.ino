#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h> // HTTPS
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <OneWire.h>             // Dallas Sensor
#include <DallasTemperature.h> // Dallas Sensor
#include <Wire.h>                // I2C Communication
#include <Adafruit_GFX.h>        // OLED GFX Library
#include <Adafruit_SSD1306.h>    // OLED Driver (SSD1306)

// --- আপনার সেটিংস এখানে পরিবর্তন করুন ---
const char* ssid = "War";
const char* password = "Hello2025@";
const char* serverUrl = "https://esp8266-0jd1.onrender.com/api/esptempu"; // HTTPS API URL

// --- পিন কনফিগারেশন ---
#define DHT11_PIN D1 // DHT11 পিন (GPIO5)
#define DHT22_PIN D2 // DHT22 পিন (GPIO4)
#define ONE_WIRE_BUS D3 // Dallas সেন্সরের ডাটা পিন (GPIO0)
// <<< I2C পিন পরিবর্তন করা হয়েছে >>>
#define I2C_SDA_PIN D7 // GPIO13
#define I2C_SCL_PIN D6 // GPIO12

// --- ডিসপ্লে কনফিগারেশন ---
#define SCREEN_WIDTH 128 // OLED ডিসপ্লে প্রস্থ, পিক্সেলে
#define SCREEN_HEIGHT 64 // OLED ডিসপ্লে উচ্চতা, পিক্সেলে
#define OLED_RESET    -1 // রিসেট পিন (-1 যদি না থাকে)
#define SCREEN_ADDRESS 0x3C // <<< আপনার ডিসপ্লের I2C এড্রেস (0x3C বা 0x3D)

// --- অন্যান্য সেটিংস ---
#define DHTTYPE11 DHT11
#define DHTTYPE22 DHT22
const long utcOffsetInSeconds = 6 * 3600; // বাংলাদেশ সময় UTC+6
const unsigned long dataSendInterval = 60 * 1000; // সার্ভারে ডেটা পাঠানোর বিরতি (১ মিনিট)
const unsigned long sensorReadInterval = 5000; // সেন্সর রিডিং ও ডিসপ্লে আপডেটের বিরতি (৫ সেকেন্ড)
// --- সেটিংস পরিবর্তন শেষ ---

// --- অবজেক্ট তৈরি ---
// সেন্সর
DHT dht11(DHT11_PIN, DHTTYPE11);
DHT dht22(DHT22_PIN, DHTTYPE22);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature dallasSensors(&oneWire);
// ডিসপ্লে (SSD1306)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// নেটওয়ার্ক ও টাইম
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
WiFiClientSecure clientSecure; // HTTPS

// --- গ্লোবাল ভেরিয়েবল ---
unsigned long lastSendTime = 0;
unsigned long lastReadTime = 0;
// সর্বশেষ সেন্সর রিডিং সংরক্ষণের জন্য ভেরিয়েবল
float latest_t1 = NAN, latest_h1 = NAN;
float latest_t2 = NAN, latest_h2 = NAN;
float latest_t3 = NAN;
bool t3_valid_global = false; // Store validity of t3

// --- ফাংশন ডিক্লারেশন ---
void readSensors();
void updateDisplay();
void sendDataToServer();

// ================= SETUP =================
void setup() {
    Serial.begin(115200);
    Serial.println("\n\nNodeMCU Starting (HTTPS + Dallas + OLED)...");

    // I2C শুরু (কাস্টম পিন দিয়ে)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("I2C Initialized on custom pins (SDA: D7, SCL: D6).");

    // ডিসপ্লে শুরু
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        // এখানে ডিসপ্লে কাজ না করলে লুপ চলতে থাকবে না
        // আপনি চাইলে এখানে অন্য ব্যবস্থা নিতে পারেন
        for(;;);
    }
    Serial.println("OLED Initialized.");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("Initializing...");
    display.println("Connecting WiFi...");
    display.display(); // ডিসপ্লেতে লেখা দেখান

    // সেন্সর শুরু
    dht11.begin();
    dht22.begin();
    dallasSensors.begin();
    Serial.println("Sensors initialized.");

    // ওয়াইফাই সংযোগ
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ");
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: "); Serial.println(WiFi.localIP());
    display.clearDisplay(); // ডিসপ্লে মুছে ফেলুন
    display.setCursor(0,0);
    display.println("WiFi Connected!");
    display.print("IP: "); display.println(WiFi.localIP());
    display.display();
    delay(2000); // আইপি এড্রেস দেখার জন্য অপেক্ষা

    // NTP ক্লায়েন্ট চালু
    timeClient.begin();
    Serial.println("NTP client started.");

    // HTTPS নিরাপত্তা
    clientSecure.setInsecure();
    Serial.println("HTTPS certificate check bypassed (setInsecure).");

    // প্রথমবার সেন্সর পড়ুন ও ডিসপ্লে আপডেট করুন
    readSensors();
    updateDisplay();
    lastReadTime = millis(); // রিডিং এর সময় রিসেট করুন
}

// ================= LOOP =================
void loop() {
    unsigned long currentTime = millis();

    // --- সেন্সর রিডিং ও ডিসপ্লে আপডেট (প্রতি sensorReadInterval পর) ---
    if (currentTime - lastReadTime >= sensorReadInterval) {
        lastReadTime = currentTime;
        readSensors();    // সেন্সর থেকে ডেটা পড়ুন
        updateDisplay(); // ডিসপ্লে আপডেট করুন
    }

    // --- সার্ভারে ডেটা পাঠানো (প্রতি dataSendInterval পর) ---
    if (currentTime - lastSendTime >= dataSendInterval) {
        lastSendTime = currentTime;
        sendDataToServer(); // সার্ভারে ডেটা পাঠান
    }

    // এখানে অন্য কোনো কাজ থাকলে যোগ করা যেতে পারে
    // তবে ডিসপ্লে আপডেটের জন্য লুপ যাতে ব্লক না হয় সেদিকে খেয়াল রাখতে হবে
    // delay(10); // খুব ছোট ডিলে দেওয়া যেতে পারে
}

// ================= FUNCTIONS =================

// --- সেন্সর থেকে ডেটা পড়ার ফাংশন ---
void readSensors() {
    Serial.println("Reading sensors...");
    // DHT সেন্সর
    latest_h1 = dht11.readHumidity();
    latest_t1 = dht11.readTemperature();
    latest_h2 = dht22.readHumidity();
    latest_t2 = dht22.readTemperature();

    // Dallas সেন্সর
    dallasSensors.requestTemperatures();
    float temp_t3 = dallasSensors.getTempCByIndex(0);
    // ভ্যালিডেশন চেক করুন এবং গ্লোবাল ভেরিয়েবলে সেভ করুন
    t3_valid_global = (temp_t3 != DEVICE_DISCONNECTED_C && temp_t3 > -55.0 && temp_t3 < 125.0);
    if (t3_valid_global) {
        latest_t3 = temp_t3;
    } else {
        latest_t3 = NAN; // ভ্যালিড না হলে NAN সেট করুন
        Serial.print("DS18B20 Read Failed! Code: "); Serial.println(temp_t3);
    }

    // সিরিয়ালে প্রিন্ট (ডিবাগিং এর জন্য)
    Serial.print("DHT11: H:"); Serial.print(isnan(latest_h1)?"NaN":String(latest_h1)); Serial.print("% T:"); Serial.print(isnan(latest_t1)?"NaN":String(latest_t1)); Serial.println("C");
    Serial.print("DHT22: H:"); Serial.print(isnan(latest_h2)?"NaN":String(latest_h2)); Serial.print("% T:"); Serial.print(isnan(latest_t2)?"NaN":String(latest_t2)); Serial.println("C");
    Serial.print("DS18B20: T:"); Serial.println(t3_valid_global ? String(latest_t3) : "NaN");
}

// --- OLED ডিসপ্লে আপডেট করার ফাংশন ---
void updateDisplay() {
    display.clearDisplay(); // ডিসপ্লে বাফার ক্লিয়ার করুন
    display.setTextSize(1);      // টেক্সট সাইজ সেট করুন
    display.setTextColor(SSD1306_WHITE); // টেক্সট কালার সেট করুন
    display.setCursor(0,0);     // কার্সর পজিশন (উপরের বাম কোণ)

    // Sensor 1 ডেটা প্রিন্ট
    display.print("S1: ");
    display.print(isnan(latest_t1) ? "--.-" : String(latest_t1, 1)); // ১ ডেসিমাল প্লেস
    display.print("C ");
    display.print(isnan(latest_h1) ? "--.-" : String(latest_h1, 1)); // ১ ডেসিমাল প্লেস
    display.println("%");

    // Sensor 2 ডেটা প্রিন্ট
    display.print("S2: ");
    display.print(isnan(latest_t2) ? "--.-" : String(latest_t2, 1));
    display.print("C ");
    display.print(isnan(latest_h2) ? "--.-" : String(latest_h2, 1));
    display.println("%");

    // Sensor 3 ডেটা প্রিন্ট
    display.print("S3: ");
    display.print(!t3_valid_global ? "--.-" : String(latest_t3, 1)); // NAN চেক করার দরকার নেই, t3_valid_global ব্যবহার করুন
    display.println("C");

    // ওয়াইফাই স্ট্যাটাস ও আইপি
    display.println("---------------"); // বিভাজক
    if (WiFi.status() == WL_CONNECTED) {
        display.print("IP: "); display.println(WiFi.localIP());
    } else {
        display.println("WiFi Disconnected");
    }

    // বর্তমান সময় (যদি NTP সফল হয়)
    if (timeClient.isTimeSet()) { // Check if time is set
        display.print("Time: "); display.println(timeClient.getFormattedTime());
    } else {
        display.println("Time: Updating...");
    }

    display.display(); // ডিসপ্লেতে লেখা দেখান
    Serial.println("Display updated.");
}

// --- সার্ভারে ডেটা পাঠানোর ফাংশন ---
void sendDataToServer() {
    Serial.println("Preparing to send data to server...");

    // সময় আপডেট (পাঠানোর ঠিক আগে একবার আপডেট করা ভালো)
    if (!timeClient.update()) { Serial.println("NTP update failed before sending."); }
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = localtime(&epochTime);
    char timeBuffer[20];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", ptm);
    String currentTimestamp(timeBuffer);

    // JSON তৈরি (গ্লোবাল ভেরিয়েবল থেকে সর্বশেষ ডেটা ব্যবহার করে)
    DynamicJsonDocument jsonDoc(384);

    JsonObject sensor1 = jsonDoc.createNestedObject("sensor1");
    if (isnan(latest_t1)) { sensor1["temperature"] = (const char*)nullptr; } else { sensor1["temperature"] = latest_t1; }
    if (isnan(latest_h1)) { sensor1["humidity"] = (const char*)nullptr; } else { sensor1["humidity"] = latest_h1; }

    JsonObject sensor2 = jsonDoc.createNestedObject("sensor2");
    if (isnan(latest_t2)) { sensor2["temperature"] = (const char*)nullptr; } else { sensor2["temperature"] = latest_t2; }
    if (isnan(latest_h2)) { sensor2["humidity"] = (const char*)nullptr; } else { sensor2["humidity"] = latest_h2; }

    JsonObject sensor3 = jsonDoc.createNestedObject("sensor3");
    if (!t3_valid_global) { sensor3["temperature"] = (const char*)nullptr; } else { sensor3["temperature"] = latest_t3; }

    jsonDoc["timestamp"] = currentTimestamp;

    String jsonPayload;
    serializeJson(jsonDoc, jsonPayload);

    // ডেটা পাঠান (HTTPS)
    Serial.println("--- Sending Data (HTTPS) ---");
    Serial.println("JSON Payload: " + jsonPayload);

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        Serial.print("Connecting to server (HTTPS): "); Serial.println(serverUrl);
        http.begin(clientSecure, serverUrl);
        http.addHeader("Content-Type", "application/json");
        int httpResponseCode = http.POST(jsonPayload);
         if (httpResponseCode > 0) { Serial.print("HTTPS Response code: "); Serial.println(httpResponseCode); }
         else { Serial.print("Error sending POST request (HTTPS). HTTP Code: "); Serial.println(httpResponseCode); Serial.printf("[HTTPS] POST... failed, error: %s\n", http.errorToString(httpResponseCode).c_str()); }
        http.end();
    } else {
        Serial.println("Error: WiFi Disconnected. Cannot send data.");
    }
    Serial.println("-----------------------\n");
}


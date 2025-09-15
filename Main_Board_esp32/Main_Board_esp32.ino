/*#define BLYNK_TEMPLATE_ID "TMPL3mZFOrjhm"
#define BLYNK_TEMPLATE_NAME "Motor pump"
#define BLYNK_AUTH_TOKEN "CrixqXjvc4Xz13zTQuy6Czr3m_0swe4r"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "ThingSpeak.h"
#include <esp_now.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
WidgetLCD lcd(V5); 
// Pin Definitions
#define SOIL_MOISTURE_PIN 34
#define DHTPIN 13
#define RELAY_PIN 33 // Relay control pin

// WiFi Credentials
const char* ssid = "Murari";
const char* password = "123456789";

// ThingSpeak Credentials
WiFiClient client;
unsigned long Channel_ID = 1;
const char* API_Key = "1UF45VWZBUFQOO06";

// DHT Sensor Initialization
DHT dht(DHTPIN, DHT11);

// OpenWeatherMap API
String currentWeatherURL = "http://api.openweathermap.org/data/2.5/weather?";
String forecastWeatherURL = "http://api.openweathermap.org/data/2.5/forecast?";
String ApiKey = "bd490c7ad91bf1d77958de3d8fe50174";
String lat = "13.28472212104206";
String lon = "77.59595678671047";

// Blynk Auth Token
char auth[] = BLYNK_AUTH_TOKEN;

// ESP-NOW Data Structure
typedef struct struct_message {
    int rnd_1;
    int rnd_2;
    int rnd_3;
} struct_message;

struct_message receive_Data;

// Global Variables
BlynkTimer timer;
int pinValue = 0;
int pinValue2 = 0;
int temp = 0, humid = 0, soil_moi = 0;
int avgTemp = 0, avgHumid = 0, avgSoilMoi = 0;
float temp_weather = 0;
float humidity_weather = 0;
float current_rainfall = 0;
float predicted_rainfall[5];
float predicted_temp[5];
float predicted_humidity[5];
float predictedRainfallPercentage[5]; // Declare it globally
float totalPredictedTemp = 0;
float totalPredictedHumidity = 0;
float totalPredictedRainfallPercentage = 0;
float avgPredictedTemp=0;
float avgPredictedHumidity=0;
float avgPredictedRainfallPercentage=0;
float currentRainfallPercentage=0;
// Crop Growth Duration (3 Months = 90 Days)
unsigned long startTime;
const int totalDays = 90;
#define MAX_RAINFALL 100  
// Growth Stages Calculation Based on Time Progression
const int transplantingDays = 15;
const int tilleringDays = 30;
const int floweringDays = 60;
const int grainFillingDays = 75;
const int maturityDays = 90;
// Function to convert rainfall into percentage
float rainfallToPercentage(float rainfall) {
    return (rainfall / MAX_RAINFALL) * 100;
}
// Function to Determine Growth Stage
String getGrowthStage() {
    int elapsedDays = (millis() - startTime) / (1000 * 60 * 60 * 24);
    if (elapsedDays < transplantingDays) return "Transplanting";
    else if (elapsedDays < tilleringDays) return "Tillering";
    else if (elapsedDays < floweringDays) return "Flowering";
    else if (elapsedDays < grainFillingDays) return "Grain Filling";
    else return "Maturity";
}

// Function to Control Irrigation Based on Growth Stage
void controlIrrigation(int soilMoisture) {
    String stage = getGrowthStage();
    int threshold;

    // Base threshold for irrigation depending on the growth stage
    if (stage == "Transplanting") threshold = 80;
    else if (stage == "Tillering") threshold = 60;
    else if (stage == "Flowering") threshold = 50;
    else if (stage == "Grain Filling") threshold = 40;
    else threshold = 30;

    // Map the rainfall percentage (50 to 100) to a value that can be added to the threshold
    if (currentRainfallPercentage > 50) {
        int rainfallAdjustment = map(currentRainfallPercentage, 50, 100, 0, 20); // Maps rainfall from 50-100% to 0-20
        threshold -= rainfallAdjustment;
        Serial.print("Current rainfall is: "); 
        Serial.print(currentRainfallPercentage); 
        Serial.print("%, Adjusting threshold by: "); 
        Serial.println(rainfallAdjustment);
    }

    // Determine whether to irrigate based on soil moisture and adjusted threshold
    if (soilMoisture < threshold) {
        // If soil moisture is below threshold, irrigate
        motorON();
        Serial.println("Motor ON: Low Soil Moisture Detected");
    } else {
        // If soil moisture is above threshold, stop irrigation
        motorOFF();
        Serial.println("Motor OFF: Sufficient Soil Moisture");
    }
}

void fetchWeatherData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        // Fetch current weather data
        String requestURL = currentWeatherURL + "lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + ApiKey;
        http.begin(requestURL);
        int httpCode = http.GET();
        if (httpCode > 0) {
            String JSON_Data = http.getString();
            DynamicJsonDocument doc(4096);
            deserializeJson(doc, JSON_Data);
            temp_weather = doc["main"]["temp"].as<float>();
            humidity_weather = doc["main"]["humidity"].as<float>();
            current_rainfall = doc["rain"]["1h"].as<float>();
            if (isnan(current_rainfall)) {
                current_rainfall = 0;
            }
        }
        http.end();

        // Fetch forecast weather data for the next 5 days
        requestURL = forecastWeatherURL + "lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + ApiKey;
        http.begin(requestURL);
        httpCode = http.GET();
        if (httpCode > 0) {
            String JSON_Data = http.getString();
            DynamicJsonDocument doc(8192);
            deserializeJson(doc, JSON_Data);

            // Loop through the forecast data and extract information for the next 5 days (3-hour intervals)
            for (int i = 0; i < 5; i++) {
                float sumTemp = 0;
                float sumHumid = 0;
                float sumRainfall = 0;
                int count = 0;

                // Calculate average temperature, humidity, and rainfall for each day
                for (int j = i * 8; j < (i + 1) * 8; j++) {  // 8 * 3 hours = 24 hours per day
                    sumTemp += doc["list"][j]["main"]["temp"].as<float>();
                    sumHumid += doc["list"][j]["main"]["humidity"].as<float>();
                    sumRainfall += doc["list"][j]["rain"]["3h"].as<float>();  // Rainfall for the 3-hour interval
                    count++;
                }

                // Store the average temperature, humidity, and rainfall for the day
                predicted_temp[i] = sumTemp / count;
                predicted_humidity[i] = sumHumid / count;
                predicted_rainfall[i] = sumRainfall / count;
                predictedRainfallPercentage[i] = rainfallToPercentage(predicted_rainfall[i]); // Convert to percentage
            }
        }
        http.end();

        // Convert current rainfall to percentage
        currentRainfallPercentage = rainfallToPercentage(current_rainfall);

        // Debug print the weather data 
        Serial.println("==== Weather Data ====");
        Serial.print("Current Temperature: "); Serial.print(temp_weather); Serial.println("°C");
        Serial.print("Current Humidity: "); Serial.print(humidity_weather); Serial.println("%");
        Serial.print("Current Rainfall Percentage: "); Serial.print(currentRainfallPercentage); Serial.println("%");

        // Print predicted temperature, humidity, and rainfall for the next 5 days
        for (int i = 0; i < 5; i++) {
            Serial.print("Predicted Temperature Day "); Serial.print(i + 1); Serial.print(": ");
            Serial.print(predicted_temp[i]); Serial.println("°C");
            Serial.print("Predicted Humidity Day "); Serial.print(i + 1); Serial.print(": ");
            Serial.print(predicted_humidity[i]); Serial.println("%");
            Serial.print("Predicted Rainfall Percentage Day "); Serial.print(i + 1); Serial.print(": ");
            Serial.print(predictedRainfallPercentage[i]); Serial.println("%");
        }

        // Calculate the average predicted temperature, humidity, and rainfall for the next 5 days
        totalPredictedTemp = 0;
        totalPredictedHumidity = 0;
        totalPredictedRainfallPercentage = 0;

        for (int i = 0; i < 5; i++) {
            totalPredictedTemp += predicted_temp[i];
            totalPredictedHumidity += predicted_humidity[i];
            totalPredictedRainfallPercentage += predictedRainfallPercentage[i];
        }

        // Calculate the overall average across all 5 days
        avgPredictedTemp = totalPredictedTemp / 5;
        avgPredictedHumidity = totalPredictedHumidity / 5;
        avgPredictedRainfallPercentage = totalPredictedRainfallPercentage / 5;
      
        // Print the averages in the forecast data output
        Serial.println("==== Average Forecast for the Next 5 Days ====");
        Serial.print("Average Predicted Temperature: "); Serial.print(avgPredictedTemp); Serial.println("°C");
        Serial.print("Average Predicted Humidity: "); Serial.print(avgPredictedHumidity); Serial.println("%");
        Serial.print("Average Predicted Rainfall Percentage: "); Serial.print(avgPredictedRainfallPercentage); Serial.println("%");

        Serial.println("=====================");
    }
}
// ESP-NOW Callback Function
void OnDataRecv(const esp_now_recv_info* recvInfo, const uint8_t* incomingData, int len) {
    memcpy(&receive_Data, incomingData, sizeof(receive_Data));
    temp = receive_Data.rnd_1;
    humid = receive_Data.rnd_2;
   soil_moi = receive_Data.rnd_3;
    Serial.println("\n--- ESP-NOW Data Received ---");
    Serial.print("Temperature: "); Serial.println(temp);
    Serial.print("Humidity: "); Serial.println(humid);
    Serial.print("Soil Moisture: "); Serial.println(soil_moi);
    Serial.println("----------------------------");
}
BLYNK_WRITE(V0) {
    pinValue = param.asInt();
    digitalWrite(RELAY_PIN, pinValue);
    Serial.print("Motor Control (V0): ");
    Serial.println(pinValue ? "OFF" : "ON");
}
BLYNK_WRITE(V1) {
    pinValue2 = param.asInt();
    Serial.print("Control Mode (V1): ");
    Serial.println(pinValue2 == 0 ? "Manual" : "Automatic");
}
// Motor Control Functions
void motorON() {
    digitalWrite(RELAY_PIN, LOW);  // Relay ON
    Blynk.virtualWrite(V0, 0);
    Serial.println("Motor is ON");
}

void motorOFF() {
    digitalWrite(RELAY_PIN, HIGH);   // Relay OFF
    Blynk.virtualWrite(V0, 1);
    Serial.println("Motor is OFF");
}
void manualMode() {
    if (pinValue == 1) {
        motorON();
    } else {
        motorOFF();
    }
}
void automaticMode() {
  controlIrrigation(avgSoilMoi);
}
int scaleSoilMoisture(int rawValue) {
    const int sensorMin = 4095; // Dry value (0% moisture)
    const int sensorMax = 1000; // Wet value (100% moisture)
    
    // Reverse mapping: High values (dry) -> 0%, Low values (wet) -> 100%
    int scaledValue = map(rawValue, sensorMax, sensorMin, 100, 0);

    // Constrain the value to ensure it remains within 0-100%
    scaledValue = constrain(scaledValue, 0, 100);
    return scaledValue;
}
// Periodic Task for Sensor Reading, ThingSpeak, and Blynk Updates
void periodicTask() {
    int localTemp = dht.readTemperature();
    int localHumid = dht.readHumidity();
    int localSoilMoi = analogRead(SOIL_MOISTURE_PIN);


    if (!isnan(localTemp) && !isnan(localHumid)) {
      int scaledLocalSoilMoi = scaleSoilMoisture(localSoilMoi);
        int scaledRemoteSoilMoi = scaleSoilMoisture(soil_moi);
       avgTemp = (localTemp + temp + temp_weather) / 3;
       avgHumid = (localHumid + humid + humidity_weather) / 3;
       avgSoilMoi = (scaledLocalSoilMoi + scaledRemoteSoilMoi) / 2;
        //avgTemp =30;
       //avgHumid =40;
       //avgSoilMoi =50;
       float currentRainfallPercentage = rainfallToPercentage(current_rainfall);
        // Send data to ThingSpeak and Blynk
        ThingSpeak.setField(1, avgTemp);
        ThingSpeak.setField(2, avgHumid);
        ThingSpeak.setField(3, avgSoilMoi);
        ThingSpeak.writeFields(Channel_ID, API_Key);

        Blynk.virtualWrite(V2, avgTemp);
        Blynk.virtualWrite(V3, avgHumid);
        Blynk.virtualWrite(V4, avgSoilMoi);
        Blynk.virtualWrite(V7,currentRainfallPercentage);
        Blynk.virtualWrite(V6,avgPredictedHumidity);
        Blynk.virtualWrite(V9,avgPredictedTemp);
        Blynk.virtualWrite(V8,avgPredictedRainfallPercentage);
        lcd.print(0, 0,"St:"+getGrowthStage());

        // Control irrigation based on soil moisture
        if (pinValue2 == 1) {
            automaticMode();         
        }
        
    }
}

// Function to Fetch Crop Prediction
void fetchCropPrediction() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://192.168.0.191:5000/predict");  // Replace with your PC's local IP

        http.addHeader("Content-Type", "application/json");

        // Prepare JSON payload
// Prepare JSON payload
String jsonPayload = "{\"temperature\": " + String(avgPredictedTemp) +  
                     ", \"humidity\": " + String(avgPredictedHumidity) +
                     ", \"soil_moisture\": " + String(avgSoilMoi) +
                     ", \"rainfall\": " + String(avgPredictedRainfallPercentage) + "}";


        // Send HTTP POST request
        int httpCode = http.POST(jsonPayload);

        if (httpCode > 0) {
            String JSON_Data = http.getString();
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, JSON_Data);

            float predicted_temp = doc["predicted_temperature"];
            String recommended_crop = doc["recommended_crop"];

            Serial.print("Predicted Temperature: ");
            Serial.println(predicted_temp);

            Serial.print("Recommended Crop: ");
            Serial.println(recommended_crop);

            // Send to Blynk
            lcd.print(0, 1,"recom:"+recommended_crop);
        } else {
            Serial.println("Error fetching prediction!");
        }
        http.end();
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(SOIL_MOISTURE_PIN, INPUT);
    digitalWrite(RELAY_PIN, HIGH);
    dht.begin();
   lcd.clear();
    // WiFi Initialization
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWiFi Connected!");
    ThingSpeak.begin(client);

    // ESP-NOW Initialization
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Initialization Failed!");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);

    // Blynk Initialization
    Blynk.begin(auth, ssid, password);
    Serial.println("Blynk Connected!");

    startTime = millis(); // Start time tracking

    timer.setInterval(1000L, periodicTask);
    timer.setInterval(60000L, fetchCropPrediction);
    timer.setInterval(30000L, fetchWeatherData);
}

void loop() {
    Blynk.run();
    timer.run();
}
*/

#define BLYNK_TEMPLATE_ID "TMPL3mZFOrjhm"
#define BLYNK_TEMPLATE_NAME "Motor pump"
#define BLYNK_AUTH_TOKEN "CrixqXjvc4Xz13zTQuy6Czr3m_0swe4r"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "ThingSpeak.h"
#include <esp_now.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
WidgetLCD lcd(V5); 
// Pin Definitions
#define FLOW_SENSOR_PIN 23
#define SOIL_MOISTURE_PIN 34
#define DHTPIN 13
#define RELAY_PIN 33 // Relay control pin
volatile int pulseCount = 0;
float flowRate = 0.0;
float totalLiters = 0.0;
unsigned long lastTime = 0;
// WiFi Credentials
const char* ssid = "Murari";
const char* password = "123456789";

// Interrupt Service Routine (ISR) to count pulses
void IRAM_ATTR pulseCounter() {
    pulseCount++;
}
// ThingSpeak Credentials
WiFiClient client;
unsigned long Channel_ID = 1;
const char* API_Key = "1UF45VWZBUFQOO06";

// DHT Sensor Initialization
DHT dht(DHTPIN, DHT11);

// OpenWeatherMap API
String currentWeatherURL = "http://api.openweathermap.org/data/2.5/weather?";
String forecastWeatherURL = "http://api.openweathermap.org/data/2.5/forecast?";
String ApiKey = "bd490c7ad91bf1d77958de3d8fe50174";
String lat = "13.28472212104206";
String lon = "77.59595678671047";

// Blynk Auth Token
char auth[] = BLYNK_AUTH_TOKEN;

// ESP-NOW Data Structure
typedef struct struct_message {
    int rnd_1;
    int rnd_2;
    int rnd_3;
} struct_message;

struct_message receive_Data;

// Global Variables
BlynkTimer timer;
int pinValue = 0;
int pinValue2 = 0;
int temp = 0, humid = 0, soil_moi = 0;
int avgTemp = 0, avgHumid = 0, avgSoilMoi = 0;
float temp_weather = 0;
float humidity_weather = 0;
float current_rainfall = 0;
float predicted_rainfall[5];
float predicted_temp[5];
float predicted_humidity[5];
float predictedRainfallPercentage[5]; // Declare it globally
float totalPredictedTemp = 0;
float totalPredictedHumidity = 0;
float totalPredictedRainfallPercentage = 0;
float avgPredictedTemp=0;
float avgPredictedHumidity=0;
float avgPredictedRainfallPercentage=0;
float currentRainfallPercentage=0;
// Crop Growth Duration (3 Months = 90 Days)
unsigned long startTime;
const int totalDays = 90;
#define MAX_RAINFALL 100  
// Growth Stages Calculation Based on Time Progression
const int transplantingDays = 15;
const int tilleringDays = 30;
const int floweringDays = 60;
const int grainFillingDays = 75;
const int maturityDays = 90;
// Function to convert rainfall into percentage
float rainfallToPercentage(float rainfall) {
    return (rainfall / MAX_RAINFALL) * 100;
}
// Function to Determine Growth Stage
String getGrowthStage() {
    int elapsedDays = (millis() - startTime) / (1000 * 60 * 60 * 24);
    if (elapsedDays < transplantingDays) return "Transplanting";
    else if (elapsedDays < tilleringDays) return "Tillering";
    else if (elapsedDays < floweringDays) return "Flowering";
    else if (elapsedDays < grainFillingDays) return "Grain Filling";
    else return "Maturity";
}

// Function to Control Irrigation Based on Growth Stage
void controlIrrigation(int soilMoisture) {
    String stage = getGrowthStage();
    int threshold;

    // Base threshold for irrigation depending on the growth stage
    if (stage == "Transplanting") threshold = 80;
    else if (stage == "Tillering") threshold = 60;
    else if (stage == "Flowering") threshold = 50;
    else if (stage == "Grain Filling") threshold = 40;
    else threshold = 30;

    // Map the rainfall percentage (50 to 100) to a value that can be added to the threshold
    if (currentRainfallPercentage > 50) {
        int rainfallAdjustment = map(currentRainfallPercentage, 50, 100, 0, 20); // Maps rainfall from 50-100% to 0-20
        threshold += rainfallAdjustment;
        Serial.print("Current rainfall is: "); 
        Serial.print(currentRainfallPercentage); 
        Serial.print("%, Adjusting threshold by: "); 
        Serial.println(rainfallAdjustment);
    }

    // Determine whether to irrigate based on soil moisture and adjusted threshold
    if (soilMoisture < threshold) {
        // If soil moisture is below threshold, irrigate
        motorON();
        Serial.println("Motor ON: Low Soil Moisture Detected");
    } else {
        // If soil moisture is above threshold, stop irrigation
        motorOFF();
        Serial.println("Motor OFF: Sufficient Soil Moisture");
    }
}

void fetchWeatherData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        // Fetch current weather data
        String requestURL = currentWeatherURL + "lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + ApiKey;
        http.begin(requestURL);
        int httpCode = http.GET();
        if (httpCode > 0) {
            String JSON_Data = http.getString();
            DynamicJsonDocument doc(4096);
            deserializeJson(doc, JSON_Data);
            temp_weather = doc["main"]["temp"].as<float>();
            humidity_weather = doc["main"]["humidity"].as<float>();
            current_rainfall = doc["rain"]["1h"].as<float>();
            if (isnan(current_rainfall)) {
                current_rainfall = 0;
            }
        }
        http.end();

        // Fetch forecast weather data for the next 5 days
        requestURL = forecastWeatherURL + "lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + ApiKey;
        http.begin(requestURL);
        httpCode = http.GET();
        if (httpCode > 0) {
            String JSON_Data = http.getString();
            DynamicJsonDocument doc(8192);
            deserializeJson(doc, JSON_Data);

            // Loop through the forecast data and extract information for the next 5 days (3-hour intervals)
            for (int i = 0; i < 5; i++) {
                float sumTemp = 0;
                float sumHumid = 0;
                float sumRainfall = 0;
                int count = 0;

                // Calculate average temperature, humidity, and rainfall for each day
                for (int j = i * 8; j < (i + 1) * 8; j++) {  // 8 * 3 hours = 24 hours per day
                    sumTemp += doc["list"][j]["main"]["temp"].as<float>();
                    sumHumid += doc["list"][j]["main"]["humidity"].as<float>();
                    sumRainfall += doc["list"][j]["rain"]["3h"].as<float>();  // Rainfall for the 3-hour interval
                    count++;
                }

                // Store the average temperature, humidity, and rainfall for the day
                predicted_temp[i] = sumTemp / count;
                predicted_humidity[i] = sumHumid / count;
                predicted_rainfall[i] = sumRainfall / count;
                predictedRainfallPercentage[i] = rainfallToPercentage(predicted_rainfall[i]); // Convert to percentage
            }
        }
        http.end();

        // Convert current rainfall to percentage
        currentRainfallPercentage = rainfallToPercentage(current_rainfall);

        // Debug print the weather data 
        Serial.println("==== Weather Data ====");
        Serial.print("Current Temperature: "); Serial.print(temp_weather); Serial.println("°C");
        Serial.print("Current Humidity: "); Serial.print(humidity_weather); Serial.println("%");
        Serial.print("Current Rainfall Percentage: "); Serial.print(currentRainfallPercentage); Serial.println("%");

        // Print predicted temperature, humidity, and rainfall for the next 5 days
        for (int i = 0; i < 5; i++) {
            Serial.print("Predicted Temperature Day "); Serial.print(i + 1); Serial.print(": ");
            Serial.print(predicted_temp[i]); Serial.println("°C");
            Serial.print("Predicted Humidity Day "); Serial.print(i + 1); Serial.print(": ");
            Serial.print(predicted_humidity[i]); Serial.println("%");
            Serial.print("Predicted Rainfall Percentage Day "); Serial.print(i + 1); Serial.print(": ");
            Serial.print(predictedRainfallPercentage[i]); Serial.println("%");
        }

        // Calculate the average predicted temperature, humidity, and rainfall for the next 5 days
        totalPredictedTemp = 0;
        totalPredictedHumidity = 0;
        totalPredictedRainfallPercentage = 0;

        for (int i = 0; i < 5; i++) {
            totalPredictedTemp += predicted_temp[i];
            totalPredictedHumidity += predicted_humidity[i];
            totalPredictedRainfallPercentage += predictedRainfallPercentage[i];
        }

        // Calculate the overall average across all 5 days
        avgPredictedTemp = totalPredictedTemp / 5;
        avgPredictedHumidity = totalPredictedHumidity / 5;
        avgPredictedRainfallPercentage = totalPredictedRainfallPercentage / 5;
      
        // Print the averages in the forecast data output
        Serial.println("==== Average Forecast for the Next 5 Days ====");
        Serial.print("Average Predicted Temperature: "); Serial.print(avgPredictedTemp); Serial.println("°C");
        Serial.print("Average Predicted Humidity: "); Serial.print(avgPredictedHumidity); Serial.println("%");
        Serial.print("Average Predicted Rainfall Percentage: "); Serial.print(avgPredictedRainfallPercentage); Serial.println("%");

        Serial.println("=====================");
    }
}
// ESP-NOW Callback Function
void OnDataRecv(const esp_now_recv_info* recvInfo, const uint8_t* incomingData, int len) {
    memcpy(&receive_Data, incomingData, sizeof(receive_Data));
    temp = receive_Data.rnd_1;
    humid = receive_Data.rnd_2;
   soil_moi = receive_Data.rnd_3;
    Serial.println("\n--- ESP-NOW Data Received ---");
    Serial.print("Temperature: "); Serial.println(temp);
    Serial.print("Humidity: "); Serial.println(humid);
    Serial.print("Soil Moisture: "); Serial.println(soil_moi);
    Serial.println("----------------------------");
}
BLYNK_WRITE(V0) {
    pinValue = param.asInt();
    digitalWrite(RELAY_PIN, pinValue);
    Serial.print("Motor Control (V0): ");
    Serial.println(pinValue ? "OFF" : "ON");
}
BLYNK_WRITE(V1) {
    pinValue2 = param.asInt();
    Serial.print("Control Mode (V1): ");
    Serial.println(pinValue2 == 0 ? "Manual" : "Automatic");
}
// Motor Control Functions
void motorON() {
    digitalWrite(RELAY_PIN, LOW);  // Relay ON
    Blynk.virtualWrite(V0, 0);
    Serial.println("Motor is ON");
}

void motorOFF() {
    digitalWrite(RELAY_PIN, HIGH);   // Relay OFF
    Blynk.virtualWrite(V0, 1);
    Serial.println("Motor is OFF");
}
void manualMode() {
    if (pinValue == 1) {
        motorON();
    } else {
        motorOFF();
    }
}
void automaticMode() {
  controlIrrigation(avgSoilMoi);
}
int scaleSoilMoisture(int rawValue) {
    const int sensorMin = 4095; // Dry value (0% moisture)
    const int sensorMax = 1000; // Wet value (100% moisture)
    
    // Reverse mapping: High values (dry) -> 0%, Low values (wet) -> 100%
    int scaledValue = map(rawValue, sensorMax, sensorMin, 100, 0);

    // Constrain the value to ensure it remains within 0-100%
    scaledValue = constrain(scaledValue, 0, 100);
    return scaledValue;
}
// Periodic Task for Sensor Reading, ThingSpeak, and Blynk Updates
void periodicTask() {
    int localTemp = dht.readTemperature();
    int localHumid = dht.readHumidity();
    int localSoilMoi = analogRead(SOIL_MOISTURE_PIN);


    if (!isnan(localTemp) && !isnan(localHumid)) {
      int scaledLocalSoilMoi = scaleSoilMoisture(localSoilMoi);
        int scaledRemoteSoilMoi = scaleSoilMoisture(soil_moi);
       avgTemp = (localTemp + temp + temp_weather) / 3;
       avgHumid = (localHumid + humid + humidity_weather) / 3;
       avgSoilMoi = (scaledLocalSoilMoi + scaledRemoteSoilMoi) / 2;

       float currentRainfallPercentage = rainfallToPercentage(current_rainfall);
        // Send data to ThingSpeak and Blynk
        ThingSpeak.setField(1, avgTemp);
        ThingSpeak.setField(2, avgHumid);
        ThingSpeak.setField(3, avgSoilMoi);
        ThingSpeak.setField(4, currentRainfallPercentage);
        ThingSpeak.setField(5, totalLiters);
        ThingSpeak.writeFields(Channel_ID, API_Key);

        Blynk.virtualWrite(V2, avgTemp);
        Blynk.virtualWrite(V3, avgHumid);
        Blynk.virtualWrite(V4, avgSoilMoi);
        Blynk.virtualWrite(V7,currentRainfallPercentage);
        Blynk.virtualWrite(V6,avgPredictedHumidity);
        Blynk.virtualWrite(V9,avgPredictedTemp);
        Blynk.virtualWrite(V8,avgPredictedRainfallPercentage);
        lcd.print(0, 0,"St:"+getGrowthStage());

        // Control irrigation based on soil moisture
        if (pinValue2 == 1) {
            automaticMode();         
        }
        
    }
}

// Function to Fetch Crop Prediction
void fetchCropPrediction() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://172.22.223.247:5000/predict");  // Replace with your PC's local http://172.22.223.247:5000

        http.addHeader("Content-Type", "application/json");

        // Prepare JSON payload
// Prepare JSON payload
String jsonPayload = "{\"temperature\": " + String(avgPredictedTemp) +  
                     ", \"humidity\": " + String(avgPredictedHumidity) +
                     ", \"soil_moisture\": " + String(avgSoilMoi) +
                     ", \"rainfall\": " + String(avgPredictedRainfallPercentage) + "}";


        // Send HTTP POST request
        int httpCode = http.POST(jsonPayload);

        if (httpCode > 0) {
            String JSON_Data = http.getString();
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, JSON_Data);

            float predicted_temp = doc["predicted_temperature"];
            String recommended_crop = doc["recommended_crop"];

            Serial.print("Predicted Temperature: ");
            Serial.println(predicted_temp);

            Serial.print("Recommended Crop: ");
            Serial.println(recommended_crop);
lcd.clear();
            // Send to Blynk
            lcd.print(0, 1,"recom:"+recommended_crop);
        } else {
            Serial.println("Error fetching prediction!");
        }
        http.end();
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(SOIL_MOISTURE_PIN, INPUT);
    digitalWrite(RELAY_PIN, HIGH);
    dht.begin();
   lcd.clear();
    // WiFi Initialization
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWiFi Connected!");
    ThingSpeak.begin(client);
pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);
    // ESP-NOW Initialization
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Initialization Failed!");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);

    // Blynk Initialization
    Blynk.begin(auth, ssid, password);
    Serial.println("Blynk Connected!");

    startTime = millis(); // Start time tracking

    timer.setInterval(1000L, periodicTask);
    timer.setInterval(60000L, fetchCropPrediction);
    timer.setInterval(30000L, fetchWeatherData);
}

void loop() {
    Blynk.run();
    unsigned long currentMillis = millis();

    // Calculate flow rate every second
    if (currentMillis - lastTime > 1000) {
        detachInterrupt(FLOW_SENSOR_PIN);  // Disable interrupt to read pulses safely
        float pulseFrequency = pulseCount;  // Read pulse count

        // Convert to flow rate (L/min)
        flowRate = (pulseFrequency / 7.5);  

        // Convert to total liters (assuming readings per second)
        totalLiters += (flowRate / 60);

        Serial.print("Flow Rate: ");
        Serial.print(flowRate);
        Serial.println(" L/min");

        Serial.print("Total Water Used: ");
        Serial.print(totalLiters);
        Serial.println(" L");

        // Reset pulse counter and re-enable interrupt
        pulseCount = 0;
        lastTime = currentMillis;
        attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);
    }
    timer.run();
}




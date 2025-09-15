#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>

// Pin Definitions
#define DHTPIN 13
#define SOIL_MOISTURE_PIN 34

// WiFi Credentials
const char* ssid = "Murari";
const char* password = "123456789";

// Peer address (Receiver ESP32)
uint8_t broadcastAddress[] = {0x78, 0x21, 0x84, 0x9C, 0xAD, 0x70};

// DHT Sensor Initialization
DHT dht(DHTPIN, DHT11);

// Variables to store sensor readings
float temp = 0, humid = 0, soil_moi = 0;

// ESP-NOW Message Structure
typedef struct struct_message {
    int rnd_1;  // Temperature
    int rnd_2;  // Humidity
    int rnd_3;  // Soil Moisture
} struct_message;

struct_message send_Data;
esp_now_peer_info_t peerInfo;

// ESP-NOW Callback Function
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Setup function
void setup() {
    Serial.begin(115200);

    // Initialize DHT Sensor
    dht.begin();

    // Initialize Soil Moisture Pin
    pinMode(SOIL_MOISTURE_PIN, INPUT);

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWiFi Connected.");

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_send_cb(OnDataSent);

    // Register Peer
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }
}

// Loop function
void loop() {
    // Read Temperature and Humidity from DHT Sensor
    temp = dht.readTemperature();
    humid = dht.readHumidity();

    // Read Soil Moisture Sensor
    soil_moi = analogRead(SOIL_MOISTURE_PIN);

    // Check if DHT Sensor Readings are Valid
    if (isnan(temp) || isnan(humid)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    // Prepare Data for Sending
    send_Data.rnd_1 = static_cast<int>(temp);   // Convert to integer if needed
    send_Data.rnd_2 = static_cast<int>(humid);  // Convert to integer if needed
    send_Data.rnd_3 = static_cast<int>(soil_moi);

    // Send Data via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&send_Data, sizeof(send_Data));
    if (result == ESP_OK) {
        Serial.println("Data sent over ESP-NOW successfully.");
    } else {
        Serial.println("Error sending data over ESP-NOW");
    }

    delay(1000); // Adjust delay as needed
}

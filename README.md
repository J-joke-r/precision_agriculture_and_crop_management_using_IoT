🌱 Precision Agriculture and Crop Management using IoT and AI

This capstone project focuses on smart irrigation and crop management by integrating IoT sensors, cloud computing, and AI-driven recommendations. The goal is to optimize water usage, increase yield efficiency, and assist farmers with irrigation scheduling and crop rotation planning.

🚀 Key Features

IoT-based Data Collection

Soil Moisture (YL-69), Temperature & Humidity (DHT22), Water Flow (YFS-201), and Rainfall sensors.

Real-time field data collected via ESP32 microcontroller.

Smart Irrigation Algorithm

Automates irrigation using soil moisture, weather API, and rainfall data.

Everyday irrigation cycle for paddy fields + optimized scheduling for other crops.

Motor pumps controlled automatically through cloud-connected logic.

Crop Rotation Prediction (AI Integration)

Uses Prophet model with 3–4 years of historical data.

Suggests which crop yields maximum output with minimum water usage.

Helps farmers decide the best crop to plant at a given time.

Cloud & App Integration

Data uploaded to ThingSpeak and stored as CSV.

Analysis results pushed back to Blynk IoT mobile app for farmer-friendly interface.

Web + mobile dashboards for real-time monitoring.

🛠️ Tech Stack

Hardware: ESP32, YL-69 (Soil Moisture), DHT22, YFS-201 (Flow Sensor), Water Pump

Software/Platforms: ThingSpeak, Blynk IoT, Python (Prophet), Spyder IDE

Algorithms: Custom irrigation logic + AI forecasting for crop rotation

Data: Sensor readings (Soil Moisture, Temperature, Humidity, Flow, Rainfall) + Historical crop yield data

📊 Outcomes

Optimized irrigation → reduced water wastage.

Real-time farmer decision support → crop selection + irrigation scheduling.

Cost-effective and scalable system for precision agriculture.

'''from flask import Flask, request, jsonify
import pandas as pd
import pickle
from prophet import Prophet

app = Flask(__name__)

# Load the trained Prophet model
with open("prophet_model.pkl", "rb") as f:
    model = pickle.load(f)

# Load the crop dataset (replace with the actual path to your dataset)
crop_data = pd.read_csv("crop_data.csv")

# Function to recommend a crop based on real-time conditions and dataset
def recommend_crop(temp, humidity, soil_moisture, rainfall):
    """Determine the best crop based on environmental conditions using dataset."""
    
    # Find the closest matching row in the dataset (you can customize the logic here)
    closest_match = crop_data.iloc[(crop_data['temp'] - temp).abs().argmin()]
    
    # You can improve this logic by finding a better match based on other factors.
    recommended_crop = closest_match['crop_name']
    
    return recommended_crop

@app.route('/predict', methods=['POST'])
def predict():
    """Receive real-time data from ESP32 and predict the best crop."""
    data = request.json

    # Extract real-time sensor data from ESP32
    temperature = data["temperature"]
    humidity = data["humidity"]
    soil_moisture = data["soil_moisture"]
    rainfall = data["rainfall"]

    # Optionally, predict future temperature using Prophet
    future = model.make_future_dataframe(periods=30)
    forecast = model.predict(future)
    predicted_temp = forecast["yhat"].iloc[-1]  # Future temp prediction

    # Recommend crop using real-time temp and conditions
    crop = recommend_crop(temperature, humidity, soil_moisture, rainfall)

    return jsonify({
        "real_time_temperature": temperature,
        "predicted_temperature": predicted_temp,
        "recommended_crop": crop
    })

if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000)

'''




from flask import Flask, request, jsonify
import pandas as pd
import pickle
from prophet import Prophet

app = Flask(__name__)

# Load the trained Prophet model
with open("prophet_model.pkl", "rb") as f:
    model = pickle.load(f)
def recommend_crop(temp, humidity, soil_moisture, rainfall):
    """Determine the best crop based on environmental conditions."""
    
    # Rice should be prioritized if all its conditions are met
    if 20 <= temp <= 30 and humidity >= 60 and soil_moisture >= 50 and rainfall >= 100:
        return "Rice"

    # Wheat should be chosen if its conditions match, and Rice wasn't selected
    elif 18 <= temp <= 25 and humidity >= 50 and soil_moisture >= 40 and rainfall < 100:
        return "Wheat"

    # Corn should be selected only if Rice and Wheat don't match
    elif 22 <= temp <= 35 and soil_moisture >=0 and humidity < 60:
        return "Corn"

    # If no crops match
    return "No suitable crop found"


@app.route('/predict', methods=['POST'])
def predict():
    """Receive real-time data from ESP32 and predict the best crop."""
    data = request.json

    # Extract real-time sensor data from ESP32
    temperature = data["temperature"]  # ✅ Now using ESP32's temperature
    humidity = data["humidity"]
    soil_moisture = data["soil_moisture"]
    rainfall = data["rainfall"]

    # ✅ Optionally, predict future temperature using Prophet
    future = model.make_future_dataframe(periods=30)
    forecast = model.predict(future)
    predicted_temp = forecast["yhat"].iloc[-1]  # Future temp prediction

    # ✅ Recommend crop using real-time temp (not predicted)
    crop = recommend_crop(temperature, humidity, soil_moisture, rainfall)

    return jsonify({
        "real_time_temperature": temperature,
        "predicted_temperature": predicted_temp,
        "recommended_crop": crop
    })

if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000)
    

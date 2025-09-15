'''import pandas as pd
import pickle
from prophet import Prophet

# Load Dataset
df = pd.read_csv("crop_data.csv")

# Rename and Convert Date Column
df.rename(columns={"Date": "ds", "Temperature": "y"}, inplace=True)
df["ds"] = pd.to_datetime(df["ds"], errors="coerce")

# Remove Duplicates & NaN Dates
df = df.drop_duplicates(subset=["ds"])
df = df.dropna(subset=["ds"])

# Train Prophet Model
model = Prophet()
model.fit(df)

# Save the trained model
with open("prophet_model.pkl", "wb") as f:
    pickle.dump(model, f)

print("✅ Prophet model trained and saved!")'''

import pandas as pd
import pickle
import matplotlib.pyplot as plt
from prophet import Prophet

# Load Dataset
df = pd.read_csv("crop_data.csv")

# Rename and Convert Date Column
df.rename(columns={"Date": "ds", "Temperature": "y"}, inplace=True)
df["ds"] = pd.to_datetime(df["ds"], errors="coerce")

# Remove Duplicates & NaN Dates
df = df.drop_duplicates(subset=["ds"])
df = df.dropna(subset=["ds"])

# Train Prophet Model
model = Prophet()
model.fit(df)

# Save the trained model
with open("prophet_model.pkl", "wb") as f:
    pickle.dump(model, f)

# Create Future DataFrame
future = model.make_future_dataframe(periods=365)

# Predict Future Values
forecast = model.predict(future)

# Plot Forecast
fig = model.plot(forecast)
plt.title("Temperature Forecast")
plt.xlabel("Date")
plt.ylabel("Temperature")
plt.show()

print("✅ Prophet model trained, saved, and forecast graph displayed!")



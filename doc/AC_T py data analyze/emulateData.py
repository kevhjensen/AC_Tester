import pandas as pd
import matplotlib.pyplot as plt

# Parse CSV file (with no header row)
df = pd.read_csv(
    "emulateData1.csv",
    header=None,          # no header in file
    names=["Index", "Voltage", "Current", "c"]  # custom column names
)


df = df.iloc[:100]

print(df.head())
print(df.tail())

import numpy as np

V = df["Voltage"].to_numpy()
I = df["Current"].to_numpy()

# RMS calculations
Vrms = np.sqrt(np.mean(V**2))
Irms = np.sqrt(np.mean(I**2))

# Real power (average of instantaneous power)
P = np.mean(V * I)

# Apparent power (Vrms * Irms)
S = Vrms * Irms

# Power factor
PF = P / S if S != 0 else np.nan

print(f"Vrms: {Vrms:.4f} V")
print(f"Irms: {Irms:.4f} A")
print(f"Power Factor: {PF:.4f}")

# Plot using DataFrame's own index
plt.figure(figsize=(10, 5))
plt.plot(df.index, df["Voltage"], label="Voltage (V)")
plt.plot(df.index, 25 * df["Current"], label="Current (A)")

plt.xlabel("DF Index")
plt.ylabel("Value")
plt.title("AC Voltage and Current")
plt.legend()
plt.grid(True)
plt.show()
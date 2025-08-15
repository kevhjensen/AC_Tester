# import serial

# ser = serial.Serial('COM3', 115200)  # adjust COM port and baudrate
# with open('log1.csv', 'w') as f:
#     while True:
#         line = ser.readline().decode(errors='ignore').strip()
#         f.write(line + '\n')
#         print(line)


import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


def ADS_raw24sInt_to_mV(raw):
    return raw * 512 / (2**23)
def AC_V_from_mV(AC_V_mV):
    return AC_V_mV / 1.009
def AC_I_from_mV(AC_I_mV):
    return AC_I_mV / 3.12 #3.12 = 7.8/2.5 AX80 burden resistor / divider ratio/1000
def AC_V_from_raw(raw):
    return AC_V_from_mV(ADS_raw24sInt_to_mV(raw))
def AC_I_from_raw(raw):
    return AC_I_from_mV(ADS_raw24sInt_to_mV(raw))

def CP_V_from_raw(raw):
    adc_V = (330 * raw + 2047) / 4095
    dist_from_mid = adc_V - 165
    dist_from_mid_actual = dist_from_mid * 11
    return 165 + dist_from_mid_actual


def analyze_pwm(df, column, sample_rate_hz):
    """
    Analyze PWM signal in a DataFrame column sampled at `sample_rate_hz`.

    Args:
        df: DataFrame with a 'time' column in seconds and one column to analyze.
        column: Name of the column to analyze (voltage waveform).
        sample_rate_hz: Sampling rate in Hz (default 2kHz).

    Returns:
        DataFrame with PWM measurements every 100ms.
    """
    window_size = int(sample_rate_hz * 0.1)  # 100 ms window
    results = []

    for i in range(0, len(df) - window_size + 1, window_size):
        window = df.iloc[i:i + window_size]
        t_vals = window['time'].to_numpy()
        v_vals = window[column].to_numpy()

        # Calculate signal thresholds
        v_high = np.max(v_vals)
        v_low = np.min(v_vals)
        threshold = (v_high + v_low) / 2.0

        # Detect edges
        rising = np.where((v_vals[1:] >= threshold) & (v_vals[:-1] < threshold))[0] + 1
        falling = np.where((v_vals[1:] <= threshold) & (v_vals[:-1] > threshold))[0] + 1

        # Only use cycles that have both rising and falling edges
        duty_cycles = []
        periods = []

        for j in range(len(rising) - 1):
            rise_start = rising[j]
            rise_next = rising[j + 1]

            # Find the falling edge between these two rising edges
            f_edges = falling[(falling > rise_start) & (falling < rise_next)]
            if len(f_edges) == 0:
                continue  # skip incomplete cycle

            fall = f_edges[0]
            high_time = t_vals[fall] - t_vals[rise_start]
            period = t_vals[rise_next] - t_vals[rise_start]

            if period > 0:
                duty = 100 * high_time / period
                duty_cycles.append(duty)
                periods.append(period)

        # Compute results
        pwm_freq = 1 / np.mean(periods) if periods else np.nan
        duty_avg = np.mean(duty_cycles) if duty_cycles else np.nan

        results.append({
            'start_time': t_vals[0],
            'v_high': v_high,
            'v_low': v_low,
            'pwm_freq_hz': pwm_freq,
            'duty_cycle_percent': duty_avg
        })

    return pd.DataFrame(results)


df = pd.read_csv("F150.csv", header=None, names=["numSamples", "AC_V_raw", "AC_I_raw", "CP_raw"])

# df = df.iloc[90000:100000]
df = df.iloc[9000:9030]

df['time'] = 0
secsElapsed = 0
for i in range(len(df)):
    if df['numSamples'].iloc[i] <= 1:
        secsElapsed += 1
    df['time'].iloc[i] = secsElapsed + float(df['numSamples'].iloc[i]) / 1734.0


df['CP_V'] = CP_V_from_raw(df['CP_raw'])

with pd.option_context('display.max_rows', None, 'display.max_columns', None):
    print(df['CP_raw'])

plt.scatter(df['time'], df['CP_raw'])
# plt.scatter(df['time'], df['CP_V'])
plt.xlabel('Time')
plt.grid(True)
plt.legend()
plt.show()

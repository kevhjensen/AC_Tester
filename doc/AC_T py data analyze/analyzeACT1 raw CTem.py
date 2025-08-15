import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

file_path = "F150_July25_raw1.csv"
data = []

# Parse only V/I lines
with open(file_path, 'r') as f:
    for line in f:
        parts = line.strip().split(',')
        if len(parts) >= 3 and parts[1].strip() in ['V', 'I']:
            try:
                timestamp = int(parts[0])
                value = int(parts[2])
                label = parts[1].strip()
                data.append({'time': timestamp, 'type': label, 'value': value})
            except ValueError:
                continue  # skip bad lines

# Build DataFrame
df = pd.DataFrame(data)
# df = df.sort_values('time').reset_index(drop=True)

df = df.iloc[:300]
df['time'] = df['time'] - df['time'].iloc[0]

def compute_negToPos_zero_crossings(t, v):
    crossings = []
    crossings_index = []
    for i in range(len(v) - 1):
        if v[i] < 0 and v[i+1] >= 0:  # Positive-going
            # Linear interpolation to estimate crossing time
            slope = v[i+1] - v[i]
            if slope == 0:
                continue  # avoid divide by zero
            t_cross = t[i] - v[i] * (t[i+1] - t[i]) / slope
            crossings.append(t_cross)
            crossings_index.append(i) # no linear interpolation
            #####
            ##
    return [np.array(crossings), np.array(crossings_index)]

df = df[df['type'] == 'V']
df = df.reset_index(drop=True)
df = df.drop(columns='type')         # Remove 'type' column
df = df.rename(columns={'value': 'V'})  # Rename 'value' to 'V'

zc_times, zcs = compute_negToPos_zero_crossings(df['time'].to_numpy(), df['V'].to_numpy())

i = 0

last_cycle = pd.DataFrame([])

def generate_CT(last_cycle_df, cur_df, last_zc_time, cur_zc_time):

    # Extract one full voltage cycle
    last_cycle_times = last_cycle_df['time'].values
    last_cycle_values = last_cycle_df['V'].values

    # Normalize time to start from 0
    last_cycle_times = last_cycle_times - last_zc_time
    total_duration = last_cycle_times[-1]

    # Normalize the voltage waveform relative to peak
    v_peak = np.abs(last_cycle_values).max()
    if v_peak == 0:
        raise ValueError("v_peak is zero — cannot scale")

    normalized_V = last_cycle_values / v_peak  # -1 to +1 range

    # Interpolation function to get value at any time offset
    interp_fn = np.interp

    CT_output = []

    for _, row in cur_df.iterrows():
        timeD = row['time'] - cur_zc_time

        if 0 <= timeD <= total_duration:
            # Interpolate the waveform shape at this time offset
            v_norm = interp_fn(timeD, last_cycle_times, normalized_V)

            # Scale as needed — for example CT_peak output is 0.6V (adjust as needed)
            CT_amplitude = v_peak # peak current signal voltage
            ct_value = v_norm * CT_amplitude
        else:
            ct_value = 0.0  # outside one cycle

        CT_output.append(ct_value)

    # Add new column to current df
    cur_df = cur_df.copy()
    cur_df['I'] = CT_output
    return cur_df


while(1):
    if i > 3:
        break
    # print(f'--10cycleChunk--{i//10}')
    start, end = zcs[i], zcs[i+1] # samples before 1st zc, 11th zc
    # print(start + 1,end)
    tempdf = df.iloc[start + 1: end + 2]  #sample after 1st zc : sample after zc
    

    if last_cycle.empty == False:
        # print(zc_times[i - 1], zc_times[i])
        # print(last_cycle)
        # print(tempdf)
        ct_df = generate_CT(last_cycle, tempdf, zc_times[i - 1], zc_times[i])
        print(ct_df)

        # Plot voltage and CT current on the same plot
        plt.figure(figsize=(10, 4))

        plt.plot(ct_df['time'], ct_df['I'], label='Synthesized CT Current (I)', color='orange')
        if 'V' in ct_df.columns:
            plt.plot(ct_df['time'], ct_df['V'], label='Voltage (V)', color='blue', alpha=0.5)

        plt.xlabel('Time (µs)')
        plt.ylabel('Signal')
        plt.title('Simulated CT Output vs Voltage')
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.show()
        



    last_cycle = tempdf
    i += 1




# with pd.option_context('display.max_rows', None, 'display.max_columns', None):
#     print(df[df['type'] == 'V'])

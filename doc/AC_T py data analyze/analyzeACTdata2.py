import pandas as pd
import matplotlib.pyplot as plt

# Read the entire file line by line
rows = []
with open("F150_July25_calc1.csv") as f:
    for line in f:
        parts = [p.strip() for p in line.split(",")]
        rows.append(parts)

# Separate AC and CP rows
ac_rows = [r for r in rows if len(r) == 9 and r[1].strip() == "AC"]
cp_rows = [r for r in rows if len(r) == 6 and "cp_H_L_D_F" in r[1]]

# Create AC DataFrame
df_ac = pd.DataFrame(ac_rows, columns=[
    "Timestamp_ms", "Tag", "Vrms", "Irms", "RealPower",
    "SampleNum", "Duration_us", "PhaseAngle_deg_x10", "Frequency_Hz_x10"
])

# Convert columns to numeric
for col in df_ac.columns:
    if col not in ["Tag"]:
        df_ac[col] = pd.to_numeric(df_ac[col], errors='coerce')

# Create CP DataFrame
df_cp = pd.DataFrame(cp_rows, columns=[
    "Timestamp_ms", "Tag", "High_x100", "Low_x100", "Duty_x100", "Freq_x10"
])

for col in df_cp.columns:
    if col not in ["Tag"]:
        df_cp[col] = pd.to_numeric(df_cp[col], errors='coerce')

df_cp["Timestamp_ms"] = (df_cp["Timestamp_ms"] // 1000 ) 
df_cp["Timestamp_ms"] = df_cp["Timestamp_ms"]- df_cp["Timestamp_ms"][0]

df_ac["Timestamp_ms"] = df_ac["Timestamp_ms"]- df_ac["Timestamp_ms"][0]




def get_cp_changes(df_cp):
    """
    Return only the rows where any CP value changes.
    """
    # Ensure numeric columns are properly set
    for col in ["High_x100", "Low_x100", "Duty_x100", "Freq_x10"]:
        df_cp[col] = pd.to_numeric(df_cp[col], errors='coerce')

    # Detect changes in any CP column
    changes = (df_cp[["High_x100", "Low_x100", "Duty_x100", "Freq_x10"]]
               .shift(1) != df_cp[["High_x100", "Low_x100", "Duty_x100", "Freq_x10"]]) \
               .any(axis=1)

    # Always keep the first row
    changes.iloc[0] = True

    return df_cp[changes].reset_index(drop=True)


df_cp_changes = get_cp_changes(df_cp)
with pd.option_context('display.max_rows', None, 'display.max_columns', None):
    print(df_cp)
    print(df_cp_changes)


def plot_ac_cp_with_changes(df_ac, df_cp):
    # Get rows where CP changes
    df_cp_changes = get_cp_changes(df_cp)

    # Create figure
    fig, ax1 = plt.subplots(figsize=(12, 6))

    # Plot Real Power
    ax1.set_xlabel("Time (ms)")
    ax1.set_ylabel("Real Power (kW)", color="orange")
    ax1.plot(df_ac["Timestamp_ms"], df_ac["RealPower"] / 1000,
             color="orange", label="Real Power (kW)")
    ax1.tick_params(axis='y', labelcolor="orange")

    # Plot CP Duty
    ax2 = ax1.twinx()
    ax2.set_ylabel("CP Duty (%)", color="green")
    ax2.plot(df_cp["Timestamp_ms"], df_cp["Duty_x100"] / 100,
             color="green", label="CP Duty (%)")
    ax2.tick_params(axis='y', labelcolor="green")

    # Add vertical lines at CP changes
    for t in df_cp_changes["Timestamp_ms"]:
        ax1.axvline(x=t, color="blue", linestyle="--", alpha=0.4)

    # Legends
    lines, labels = [], []
    for ax in [ax1, ax2]:
        line, label = ax.get_legend_handles_labels()
        lines += line
        labels += label
    ax1.legend(lines, labels, loc="upper left")

    plt.title("AC Real Power & CP Duty with CP State Changes")
    plt.grid(True)
    plt.tight_layout()
    plt.show()


#plot_ac_cp_with_changes(df_ac, df_cp)




def plot_ac_cp(df_ac, df_cp):
    fig, ax1 = plt.subplots(figsize=(12, 6))

    # Plot Real Power on the left Y-axis
    ax1.set_xlabel("Time (ms)")
    ax1.set_ylabel("Real Power (kW)", color="orange")
    ax1.plot(df_ac["Timestamp_ms"], df_ac["RealPower"] / 1000,
             color="orange", label="Real Power (kW)")
    ax1.tick_params(axis='y', labelcolor="orange")

    # Add a second Y-axis for CP Duty
    ax2 = ax1.twinx()
    ax2.set_ylabel("CP Duty (%)", color="green")
    ax2.plot(df_cp["Timestamp_ms"], df_cp["Duty_x100"] / 100,
             color="green", label="CP Duty (%)")
    ax2.tick_params(axis='y', labelcolor="green")

    # Add CP High State as a line or shaded region
    ax3 = ax1.twinx()
    ax3.spines["right"].set_position(("outward", 60))
    ax3.set_ylabel("CP High", color="blue")
    ax3.plot(df_cp["Timestamp_ms"], df_cp["High_x100"],
             color="blue", linestyle="--", label="CP High")
    ax3.tick_params(axis='y', labelcolor="blue")

    # Combine legends
    lines, labels = [], []
    for ax in [ax1, ax2, ax3]:
        line, label = ax.get_legend_handles_labels()
        lines += line
        labels += label
    ax1.legend(lines, labels, loc="upper left")

    plt.title("AC Real Power, CP Duty, and CP High State")
    plt.grid(True)
    plt.tight_layout()
    plt.show()

plot_ac_cp(df_ac, df_cp)
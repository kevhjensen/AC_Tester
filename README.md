
# AC_Tester STM32 Project

CubeMX HAL C project for **AC_Tester** PCB

---

## 🔧 Hardware Overview

- **ADS1220** ADC with resistor divider:
  - For sampling AC mains voltage and waveform
  - Also measures CT signal via ADC1_2 and 1.65V to IAP/IAN/AGND
- **MCP4725** DAC:
  - Paired with op-amps to buffer and invert signal
  - Creates a differential "fake" bipolar output centered at 1.65V to emulate CT signal
- ADC and DAC have **isolated voltage planes**
- **J-1772** EV side hardware:
  - Option to connect to external EV (for CCS board / ISO15118 comms)
- **Voltage dividers** + STM32 ADC:
  - Reads EVSE-side CP continuously
  - Reads EV-side CP (post-diode) in State B and later

---

## 💻 Software Overview (see `/Core/Src/AC_T`)

- **ADS1220 DRDY interrupt-based reads:**
  - AC_V read at 1.93kHz (1 channel), or 866Hz (2 channels)
- **Drivers and Hardware Interface:**
  - ADS + MCP drivers, high-level init/read logic
  - J-1772 controller logic
- **AC Processor:**
  - Computes Vrms, Irms, Real Power, Energy, etc.
  - Analysis based on 10-cycle chunks
- **Control Pilot (CP) Signal Reader:**
  - 38.9kHz timer → triggers ADC via DMA every 25.6µs
  - Buffer processes every ~99.96ms (half full)
  - CP frequency/duty may show beat/harmonics if near 40kHz
  - Determines:
    - Actual CP state (with debouncing: needs 2+ cycles to switch)
    - Current limit in amps when in state C
    - `isPLC` (powerline communication detected)
  - Tolerant CP state detection: ±2% DC duty allowed per J-1772, +-1V for B, C... states, current code has +-1.5V so not undefined at 7.5V for example
  - **Hardware Accuracy Comparison:**

    | State | AC_T Read (V) | Scope Measured (V) |
    |-------|---------------|---------------------|
    | A     | 12.23         | 11.5                |
    | B     | 8.79          | 8.72                |
    | C     | 5.37          | 5.62                |
    | PWM Low | -13.47      | -11.61              |

    Frequency and DC is plenty accurate now

- **AC_Tester Modes:**
  - USB CDC communication via `/USB_DEVICE/App/usbd_cdc_if.c`
  - Works over any baud rate
  - Accepts serial commands:
    - Change CP state
    - Set data mode, sample rate
    - Responds with telemetry data

---

## 📝 To-Do / Roadmap

- [ ] **Add CP states:**
  - State 9: Unplug CP without going to state B2 first +6V (so +-12V pwm state for a few secs after unplug)
  - State 10: State C2 (go from A to C skipping B or C to B) +-12V, state 9 and 10 might be the same
- [ ] **Emulate CT signal**:
  - Save 1 AC waveform cycle, loop output via DAC
  - Use global setpoint for current amplitude and phase angle
- [ ] **Expand AC_Tester_modes:**
  - Full serial control interface (see Excel reference)
  - Periodic USB output (100–1000ms)
  - Optimize with ring buffer, check `USB != busy`
- [ ] **Vehicle Profile Emulation:**
  - Plug-in triggers B1
  - Wait for B2, then enter C
  - Ramp to target current
  - Handle unplug logic
- [ ] **Detect charger AC output (optocoupler):**
  - Validate AC relay output is live (vehicle actually getting AC power in)
- [ ] **Improve energy metering accuracy:**
  - Current code output vs AX80 (258Wh vs. 266.3Wh)
  - Sample AX80 via Modbus at 100ms intervals
  - Account for:
    - Time drift (~2s/day on STM32) insignificant
    - Int math truncation in uW/mV conversion
    - Sampling rate mismatch (AX80 = 8kHz, STM32 = 866Hz) if no high freq. noise should be fine
  - Verify ADS DRDY interrupt timing stability (numSamples is stable) and not impacted by CPU being busy 
  - Calibrate to Tesco/AX80 values
- [ ] **Integrate FOCCI/CLARA CCS Board:**
  - For ISO15118 / AutoCharge testing
- [ ] **Add CAN bus interface:**
  - More robust than USB CDC for long-term testing

---
# Handheld Laser Distance Meter - Project Summary

**Arduino Nano ESP32 with VL53L1X ToF Sensor**

---

## 📋 Project Overview

A portable handheld distance measurement device using:
- **Microcontroller:** Arduino Nano ESP32 (240MHz, dual-core)
- **Distance Sensor:** VL53L1X Time-of-Flight (4m range)
- **Display:** 0.96" 128×64 I²C OLED
- **Laser Pointer:** KY-008 (650nm red, 5mW)
- **Power:** 18650 Li-ion battery with TP4056 charging module
- **Control:** Single momentary push button (hold to measure)

**Ideal for:** Hobbyists, makers, quick distance measurements up to 4 meters

---

## 🎯 Operating Principle

### User Interaction (Hold-to-Measure)
1. **Button held down** → Device powers on (laser + display active)
2. **Continuous measurement** → Distance updates in real-time on OLED every ~50ms
3. **Button released** → Device powers off (laser off, screen blank)

### Display Output
- **Large text (size 3):** Distance in **meters** (e.g., "1.45m")
- **Smaller text (size 2):** Distance in **centimeters** directly below (e.g., "144.5cm")
- **Out of range:** Shows placeholder "-- . -- m" with message

---

## 📦 Hardware Components

### Core Hardware
| Component | Your Part | Spec | Status |
|-----------|-----------|------|--------|
| Microcontroller | Arduino Nano ESP32 | 32-bit, 240MHz, dual-core | ✓ Perfect |
| Distance Sensor | VL53L1X | Time-of-Flight, I²C, 4m range | ✓ Excellent |
| Display | SSD1306 OLED | 0.96" 128×64 pixels, I²C | ✓ Compatible |
| Laser Module | KY-008 | 650nm red, 5mW, PWM controllable | ✓ Compatible |
| Button | Momentary SPST | Single press/hold control | ✓ Perfect |
| Battery | 18650 Li-ion | 3.7V nominal, 2000-3000mAh | ✓ Good |
| Charging Circuit | TP4056 | Li-ion charger + protection | ✓ Professional |

### Support Components
- Breadboard (830 holes)
- Jumper wires (male/female)
- USB micro cable (for charging)
- Battery holder
- 100nF capacitors (I²C decoupling)
- 10µF capacitor (power smoothing)
- Resistors (pull-ups, current limiting)

---

## 🔌 Wiring Diagram & Pin Assignments

### GPIO Pin Connections

| GPIO Pin | Connected To | Component | Voltage | Function |
|----------|--------------|-----------|---------|----------|
| **GPIO21** | SDA (I²C) | VL53L1X, OLED | 3.3V | Shared I²C data line |
| **GPIO22** | SCL (I²C) | VL53L1X, OLED | 3.3V | Shared I²C clock line |
| **GPIO15** | Signal | KY-008 Laser | 3.3V | Laser on/off control |
| **GPIO4** | Button Input | Push Button | 3.3V | Measurement trigger |
| **VIN** | Power Rail | TP4056 output | 5V | Main power input |
| **GND** | Common Ground | All components | 0V | Ground return |

### Component Connections (Detailed)

#### VL53L1X Sensor
```
VCC  → 3.3V
GND  → GND
SDA  → GPIO21 (add 100nF cap to GND)
SCL  → GPIO22 (add 100nF cap to GND)
```

#### OLED Display (SSD1306)
```
VCC  → 3.3V
GND  → GND
SDA  → GPIO21 (shared I²C bus)
SCL  → GPIO22 (shared I²C bus)
```

#### Push Button
```
Leg 1 → GPIO4
Leg 2 → GND
(Internal pull-up enabled in code)
```

#### KY-008 Laser Module
```
VCC    → 5V (from TP4056 or boost converter)
GND    → GND
Signal → GPIO15 (PWM control, 3.3V logic)
```

#### Power Circuit
```
18650 Battery → TP4056 charging module
TP4056 OUT+ → Arduino VIN
TP4056 OUT- → Arduino GND
USB micro cable → TP4056 USB input (for charging)
```

---

## 💻 Software Architecture

### Libraries Required
- `Wire.h` (I²C communication - built-in)
- `Adafruit_VL53L1X.h` (distance sensor)
- `Adafruit_SSD1306.h` (OLED display)
- `Adafruit_GFX.h` (graphics library)

### Key Code Features

#### Hold-to-Measure Logic
```cpp
- Button held (LOW) → Device powers on, laser ON, sensor starts ranging
- Button released (HIGH) → Sensor stops, laser OFF, display blanks
- Debounce: 30ms for stable state detection
```

#### Continuous Measurement Loop
- While button held, the loop runs at ~20Hz (50ms timing budget)
- Each cycle reads the sensor, validates the measurement
- Updates OLED display with new distance in real-time

#### Display Update Cycle
```
Hold Button
  ↓
Laser ON, Sensor starts
  ↓
Wait for sensor data ready (~50ms)
  ↓
Read distance (mm)
  ↓
Update OLED:
  - Large meters display
  - Smaller cm display below
  ↓
Repeat until button released
  ↓
Release Button
  ↓
Laser OFF, Sensor stops, Screen blanks
```

---

## 📏 Specifications & Performance

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Measurement Range** | 30mm - 4000mm | Optimal 1-4 meters |
| **Accuracy** | ±2-3% | Depends on target reflectivity |
| **Update Rate** | ~20Hz | 50ms timing budget |
| **Field of View** | ~27° | Narrow beam, good accuracy |
| **I²C Speed** | 400 kHz | Standard mode |
| **Power Consumption (active)** | ~150-200mA | Laser + sensor + display |
| **Power Consumption (idle)** | <10mA | Display off, laser off |
| **Battery Runtime** | 8-12 hours | Continuous hold, 2000mAh battery |
| **Laser Power** | 5mW | Safe for enclosed operation |
| **Display Resolution** | 128×64 pixels | Clear distance readout |
| **Distance Units** | Meters & Centimeters | Displayed simultaneously |

---

## 🚀 Assembly & Testing

### Pre-Build Checklist
- [ ] Install Adafruit_VL53L1X library (Arduino IDE)
- [ ] Verify battery voltage: 3.6-4.2V
- [ ] Test TP4056 charging module with USB cable
- [ ] Verify boost converter or TP4056 output: 5.0V ±0.2V
- [ ] Check all jumper wires and connections
- [ ] Gather breadboard, capacitors, resistors

### Assembly Steps (1-1.5 hours)

**Step 1: Power Circuit (15-20 min)**
- Connect battery to TP4056 charging module
- Verify output voltage at 5V
- Connect Arduino VIN to TP4056 output, GND to common ground

**Step 2: Arduino Installation (5-10 min)**
- Place Arduino Nano ESP32 on breadboard center
- Connect 5V and GND rails
- Verify voltage with multimeter

**Step 3: I²C Sensors - Shared Bus (15-20 min)**
- Connect VL53L1X: VCC→3.3V, GND, SDA→GPIO21, SCL→GPIO22
- Add 100nF capacitor across VL53L1X power
- Connect OLED: VCC→3.3V, GND, SDA→GPIO21, SCL→GPIO22 (same pins!)
- Add 100nF capacitor across OLED power

**Step 4: Laser Module (5-10 min)**
- Connect KY-008: VCC→5V, GND, Signal→GPIO15
- Add 100nF capacitor across laser power

**Step 5: Button & Debouncing (5 min)**
- Connect push button: one leg→GPIO4, other leg→GND
- 10kΩ pull-up to 3.3V (or use internal pull-up in code)

**Step 6: Software & Testing (15-20 min)**
- Install required libraries
- Upload `laser_distance_meter_hold.ino`
- Test I²C communication (I²C scanner)
- Test button response
- Verify distance measurements at known distances

### Functional Tests

**Test 1: Button Response**
- [ ] Hold button → Laser turns on (red dot visible)
- [ ] Release button → Laser turns off
- [ ] Screen shows "Measuring..." during hold

**Test 2: I²C Communication**
- [ ] OLED displays text
- [ ] VL53L1X is detected at address 0x29
- [ ] No I²C conflicts or glitches

**Test 3: Distance Measurement**
- [ ] Point at white wall 1 meter away
- [ ] OLED shows ~1.00m (large) and ~100.0cm (small)
- [ ] Multiple holds at same distance show consistent readings

**Test 4: Range Limits**
- [ ] Closest: 30cm should read ~0.30m
- [ ] Furthest: 4m should read ~4.00m
- [ ] Beyond range shows "-- . -- m"

---

## 🔧 Troubleshooting Guide

| Issue | Likely Cause | Solution |
|-------|--------------|----------|
| VL53L1X not detected (0x29) | I²C connection loose | Check GPIO21/22 solder joints, verify 3.3V to sensor |
| OLED shows nothing | Wrong I²C address or power | Try 0x3D address in code, check 3.3V power |
| OLED shows garbage | Memory corruption | Reset Arduino, re-upload code |
| Unstable readings | Electrical noise | Add 100µF cap across power, shorter I²C wires |
| Laser won't turn on | GPIO15 not driving 3.3V | Test with multimeter, check transistor on laser module |
| Button not responding | GPIO4 not pulled up | Verify 10kΩ pull-up to 3.3V, test with Serial.println() |
| Battery won't charge | TP4056 not connected or broken | Check USB cable, verify LED indicator on module |
| Out of range every time | Target too dark or far | Use white surface, stay within 4m, clean sensor lens |

---

## 📊 Code Files Provided

### `laser_distance_meter_hold.ino`
- **Purpose:** Main firmware (current, hold-to-measure version)
- **Key Features:**
  - Hold button to power on and start measuring
  - Release button to power off and blank display
  - Real-time distance update (large meters, small cm)
  - 30ms debounce for stable button detection
  - Automatic out-of-range handling
  - I²C error checking on startup

### `parts_compatibility_analysis.md`
- Detailed parts verification
- Library changes and API differences
- Power circuit architecture
- Power budget calculations (8-12h runtime)

### `assembly_guide_YOUR_PARTS.md`
- Step-by-step assembly with photos
- Power circuit testing with multimeter
- I²C bus configuration
- Functional testing procedures
- Troubleshooting for specific components

### `Laser_Distance_Meter_Summary.pdf`
- Professional PDF summary (printable)
- All specs and wiring in one document
- Quick reference tables and checklist

---

## 🔋 Power Management

### Operating States

**Active (Button Held)**
- Laser: ON (50mA)
- Sensor: Ranging (20mA)
- OLED: Display active (25mA)
- Arduino: Running (100mA)
- **Total: ~195mA**

**Idle (Button Released)**
- Laser: OFF (0mA)
- Sensor: Stopped (0mA)
- OLED: Blank (0.5mA)
- Arduino: Running but low (80mA)
- **Total: ~80mA**

### Battery Runtime Estimates

| Scenario | Avg Current | Runtime (2000mAh) |
|----------|-------------|-------------------|
| Continuous hold | 195mA | ~10 hours |
| Mixed use (50% hold) | 135mA | ~15 hours |
| Intermittent (10% hold) | 96mA | ~20 hours |

---

## 🎯 Next Steps After Working Prototype

### 1. **Calibration**
   - Test against 5 known distances (30cm, 60cm, 1m, 2m, 4m)
   - Add software offset if readings are consistently off
   - Document calibration curve if needed

### 2. **Mechanical Design**
   - 3D-print or fabricate enclosure matching your design
   - Mount VL53L1X at front center
   - Position KY-008 laser coaxially with sensor
   - Mount OLED on top for viewing angle
   - Place button on side for thumb operation
   - Access port for USB charging

### 3. **Perfboard Assembly**
   - Solder components to perfboard for durability
   - Remove breadboard dependencies
   - Compact final form factor

### 4. **Power Optimization**
   - Implement actual deep sleep (hardware power-off) if needed
   - Use physical on/off switch on TP4056 or add MOSFET load switch
   - Monitor battery voltage with ADC for low-battery warning

### 5. **Feature Additions**
   - Min/max distance tracking during hold
   - Average distance calculation
   - Unit selection (m/cm/feet/inches)
   - Data logging to EEPROM
   - WiFi connectivity for remote logging
   - Temperature compensation for accuracy

---

## 🚨 Safety & Best Practices

### Laser Safety
- ⚠️ Never point laser at eyes or animals
- Use proper warning labels on enclosure
- Ensure laser is contained in case
- Comply with local laser safety regulations (typically Class 3R for 5mW)

### Electrical Safety
- ⚠️ Check battery polarity before connecting
- Use TP4056 protection module to prevent over-discharge
- Do not exceed component voltage ratings
- Never leave LiPo battery unattended while charging
- Use proper fuse on battery lead (500mA recommended)

### Measurement Accuracy
- Best results on matte white surfaces (Lambertian reflection)
- Avoid shiny black surfaces (poor reflection)
- Keep sensor lens clean
- Use at room temperature for consistent accuracy
- Calibrate against known distance if ±3% accuracy critical

---

## 📚 Resources & References

- **VL53L1X Datasheet:** https://www.st.com/resource/en/datasheet/vl53l1x.pdf
- **Arduino Nano ESP32 Docs:** https://docs.arduino.cc/hardware/nano-esp32
- **Adafruit VL53L1X Library:** https://github.com/adafruit/Adafruit_VL53L1X
- **GitHub Project:** https://github.com/Hirunthakan432/Laser-Distance-Meter-Nano-ESP32

---

## ✅ Project Status

| Component | Status | Notes |
|-----------|--------|-------|
| Hardware | ✓ Ready | All parts verified |
| Wiring | ✓ Complete | GPIO assignments finalized |
| Firmware | ✓ Ready | Hold-to-measure implemented |
| Testing | ⏳ Pending | Awaiting assembly |
| Enclosure | ⏳ Design | Ready for 3D printing |
| Documentation | ✓ Complete | Full guides provided |

---

## 📝 Revision History

- **v1.0 (Current):** Hold-to-measure behavior, real-time continuous measurement, dual display (meters/cm)
- **v0.9 (Previous):** Single-shot measurement per button press
- **v0.8:** Original guide compatibility with two-button control

---

## 📞 Support & Troubleshooting

### Quick Diagnosis
1. **No power:** Check battery voltage (multimeter), verify TP4056 OUT=5V
2. **No OLED:** Check I²C address (0x3C vs 0x3D), verify SDA/SCL
3. **Sensor not found:** Check I²C address 0x29, verify 3.3V power
4. **Laser won't turn on:** Test GPIO15 output, check laser module power
5. **Unstable readings:** Add capacitors, keep I²C wires short

### Getting Help
- Check serial monitor output (115200 baud) for debug messages
- Run I²C scanner to find all devices on bus
- Test each component separately before integration
- Refer to component datasheets for pinout verification

---

**Built with ❤️ for makers and hobbyists**

*Last Updated: August 2026*

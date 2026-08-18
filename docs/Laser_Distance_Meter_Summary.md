# Handheld Laser Distance Meter - Project Summary

**ESP32-WROOM-32 with VL53L1X ToF Sensor**

---

## 📋 Project Overview

A portable handheld distance measurement device using:
- **Microcontroller:** ESP32-WROOM-32 (dual-core Xtensa LX6 @ 240 MHz)
- **Distance Sensor:** VL53L1X Time-of-Flight (4 m range)
- **Display:** 0.96" 128×64 I²C OLED (SSD1306)
- **Laser Pointer:** KY-008 (650 nm red, ~5 mW)
- **Power:** 18650 Li-ion battery with TP4056 charging module
- **Control:** Single momentary push button (hold to measure)

**Ideal for:** Hobbyists, makers, quick distance measurements up to 4 meters

---

## 🎯 Operating Principle

### User Interaction (Hold-to-Measure)
1. **Button held down** → Device powers on (laser + display active)
2. **Continuous measurement** → Distance updates in real-time on OLED every ~50 ms
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
| Microcontroller | ESP32-WROOM-32 | 32-bit dual-core, 240 MHz | ✓ Perfect |
| Distance Sensor | VL53L1X | Time-of-Flight, I²C, 4 m range | ✓ Excellent |
| Display | SSD1306 OLED | 0.96" 128×64 pixels, I²C | ✓ Compatible |
| Laser Module | KY-008 | 650 nm red, ~5 mW | ✓ Compatible |
| Button | Momentary SPST | Single press/hold control | ✓ Perfect |
| Battery | 18650 Li-ion | 3.7 V nominal | ✓ Good |
| Charging Circuit | TP4056 | Li-ion charger + protection | ✓ Professional |

---

## 🔌 Wiring Diagram & Pin Assignments (ESP32-WROOM-32)

### GPIO Pin Connections

| GPIO Pin | Connected To | Component | Voltage | Function |
|----------|--------------|-----------|---------|----------|
| **GPIO21** | SDA (I²C) | VL53L1X, OLED | 3.3 V | Shared I²C data line |
| **GPIO22** | SCL (I²C) | VL53L1X, OLED | 3.3 V | Shared I²C clock line |
| **GPIO15** | Signal | KY-008 Laser | 3.3 V | Laser on/off control |
| **GPIO4** | Button Input | Push Button | 3.3 V | Measurement trigger |
| **VIN** | Power Rail | TP4056 output | 5 V | Main power input |
| **GND** | Common Ground | All components | 0 V | Ground return |

### Component Connections (Detailed)

#### VL53L1X Sensor
```
VCC  → 3.3V
GND  → GND
SDA  → GPIO21
SCL  → GPIO22
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
VCC    → 5V (from TP4056)
GND    → GND
Signal → GPIO15
```

#### Power Circuit
```
18650 Battery → TP4056 charging module
TP4056 OUT+ → ESP32 VIN
TP4056 OUT- → ESP32 GND
```

---

## 💻 Software

### Libraries Required
- `Wire.h` (built-in)
- `Adafruit_VL53L1X.h`
- `Adafruit_SSD1306.h`
- `Adafruit_GFX.h`

### Board Selection
- Arduino IDE → Tools → Board → **ESP32 Dev Module**

### Key Behavior
- Hold button (LOW) → Laser ON, sensor ranging, display active
- Release button (HIGH) → Laser OFF, sensor stopped, display blank
- 30 ms debounce
- 50 ms timing budget on VL53L1X

---

## 📏 Specifications

| Parameter | Value |
|-----------|-------|
| Measurement Range | ~30 mm – 4000 mm |
| Accuracy | ±2–3% typical |
| Update Rate | ~20 Hz (50 ms budget) |
| I²C Speed | 400 kHz |
| Display | Meters (large) + Centimeters (smaller) |

---

## 🚨 Safety

- Never point the aiming laser at eyes, animals, or vehicles.
- Use only low-power (Class 2 or lower) red laser modules.
- VL53L1X IR laser is Class 1 (eye-safe).
- Observe proper Li-ion battery charging practices with the TP4056.

---

## 📚 Resources

- **VL53L1X Datasheet:** https://www.st.com/resource/en/datasheet/vl53l1x.pdf
- **ESP32-WROOM-32:** Espressif documentation
- **GitHub Project:** https://github.com/Hirunthakan432/Laser-Distance-Meter-Nano-ESP32

---

**Updated for ESP32-WROOM-32 module**  
*Last Updated: August 2026*

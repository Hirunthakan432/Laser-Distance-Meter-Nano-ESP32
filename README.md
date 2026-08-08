# Handheld Arduino Nano ESP32 Laser Distance Meter

A compact, handheld laser distance meter built with the **Arduino Nano ESP32**, **VL53L1X** Time-of-Flight sensor, 0.96" OLED display, press-and-hold measurement button, and a low-power aiming laser.

**Project title:** Handheld Arduino Nano ESP32 Laser Distance Meter with Press-and-Hold Measurement

## Features

- Press-and-hold measurement (device only measures while the button is held)
- Real-time distance display on OLED (mm / cm)
- Separate low-power red laser for aiming (not used for measurement)
- VL53L1X ToF sensor for accurate laser ranging (up to ~4 m)
- Battery powered with on/off switch
- Simple one-handed operation
- Low power standby when button is released

## Core Components

| Component                  | Purpose                          |
|---------------------------|----------------------------------|
| Arduino Nano ESP32        | Main controller                  |
| VL53L1X ToF sensor        | Distance measurement             |
| 0.96" I²C OLED (SSD1306)  | Displays distance and status     |
| Momentary push button     | Press-and-hold measurement control |
| Low-power laser module    | Aiming reference                 |
| Li-ion / LiPo battery     | Portable power                   |
| Charging / power circuit  | Battery charging & regulation    |
| On/off switch             | Main power control               |

## How It Works

1. **Power ON** → Device enters standby (OLED and laser off)
2. **Press & hold button** → OLED turns on, aiming laser turns on, VL53L1X starts continuous measurement
3. **Distance displayed** live on the OLED while button is held
4. **Release button** → Measurement stops, laser off, OLED off → back to standby

> **Important:** The red laser is only for aiming. The VL53L1X performs the actual Time-of-Flight distance measurement using its own invisible IR laser (Class 1 eye-safe).

## Wiring

### I²C Bus (shared)
| Device     | Pin  | Nano ESP32     |
|------------|------|----------------|
| VL53L1X    | VCC  | 3.3 V          |
| VL53L1X    | GND  | GND            |
| VL53L1X    | SDA  | GPIO21 (A4)    |
| VL53L1X    | SCL  | GPIO22 (A5)    |
| VL53L1X    | XSHUT| GPIO4 (optional)|
| OLED       | VCC  | 3.3 V          |
| OLED       | GND  | GND            |
| OLED       | SDA  | GPIO21         |
| OLED       | SCL  | GPIO22         |

### GPIO
| Device          | Pin       | Nano ESP32 |
|-----------------|-----------|------------|
| Push Button     | One side  | GPIO15     |
| Push Button     | Other side| GND        |
| Aiming Laser    | Enable    | GPIO5 (via MOSFET) |

**Aiming Laser drive:** Use an N-MOSFET (e.g. 2N7002) so the ESP32 does not source the laser current.

## Software

### Required Libraries
- `VL53L1X` by Pololu (or SparkFun VL53L1X)
- `Adafruit SSD1306`
- `Adafruit GFX`

Install via Arduino Library Manager.

### Board Setup
1. Install ESP32 board support in Arduino IDE (Boards Manager → search "esp32" by Espressif Systems)
2. Select board: **Arduino Nano ESP32**
3. Upload the sketch from the `src/` folder

## Project Structure

```
Laser-Distance-Meter-Nano-ESP32/
├── README.md
├── src/
│   └── LaserDistanceMeter.ino   # Main Arduino sketch
├── docs/
│   └── Project-Summary.md       # Full project brief
└── LICENSE
```

## Development Phases

1. Controller setup (Nano ESP32)
2. VL53L1X distance sensor
3. OLED display
4. Press-and-hold button logic
5. Aiming laser control
6. Battery + power circuit
7. Enclosure
8. Testing & calibration

## Safety

- Never point the aiming laser at eyes, aircraft, or vehicles.
- Use only a low-power (Class 2 or lower) red laser module.
- The VL53L1X IR laser is Class 1 (eye-safe).

## License

MIT License – feel free to use, modify, and share.

---

**Author:** Hirunthakan432  
**Created with:** Arduino Nano ESP32 + VL53L1X  

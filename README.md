# Handheld ESP32-WROOM-32 Laser Distance Meter

A compact, handheld laser distance meter built with the **ESP32-WROOM-32** module, **VL53L1X** Time-of-Flight sensor, 0.96" OLED display, press-and-hold measurement button, and a low-power aiming laser.

**Project title:** Handheld ESP32-WROOM-32 Laser Distance Meter with Press-and-Hold Measurement

## Features

- Press-and-hold measurement (device only measures while the button is held)
- Real-time distance display on OLED (meters + centimeters)
- Separate low-power red laser for aiming (not used for measurement)
- VL53L1X ToF sensor for accurate laser ranging (up to ~4 m)
- Battery powered with charging support (TP4056)
- Simple one-handed operation
- Low power standby when button is released

## Core Components

| Component                  | Purpose                          |
|---------------------------|----------------------------------|
| ESP32-WROOM-32 module     | Main controller (dual-core 240 MHz) |
| VL53L1X ToF sensor        | Distance measurement             |
| 0.96" I²C OLED (SSD1306)  | Displays distance and status     |
| Momentary push button     | Press-and-hold measurement control |
| Low-power laser module (KY-008) | Aiming reference            |
| Li-ion / LiPo battery     | Portable power                   |
| TP4056 charging module    | Battery charging & protection    |
| On/off switch (optional)  | Main power control               |

## How It Works

1. **Power ON** → Device enters standby (OLED and laser off)
2. **Press & hold button** → OLED turns on, aiming laser turns on, VL53L1X starts continuous measurement
3. **Distance displayed** live on the OLED while button is held
4. **Release button** → Measurement stops, laser off, OLED off → back to standby

> **Important:** The red laser is only for aiming. The VL53L1X performs the actual Time-of-Flight distance measurement using its own invisible IR laser (Class 1 eye-safe).

## Wiring (ESP32-WROOM-32)

### I²C Bus (shared)
| Device     | Pin  | ESP32-WROOM-32 |
|------------|------|----------------|
| VL53L1X    | VCC  | 3.3 V          |
| VL53L1X    | GND  | GND            |
| VL53L1X    | SDA  | **GPIO21**     |
| VL53L1X    | SCL  | **GPIO22**     |
| OLED       | VCC  | 3.3 V          |
| OLED       | GND  | GND            |
| OLED       | SDA  | **GPIO21**     |
| OLED       | SCL  | **GPIO22**     |

### GPIO
| Device          | Pin       | ESP32-WROOM-32 |
|-----------------|-----------|----------------|
| Push Button     | One side  | **GPIO4**      |
| Push Button     | Other side| GND            |
| Aiming Laser    | Signal    | **GPIO15**     |
| Aiming Laser    | VCC       | 5 V            |
| Aiming Laser    | GND       | GND            |

**Note:** GPIO15 is a strapping pin but works well as an output after boot. Use an N-MOSFET (e.g. 2N7002) if you want to drive higher laser current safely.

### Power
- Battery → TP4056 → ESP32 **VIN** (5 V)
- Common GND

## Software

### Required Libraries
- `Adafruit_VL53L1X`
- `Adafruit_SSD1306`
- `Adafruit_GFX`
- `Wire` (built-in)

Install via Arduino Library Manager.

### Board Setup
1. Install ESP32 board support in Arduino IDE (Boards Manager → search "esp32" by Espressif Systems)
2. Select board: **ESP32 Dev Module** (works with ESP32-WROOM-32)
3. Upload the sketch from `Code/code.ino`

## Project Structure

```
Laser-Distance-Meter-Nano-ESP32/
├── README.md
├── Code/
│   └── code.ino                 # Main Arduino sketch (ESP32-WROOM-32)
├── docs/
│   └── Laser_Distance_Meter_Summary.md
└── LICENSE
```

## Pin Summary (Current)

| Function     | GPIO   |
|--------------|--------|
| I2C SDA      | 21     |
| I2C SCL      | 22     |
| Button       | 4      |
| Laser        | 15     |

## Safety

- Never point the aiming laser at eyes, aircraft, or vehicles.
- Use only a low-power (Class 2 or lower) red laser module.
- The VL53L1X IR laser is Class 1 (eye-safe).

## License

MIT License – feel free to use, modify, and share.

---

**Author:** Hirunthakan432  
**Updated for:** ESP32-WROOM-32 module  

/*
 * Handheld Arduino Nano ESP32 Laser Distance Meter
 * ------------------------------------------------
 * Features:
 *  - Press-and-hold measurement
 *  - VL53L1X Time-of-Flight sensor
 *  - 0.96" SSD1306 OLED display
 *  - Low-power aiming laser (GPIO controlled)
 *
 * Libraries required:
 *  - VL53L1X by Pololu
 *  - Adafruit SSD1306
 *  - Adafruit GFX
 *
 * Board: Arduino Nano ESP32
 */

#include <Wire.h>
#include <VL53L1X.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -------------------- Pin Definitions --------------------
const int BTN_PIN   = 15;   // Momentary push button (INPUT_PULLUP)
const int LASER_PIN = 5;    // Aiming laser enable (HIGH = ON)
const int XSHUT_PIN = 4;    // Optional VL53L1X shutdown pin

// -------------------- Display --------------------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------------------- Sensor --------------------
VL53L1X sensor;

// -------------------- State --------------------
bool measuring = false;
uint16_t lastDistance = 0;   // mm

void setup() {
  Serial.begin(115200);
  delay(100);

  // Pins
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LASER_PIN, OUTPUT);
  pinMode(XSHUT_PIN, OUTPUT);

  digitalWrite(LASER_PIN, LOW);     // Laser off
  digitalWrite(XSHUT_PIN, HIGH);    // Sensor awake

  // I2C
  Wire.begin(21, 22);               // SDA = 21, SCL = 22 (Nano ESP32 defaults)

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Laser Distance"));
  display.println(F("Meter"));
  display.println();
  display.println(F("Ready..."));
  display.display();
  delay(1500);

  // Turn OLED off for standby (blank screen)
  display.clearDisplay();
  display.display();

  // VL53L1X
  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.println(F("Failed to detect VL53L1X"));
    while (1);
  }

  // Long range mode (up to ~4 m)
  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(50000);  // 50 ms
  sensor.startContinuous(50);

  Serial.println(F("Laser Distance Meter ready"));
}

void loop() {
  bool btnPressed = (digitalRead(BTN_PIN) == LOW);

  // ---------- Button just pressed ----------
  if (btnPressed && !measuring) {
    measuring = true;
    digitalWrite(LASER_PIN, HIGH);          // Aiming laser ON

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("Measuring..."));
    display.display();
  }

  // ---------- Button just released ----------
  if (!btnPressed && measuring) {
    measuring = false;
    digitalWrite(LASER_PIN, LOW);           // Aiming laser OFF

    // Blank the OLED (standby)
    display.clearDisplay();
    display.display();
  }

  // ---------- Continuous measurement while held ----------
  if (measuring) {
    uint16_t distance = sensor.read();      // distance in mm

    if (!sensor.timeoutOccurred()) {
      lastDistance = distance;
    }

    // Update display
    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("LASER DISTANCE"));

    display.setTextSize(2);
    display.setCursor(0, 18);

    if (lastDistance == 0 || lastDistance > 4000) {
      display.println(F("---"));
    } else {
      // Show in cm with one decimal
      float cm = lastDistance / 10.0;
      display.print(cm, 1);
      display.println(F(" cm"));
    }

    display.setTextSize(1);
    display.setCursor(0, 48);
    display.print(lastDistance);
    display.println(F(" mm"));

    display.display();
  }

  delay(30);   // small delay for stability
}

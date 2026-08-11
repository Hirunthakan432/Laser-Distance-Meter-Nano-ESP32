/*
 * Handheld Laser Distance Meter
 * Arduino Nano ESP32 + VL53L1X + SSD1306 OLED + KY-008 Laser
 *
 * Wiring (from build guide):
 *   VL53L1X   VCC->3.3V  GND->GND  SDA->GPIO21  SCL->GPIO22
 *   OLED      VCC->3.3V  GND->GND  SDA->GPIO21  SCL->GPIO22
 *   Button    one leg->GPIO4  other leg->GND (internal pull-up)
 *   KY-008    VCC->5V    GND->GND  Signal->GPIO15
 *   Power     Battery -> TP4056 -> Arduino VIN
 *
 * Behavior:
 *   - HOLD button: device "powers on" (laser + display + sensor active)
 *     and measures continuously the whole time it's held.
 *   - RELEASE button: measuring stops and device "powers off"
 *     (laser off, screen blanked).
 *   - Display: distance in METERS shown large, CENTIMETERS shown
 *     smaller directly below it.
 */

#include <Wire.h>
#include <Adafruit_VL53L1X.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// ==================== PIN DEFINITIONS ====================
#define SDA_PIN      21
#define SCL_PIN      22
#define BUTTON_PIN   4
#define LASER_PIN    15

// ==================== DISPLAY CONFIG ====================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C   // try 0x3D if this doesn't work

// ==================== SENSOR CONFIG ====================
#define VL53L1X_ADDRESS 0x29

// ==================== DEBOUNCE ====================
#define DEBOUNCE_DELAY 30    // ms

// ==================== OBJECTS ====================
Adafruit_VL53L1X sensor = Adafruit_VL53L1X();
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==================== STATE ====================
bool rawButtonState   = HIGH;  // instantaneous read
bool debouncedState    = HIGH;  // debounced (stable) state
bool lastDebouncedState = HIGH;
unsigned long lastDebounceTime = 0;

bool devicePoweredOn = false;   // true while button is held & device active

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Handheld Laser Distance Meter ===");

  // Pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);   // laser off by default

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found at 0x3C, trying 0x3D...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("ERROR: OLED not found. Halting.");
      while (1) delay(10);
    }
  }
  Serial.println("OLED OK");

  // VL53L1X
  if (!sensor.begin(VL53L1X_ADDRESS, &Wire)) {
    Serial.print("ERROR: VL53L1X not found: ");
    Serial.println(sensor.vl_status);
    displayMessage("Sensor error!", "Check wiring.");
    while (1) delay(10);
  }
  Serial.println("VL53L1X OK");
  sensor.setTimingBudget(50);  // ms, balance of speed vs accuracy

  Serial.println("Setup complete. Hold button to measure.");
  goToSleepScreen();
}

// ==================== MAIN LOOP ====================
void loop() {
  updateButtonState();

  // Just pressed -> power on
  if (debouncedState == LOW && lastDebouncedState == HIGH) {
    powerOn();
  }

  // Just released -> power off
  if (debouncedState == HIGH && lastDebouncedState == LOW) {
    powerOff();
  }

  // While held, keep measuring and updating the display
  if (devicePoweredOn) {
    runMeasurementCycle();
  }

  lastDebouncedState = debouncedState;
}

// ==================== BUTTON DEBOUNCE ====================
void updateButtonState() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != rawButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    debouncedState = reading;
  }

  rawButtonState = reading;
}

// ==================== POWER CONTROL ====================
void powerOn() {
  Serial.println("Button held - powering ON, starting measurement");
  devicePoweredOn = true;

  digitalWrite(LASER_PIN, HIGH);   // laser on

  if (!sensor.startRanging()) {
    Serial.print("ERROR: couldn't start ranging: ");
    Serial.println(sensor.vl_status);
  }

  displayMessage("Measuring...", "");
}

void powerOff() {
  Serial.println("Button released - stopping measurement, powering OFF");
  devicePoweredOn = false;

  digitalWrite(LASER_PIN, LOW);    // laser off
  sensor.stopRanging();

  goToSleepScreen();
}

// ==================== MEASUREMENT ====================
void runMeasurementCycle() {
  if (sensor.dataReady()) {
    int16_t distanceMM = sensor.distance();
    sensor.clearInterrupt();       // triggers next measurement

    if (distanceMM < 0) {
      Serial.println("No valid target");
      showOutOfRange();
    } else {
      Serial.print("Distance: ");
      Serial.print(distanceMM);
      Serial.println(" mm");
      showDistance(distanceMM);
    }
  }
}

// ==================== DISPLAY ====================

// Big meters reading with smaller centimeters below it
void showDistance(int16_t distanceMM) {
  float meters = distanceMM / 1000.0;
  float centimeters = distanceMM / 10.0;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // ---- Meters: large text ----
  display.setTextSize(3);
  char meterStr[8];
  snprintf(meterStr, sizeof(meterStr), "%.2fm", meters);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(meterStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 8);
  display.print(meterStr);

  // ---- Centimeters: smaller text, below ----
  display.setTextSize(2);
  char cmStr[10];
  snprintf(cmStr, sizeof(cmStr), "%.1fcm", centimeters);
  display.getTextBounds(cmStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 40);
  display.print(cmStr);

  display.display();
}

void showOutOfRange() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.println("-- . -- m");
  display.setTextSize(1);
  display.setCursor(10, 45);
  display.println("Out of range / no target");
  display.display();
}

void displayMessage(const char* line1, const char* line2) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(line1);
  if (strlen(line2) > 0) {
    display.println(line2);
  }
  display.display();
}

// Blank screen while device is "off" (button not held)
void goToSleepScreen() {
  display.clearDisplay();
  display.display();
}
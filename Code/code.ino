/*
 * Handheld Laser Distance Meter
 * ESP32-WROOM-32 + VL53L1X + SSD1306 OLED + KY-008 Laser
 *
 * UPDATED FOR ESP32-WROOM-32 MODULE - Key Changes:
 *   • I2C pins: GPIO21 (SDA), GPIO22 (SCL) - standard ESP32 defaults
 *   • Dual-core Xtensa LX6 @ 240 MHz
 *   • Classic WROOM-32 form factor (larger than C3 Super Mini)
 *   • 3.3V logic, 5V input capable (VIN)
 *
 * Wiring:
 *   VL53L1X   VCC->3.3V  GND->GND  SDA->GPIO21  SCL->GPIO22
 *   OLED      VCC->3.3V  GND->GND  SDA->GPIO21  SCL->GPIO22
 *   Button    one leg->GPIO4  other leg->GND
 *   KY-008    VCC->5V    GND->GND  Signal->GPIO15
 *   Power     Battery -> TP4056 -> ESP32 VIN (5V)
 *
 * Behavior:
 *   - HOLD button: device powers on (laser + display + sensor active)
 *     and measures continuously
 *   - RELEASE button: measuring stops and device powers off
 *     (laser off, screen blank)
 *   - Display: distance in METERS shown large, CENTIMETERS smaller below
 */

#include <Wire.h>
#include <Adafruit_VL53L1X.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// ==================== PIN DEFINITIONS (ESP32-WROOM-32) ====================
#define SDA_PIN      21  // GPIO21 (SDA) - standard ESP32 default
#define SCL_PIN      22  // GPIO22 (SCL) - standard ESP32 default
#define BUTTON_PIN   4   // GPIO4 for button input
#define LASER_PIN    15  // GPIO15 for laser control

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
  Serial.println("\n\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║  Handheld Laser Distance Meter - ESP32-WROOM-32       ║");
  Serial.println("╚════════════════════════════════════════════════════════╝\n");

  // Pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);   // laser off by default

  Serial.println("✓ GPIO pins initialized");

  // I2C with standard ESP32-WROOM-32 pins
  Wire.begin(SDA_PIN, SCL_PIN);  // SDA=GPIO21, SCL=GPIO22
  Wire.setClock(400000);
  delay(100);
  Serial.print("✓ I2C bus initialized (SDA=GPIO");
  Serial.print(SDA_PIN);
  Serial.print(", SCL=GPIO");
  Serial.print(SCL_PIN);
  Serial.println(")");

  // OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found at 0x3C, trying 0x3D...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("✗ ERROR: OLED display not found at any address");
      Serial.println("  Addresses tried: 0x3C, 0x3D");
      displayMessage("OLED Error", "Check I2C wiring");
      while(1) delay(10);
    }
    Serial.println("✓ OLED found at 0x3D");
  } else {
    Serial.println("✓ OLED found at 0x3C");
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Distance Meter");
  display.println("Initializing...");
  display.display();

  // VL53L1X Distance Sensor
  if (!sensor.begin(VL53L1X_ADDRESS, &Wire)) {
    Serial.print("✗ ERROR: VL53L1X not found at address 0x");
    Serial.println(VL53L1X_ADDRESS, HEX);
    Serial.println("  Status: ");
    Serial.println(sensor.vl_status);
    displayMessage("Sensor Error", "Check I2C wiring");
    while(1) delay(10);
  }
  Serial.println("✓ VL53L1X sensor initialized (address: 0x29)");

  if (!sensor.startRanging()) {
    Serial.print("✗ ERROR: couldn't start ranging. Status: ");
    Serial.println(sensor.vl_status);
    displayMessage("Sensor Error", "Ranging failed");
    while(1) delay(10);
  }

  // Timing budget: 50ms balances speed vs accuracy
  sensor.setTimingBudget(50);
  Serial.println("✓ VL53L1X timing budget set to 50ms");

  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║  Setup Complete - Ready to Measure                   ║");
  Serial.println("║  Hold button to start measuring                      ║");
  Serial.println("║  Release button to stop                              ║");
  Serial.println("╚════════════════════════════════════════════════════════╝\n");

  goToSleepScreen();
}

// ==================== MAIN LOOP ====================
void loop() {
  updateButtonState();

  // Just pressed (LOW) -> power on
  if (debouncedState == LOW && lastDebouncedState == HIGH) {
    powerOn();
  }

  // Just released (HIGH) -> power off
  if (debouncedState == HIGH && lastDebouncedState == LOW) {
    powerOff();
  }

  // While held, keep measuring and updating the display
  if (devicePoweredOn) {
    runMeasurementCycle();
  }

  lastDebouncedState = debouncedState;
  delay(5);  // Small delay to prevent watchdog issues
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
  Serial.println("\n→ Button held - POWERING ON");
  Serial.println("  • Laser ON");
  Serial.println("  • Sensor ranging started");
  Serial.println("  • Display active");

  devicePoweredOn = true;

  // Laser on
  digitalWrite(LASER_PIN, HIGH);
  Serial.println("  ✓ Laser: ON (GPIO15 = HIGH)");

  // Start sensor
  if (!sensor.startRanging()) {
    Serial.print("  ✗ WARNING: couldn't start ranging: ");
    Serial.println(sensor.vl_status);
  }

  displayMessage("Measuring...", "Hold button");
}

void powerOff() {
  Serial.println("\n→ Button released - POWERING OFF");
  Serial.println("  • Sensor ranging stopped");
  Serial.println("  • Laser OFF");
  Serial.println("  • Display blanked");

  devicePoweredOn = false;

  // Stop sensor
  sensor.stopRanging();

  // Laser off
  digitalWrite(LASER_PIN, LOW);
  Serial.println("  ✓ Laser: OFF (GPIO15 = LOW)");

  goToSleepScreen();
}

// ==================== MEASUREMENT ====================
void runMeasurementCycle() {
  // Check if a measurement is ready
  if (sensor.dataReady()) {
    int16_t distanceMM = sensor.distance();
    sensor.clearInterrupt();

    if (distanceMM < 0 || distanceMM > 4000) {
      // Out of range
      Serial.println("  [Sensor] Out of range / invalid reading");
      showOutOfRange();
    } else {
      // Valid measurement
      showDistance(distanceMM);
    }
  }

  // Small delay to prevent hammer-reading
  delay(10);
}

// ==================== DISPLAY ====================

// Big meters reading with smaller centimeters below it
void showDistance(int16_t distanceMM) {
  float meters = distanceMM / 1000.0;
  float centimeters = distanceMM / 10.0;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // ---- Meters: large text (size 3) ----
  display.setTextSize(3);
  char meterStr[8];
  snprintf(meterStr, sizeof(meterStr), "%.2fm", meters);
  
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(meterStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 8);
  display.print(meterStr);

  // ---- Centimeters: smaller text (size 2), below ----
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
  display.println("Out of range");
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

// ==================== DEBUG: Print ESP32-WROOM-32 Board Info ====================
void printBoardInfo() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║            ESP32-WROOM-32 Module Info                 ║");
  Serial.println("╠════════════════════════════════════════════════════════╣");
  Serial.print("║ Chip Model: ESP32 (WROOM-32)");
  Serial.println("                          ║");
  Serial.print("║ CPU Cores: 2 (dual-core, 240MHz)");
  Serial.println("                       ║");
  Serial.print("║ Flash Memory: ");
  Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
  Serial.println("MB");
  Serial.println("║ SDA Pin: GPIO21");
  Serial.println("║ SCL Pin: GPIO22");
  Serial.println("║ I2C Speed: 400kHz");
  Serial.println("║ Laser Control: GPIO15 (PWM capable)");
  Serial.println("║ Button Input: GPIO4 (with pull-up)");
  Serial.println("╚════════════════════════════════════════════════════════╝\n");
}

/*
 * ==================== BOARD MIGRATION NOTES ====================
 * 
 * Changes for ESP32-WROOM-32:
 * 
 * 1. I2C PINS (CRITICAL!):
 *    • ESP32-C3 Super Mini: GPIO21 (SDA), GPIO20 (SCL)
 *    • ESP32-WROOM-32:     GPIO21 (SDA), GPIO22 (SCL) ← UPDATED
 *    Updated in code: Wire.begin(21, 22)
 * 
 * 2. Processing:
 *    • ESP32-WROOM-32: Dual-core Xtensa LX6 @ 240 MHz
 *    • More performance headroom than ESP32-C3
 * 
 * 3. Form Factor:
 *    • Classic WROOM-32 module (or DevKit boards based on it)
 *    • Larger than C3 Super Mini but very common and well-supported
 * 
 * 4. Board Selection in Arduino IDE:
 *    Tools → Board → ESP32 Arduino → "ESP32 Dev Module"
 *    (or "ESP32-WROOM-32" if available in your package)
 * 
 * 5. GPIO:
 *    GPIO4 and GPIO15 are safe and commonly used on WROOM-32 boards.
 *    Avoid strapping pins if possible (GPIO0, 2, 12, 15 can affect boot).
 *    GPIO15 is used here for laser; it is a strapping pin but works fine
 *    for output after boot.
 * 
 * ==================== INSTALLATION INSTRUCTIONS ====================
 * 
 * 1. Arduino IDE Board Support:
 *    Tools → Board Manager → Search "esp32"
 *    Install "esp32" by Espressif Systems
 *    Select Tools → Board → "ESP32 Dev Module"
 * 
 * 2. Libraries:
 *    • Adafruit_VL53L1X
 *    • Adafruit_SSD1306
 *    • Adafruit_GFX
 *    • Wire (built-in)
 * 
 * 3. Upload:
 *    • Select correct COM port
 *    • Board: ESP32 Dev Module
 *    • Click Upload
 * 
 * ==================== IF I2C DOESN'T WORK ====================
 * 
 * Standard ESP32-WROOM-32 I2C:
 *   SDA = GPIO21, SCL = GPIO22
 * 
 * If sensors not detected:
 * 1. Check physical connections with multimeter
 * 2. Run I2C scanner to find available devices
 * 3. Ensure 3.3V power to sensors (not 5V)
 * 4. Add 4.7k–10k pull-up resistors on SDA/SCL if missing
 * 5. Keep I2C wires short
 * 
 */
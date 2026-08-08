/*
 * Arduino Nano ESP32 Handheld Laser Distance Meter (UPDATED)
 * 
 * HARDWARE CONFIGURATION:
 * - Arduino Nano ESP32
 * - VL53L1X Time-of-Flight sensor (upgraded from VL53L0X)
 * - 0.96" 128×64 I²C OLED display
 * - KY-008 laser module (5mW red, 650nm)
 * - 1x momentary push button (single button, dual function)
 * - 1-cell Li-ion/LiPo battery with charging circuit
 * - 5V boost converter from 3.7V battery
 * 
 * BUTTON FUNCTION:
 * - Short press (<500ms): Cycle measurement mode
 * - Long press (>500ms): Toggle power on/off
 * 
 * MEASUREMENT MODES:
 * - IDLE: Device on but not measuring (low power)
 * - CONTINUOUS: Rapid continuous measurements
 * - SINGLE_SHOT: One measurement per button press
 */

#include <Wire.h>
#include <Adafruit_VL53L1X.h>  // ← CHANGED from VL53L0X
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// ==================== CONFIGURATION ====================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Pin Definitions
#define LASER_PIN 2              // PWM laser control
#define BUTTON_MODE 4            // Single button: mode + power
#define LED_POWER 13             // Optional: power indicator
#define BUZZER_PIN 12            // Optional: buzzer

// Sensor Settings
#define VL53L1X_ADDRESS 0x29     // Standard I2C address
#define MEASUREMENT_INTERVAL 50  // ms between measurements
#define DISTANCE_SAMPLES 5       // Moving average sample count

// Display & Timing
#define DISPLAY_UPDATE_INTERVAL 500  // ms
#define BUTTON_DEBOUNCE 50           // ms debounce time
#define LONG_PRESS_THRESHOLD 500     // ms for long press detection

// ==================== OBJECTS ====================

Adafruit_VL53L1X sensor;  // ← CHANGED from 'lox'
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==================== ENUMERATIONS ====================

enum Mode { IDLE, CONTINUOUS, SINGLE_SHOT };
enum PowerState { OFF, ON };

// ==================== STATE VARIABLES ====================

Mode currentMode = IDLE;
PowerState powerState = OFF;
bool laserEnabled = false;

// Sensor & Measurement Data
uint16_t lastDistance = 0;
uint16_t filteredDistance = 0;
uint8_t measurementCount = 0;
uint32_t distanceSum = 0;
bool sensorReady = false;

// Timing
unsigned long lastDisplayUpdate = 0;
unsigned long lastMeasurement = 0;
unsigned long buttonPressStart = 0;
bool buttonPressed = false;
bool buttonWasPressed = false;

// Battery monitoring (optional)
uint16_t batteryVoltage = 0;
uint8_t batteryPercent = 100;

// ==================== SETUP ====================

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n╔════════════════════════════════════════════════════════════╗");
  Serial.println("║   Arduino Nano ESP32 Laser Distance Meter (VL53L1X)     ║");
  Serial.println("╚════════════════════════════════════════════════════════════╝\n");

  // Initialize GPIO pins
  pinMode(LASER_PIN, OUTPUT);
  pinMode(BUTTON_MODE, INPUT_PULLUP);
  pinMode(LED_POWER, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Start with everything off
  digitalWrite(LASER_PIN, LOW);
  digitalWrite(LED_POWER, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("✓ GPIO pins initialized");

  // Initialize I2C Bus
  Wire.begin(21, 22);  // SDA=GPIO21, SCL=GPIO22
  Wire.setClock(400000);  // 400kHz I2C speed
  delay(100);
  Serial.println("✓ I2C bus initialized");

  // Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    // Try alternate address
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("✗ ERROR: OLED display initialization failed!");
      Serial.println("  Checked addresses: 0x3C and 0x3D");
      displayError("OLED Failed");
      while(1);
    }
    Serial.println("✓ OLED display found at 0x3D");
  } else {
    Serial.println("✓ OLED display found at 0x3C");
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();
  delay(500);

  // Initialize VL53L1X Sensor
  // ← CHANGED: VL53L1X initialization
  if (!sensor.begin(VL53L1X_ADDRESS)) {
    Serial.println("✗ ERROR: VL53L1X sensor initialization failed!");
    Serial.print("  I2C Address: 0x");
    Serial.println(VL53L1X_ADDRESS, HEX);
    displayError("Sensor Failed");
    while(1);
  }
  Serial.println("✓ VL53L1X sensor initialized");

  // Configure sensor for optimal performance
  // VL53L1X supports timing budgets from 20ms to 200ms
  sensor.setTimingBudget(50);  // 50ms timing budget (balance speed vs accuracy)
  Serial.print("  Timing Budget: 50ms");
  
  // Optional: Set inter-measurement period
  sensor.setInterMeasurementPeriod(100);  // 100ms between measurements
  Serial.println("  Inter-measurement period: 100ms");

  // Start initial measurement
  sensor.startRanging();
  sensorReady = true;

  Serial.println("\n✓ All systems initialized - Ready to measure!");
  Serial.println("\nButton Functions:");
  Serial.println("  • Short press (<500ms): Cycle measurement mode");
  Serial.println("  • Long press (>500ms): Toggle power on/off");
  Serial.println("\nAwait user action...\n");

  // Display ready screen
  powerState = OFF;
  displayReadyScreen();
}

// ==================== MAIN LOOP ====================

void loop() {
  // Handle single button input
  handleButton();

  // Update laser state based on mode and power
  updateLaserState();

  // Perform measurements if device is powered and active
  if (powerState == ON && currentMode != IDLE) {
    if (millis() - lastMeasurement >= MEASUREMENT_INTERVAL) {
      performMeasurement();
      lastMeasurement = millis();
    }
  }

  // Update display periodically
  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  delay(10);  // Prevent watchdog timeout
}

// ==================== BUTTON HANDLING ====================

void handleButton() {
  // Read button state (LOW when pressed, HIGH when released)
  bool buttonState = (digitalRead(BUTTON_MODE) == LOW);

  // Detect button press (HIGH → LOW transition)
  if (buttonState && !buttonPressed) {
    buttonPressed = true;
    buttonPressStart = millis();
    buttonWasPressed = true;
    Serial.println("[Button] Pressed...");
  }

  // Detect button release (LOW → HIGH transition)
  if (!buttonState && buttonPressed) {
    buttonPressed = false;
    unsigned long pressDuration = millis() - buttonPressStart;

    Serial.print("[Button] Released after ");
    Serial.print(pressDuration);
    Serial.println("ms");

    // Determine action based on press duration
    if (pressDuration > LONG_PRESS_THRESHOLD) {
      // Long press: Toggle power
      Serial.println("→ Action: Long press detected - Toggling power");
      togglePower();
      beep(200);
      beep(200);
    } else {
      // Short press: Cycle mode
      Serial.println("→ Action: Short press detected - Cycling mode");
      if (powerState == ON) {
        cycleMode();
        beep(100);
      } else {
        beep(50);
        beep(50);
        Serial.println("  (Device off - press long to power on)");
      }
    }
  }
}

void togglePower() {
  powerState = (powerState == ON) ? OFF : ON;

  if (powerState == ON) {
    digitalWrite(LED_POWER, HIGH);
    currentMode = IDLE;
    Serial.println("✓ Device powered ON");
  } else {
    digitalWrite(LED_POWER, LOW);
    digitalWrite(LASER_PIN, LOW);
    laserEnabled = false;
    currentMode = IDLE;
    filteredDistance = 0;
    measurementCount = 0;
    distanceSum = 0;
    Serial.println("✓ Device powered OFF");
  }
}

void cycleMode() {
  if (powerState != ON) return;

  if (currentMode == IDLE) {
    currentMode = CONTINUOUS;
    sensor.startRanging();  // Start continuous measurements
    Serial.println("→ Mode: CONTINUOUS");
  } else if (currentMode == CONTINUOUS) {
    currentMode = SINGLE_SHOT;
    Serial.println("→ Mode: SINGLE SHOT (press button to measure)");
  } else {
    currentMode = IDLE;
    Serial.println("→ Mode: IDLE");
  }

  // Reset measurement buffer
  measurementCount = 0;
  distanceSum = 0;
  filteredDistance = 0;
}

// ==================== LASER CONTROL ====================

void updateLaserState() {
  bool shouldBeOn = (powerState == ON && currentMode != IDLE);

  if (shouldBeOn && !laserEnabled) {
    digitalWrite(LASER_PIN, HIGH);
    laserEnabled = true;
    Serial.println("[Laser] Turned ON");
  } else if (!shouldBeOn && laserEnabled) {
    digitalWrite(LASER_PIN, LOW);
    laserEnabled = false;
    Serial.println("[Laser] Turned OFF");
  }
}

// ==================== MEASUREMENT ====================

void performMeasurement() {
  // ← CHANGED: VL53L1X measurement API is different
  
  if (!sensor.isRangeReady()) {
    // Measurement not ready yet, try again
    return;
  }

  // Get distance in millimeters
  uint16_t distance = sensor.getDistance();

  // Start next measurement
  sensor.startRanging();

  // Filter out invalid readings (out of range: 0 or > 4000mm for VL53L1X)
  if (distance == 0 || distance > 4000) {
    if (distance == 0) {
      Serial.println("[Sensor] Out of range (no valid reading)");
    } else {
      Serial.println("[Sensor] Out of range (too far)");
    }
    return;
  }

  // Accumulate for moving average
  distanceSum += distance;
  measurementCount++;

  // Calculate and filter average
  if (measurementCount >= DISTANCE_SAMPLES) {
    filteredDistance = distanceSum / DISTANCE_SAMPLES;
    measurementCount = 0;
    distanceSum = 0;

    Serial.print("[Measurement] Distance: ");
    Serial.print(filteredDistance);
    Serial.println(" mm");

    // Feedback sounds
    if (currentMode == SINGLE_SHOT) {
      beep(100);
    } else if (currentMode == CONTINUOUS) {
      beep(20);  // Subtle feedback
    }
  }
}

// ==================== DISPLAY ====================

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (powerState == OFF) {
    displayPowerOffScreen();
  } else if (currentMode == IDLE) {
    displayIdleScreen();
  } else {
    displayMeasurementScreen();
  }

  display.display();
}

void displayPowerOffScreen() {
  display.setTextSize(2);
  display.setCursor(15, 20);
  display.println("POWER OFF");
  display.setTextSize(1);
  display.setCursor(10, 45);
  display.println("Press button to");
  display.setCursor(20, 55);
  display.println("power on");
}

void displayIdleScreen() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("╔═ DISTANCE METER ═╗");
  display.println("║                  ║");
  display.println("║  Status: READY   ║");
  display.println("║                  ║");
  display.println("║  Short Press:    ║");
  display.println("║    Start Measure ║");
  display.println("║                  ║");
  display.println("║  Long Press:     ║");
  display.println("║    Power Off     ║");
  display.println("╚══════════════════╝");
}

void displayMeasurementScreen() {
  // Title bar
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("╔═ DISTANCE METER ═╗");

  // Mode indicator
  display.setCursor(0, 10);
  display.print("║ Mode: ");
  if (currentMode == CONTINUOUS) {
    display.println("CONTINUOUS    ║");
  } else if (currentMode == SINGLE_SHOT) {
    display.println("SINGLE SHOT   ║");
  } else {
    display.println("IDLE          ║");
  }

  // Distance display (large)
  display.setTextSize(2);
  display.setCursor(5, 22);
  
  if (filteredDistance > 0) {
    display.print(filteredDistance / 10);  // Convert mm to cm
    display.println(" cm");
  } else {
    display.println("---- cm");
  }

  // Secondary unit (meters)
  display.setTextSize(1);
  display.setCursor(5, 42);
  display.print("Meters: ");
  if (filteredDistance > 0) {
    display.print(filteredDistance / 1000.0, 2);
  } else {
    display.print("0.00");
  }
  display.println(" m");

  // Status footer
  display.setCursor(0, 54);
  display.println("║ Laser: ON        ║");
  display.setCursor(0, 62);
  
  if (currentMode == SINGLE_SHOT) {
    display.println("╚ Press to measure╝");
  } else {
    display.println("╚ Continuous mode ╝");
  }
}

void displayError(const char* errorMsg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 25);
  display.println("ERROR");
  display.setCursor(0, 35);
  display.println(errorMsg);
  display.display();
}

void displayReadyScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("╔═ DISTANCE METER ═╗");
  display.println("║                  ║");
  display.println("║  Status: READY   ║");
  display.println("║                  ║");
  display.println("║  Short Press:    ║");
  display.println("║    Start Measure ║");
  display.println("║                  ║");
  display.println("║  Long Press:     ║");
  display.println("║    Power Off     ║");
  display.println("╚══════════════════╝");
  display.display();
}

// ==================== UTILITY FUNCTIONS ====================

void beep(uint16_t duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

// ==================== SENSOR DIAGNOSTICS ====================

void printSensorInfo() {
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║      VL53L1X Sensor Information      ║");
  Serial.println("╠═══════════════════════════════════════╣");
  Serial.print("║ I2C Address: 0x");
  Serial.println(VL53L1X_ADDRESS, HEX);
  Serial.println("║ Sensor Type: VL53L1X (ToF)");
  Serial.println("║ Max Range: 4000 mm (4 m)");
  Serial.println("║ I2C Speed: 400 kHz");
  Serial.println("║ Measurement Mode: Continuous");
  Serial.println("╚═══════════════════════════════════════╝\n");
}

// ==================== OPTIONAL: CALIBRATION SUPPORT ====================

/*
 * CALIBRATION PROCEDURE:
 * 
 * 1. Test against known distances (30cm, 60cm, 100cm, 200cm)
 * 2. Record measured vs actual values
 * 3. Calculate offset: offset = measured - actual
 * 4. Apply correction in performMeasurement():
 *
 *    const int DISTANCE_OFFSET = -5;  // in mm (example: -5mm correction)
 *    filteredDistance = (distanceSum / DISTANCE_SAMPLES) + DISTANCE_OFFSET;
 *
 * VL53L1X Typical Accuracy: ±20mm (±2%) with white target
 */

/*
 * POWER OPTIMIZATION TIPS:
 * 
 * • Reduce measurement frequency: Increase MEASUREMENT_INTERVAL
 * • Use SINGLE_SHOT mode: Only measure when button pressed
 * • Disable buzzer/LED: Comment out beep() and digitalWrite(LED_POWER)
 * • Lower display update rate: Increase DISPLAY_UPDATE_INTERVAL
 * • Implement deep sleep: Put ESP32 in light sleep between measurements
 * 
 * Expected battery life:
 * • Continuous mode: 8-10 hours (200mA average)
 * • Single shot mode: 12-15 hours (lower average draw)
 * • With deep sleep: 24+ hours possible
 */

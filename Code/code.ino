/*
 * Handheld Laser Distance Meter
 * Arduino Nano ESP32 + VL53L1X + SSD1306 OLED + KY-008 Laser
 *
 * Matches wiring from build guide:
 *   VL53L1X   VCC->3.3V  GND->GND  SDA->GPIO21  SCL->GPIO22
 *   OLED      VCC->3.3V  GND->GND  SDA->GPIO21  SCL->GPIO22
 *   Button    one leg->GPIO4  other leg->GND (internal pull-up)
 *   KY-008    VCC->5V    GND->GND  Signal->GPIO15
 *   Power     Battery -> TP4056 -> Arduino VIN
 *
 * Behavior:
 *   - Idle screen shown on OLED, waiting for button press
 *   - On button press: laser turns ON, sensor takes a measurement,
 *     distance is shown on OLED, laser turns back OFF
 *   - Debounced single-shot measurement (no continuous mode)
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
#define DEBOUNCE_DELAY 50    // ms

// ==================== OBJECTS ====================
Adafruit_VL53L1X sensor = Adafruit_VL53L1X();
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==================== STATE ====================
bool lastButtonState = HIGH;   // not pressed (pull-up)
unsigned long lastDebounceTime = 0;

int16_t lastDistanceMM = -1;   // -1 = no reading yet
bool sensorOK = false;

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
    Serial.println("ERROR: OLED not found at 0x3C, trying 0x3D...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("ERROR: OLED not found. Halting.");
      while (1) delay(10);
    }
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Distance Meter");
  display.println("Initializing...");
  display.display();
  Serial.println("OLED OK");

  // VL53L1X
  if (!sensor.begin(VL53L1X_ADDRESS, &Wire)) {
    Serial.print("ERROR: VL53L1X not found: ");
    Serial.println(sensor.vl_status);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Sensor error!");
    display.println("Check wiring.");
    display.display();
    while (1) delay(10);
  }
  Serial.println("VL53L1X OK");

  if (!sensor.startRanging()) {
    Serial.print("ERROR: couldn't start ranging: ");
    Serial.println(sensor.vl_status);
    while (1) delay(10);
  }
  sensor.setTimingBudget(50);  // ms, balance of speed vs accuracy
  sensorOK = true;

  Serial.println("Setup complete. Press button to measure.");
  showIdleScreen();
}

// ==================== MAIN LOOP ====================
void loop() {
  bool reading = digitalRead(BUTTON_PIN);

  // Detect a debounced press (HIGH -> LOW transition)
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading == LOW && lastButtonState == HIGH) {
      // Confirmed new press
      takeMeasurement();
    }
  }

  lastButtonState = reading;
  delay(10);
}

// ==================== MEASUREMENT ====================
void takeMeasurement() {
  Serial.println("Button pressed - measuring...");

  // Laser on during measurement
  digitalWrite(LASER_PIN, HIGH);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Measuring...");
  display.display();

  // Wait for a fresh sample
  int16_t distance = -1;
  unsigned long startWait = millis();
  while (!sensor.dataReady()) {
    if (millis() - startWait > 500) break;  // timeout safeguard
    delay(5);
  }

  if (sensor.dataReady()) {
    distance = sensor.distance();
    sensor.clearInterrupt();
  }

  // Laser off after measurement
  digitalWrite(LASER_PIN, LOW);

  if (distance < 0) {
    Serial.println("Measurement failed / out of range");
    showResult(-1);
  } else {
    lastDistanceMM = distance;
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");
    showResult(distance);
  }
}

// ==================== DISPLAY ====================
void showIdleScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Distance Meter");
  display.println("--------------------");
  display.println("");
  display.println("Press button to");
  display.println("take a measurement");

  if (lastDistanceMM >= 0) {
    display.println("");
    display.print("Last: ");
    display.print(lastDistanceMM / 10.0, 1);
    display.println(" cm");
  }
  display.display();
}

void showResult(int16_t distanceMM) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Distance Meter");
  display.println("--------------------");

  if (distanceMM < 0) {
    display.setCursor(0, 25);
    display.println("Out of range");
    display.println("or no target");
  } else {
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.print(distanceMM / 10.0, 1);
    display.println(" cm");

    display.setTextSize(1);
    display.setCursor(0, 45);
    display.print(distanceMM / 1000.0, 2);
    display.println(" m");
  }

  display.setCursor(0, 56);
  display.println("Press to re-measure");
  display.display();
}
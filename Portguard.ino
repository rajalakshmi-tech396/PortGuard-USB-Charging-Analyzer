
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal_I2C.h>

// -------- Components --------
Adafruit_INA219 ina219;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------- Pins --------
const int BUTTON_PIN = 2;
const int GREEN_LED = 5;
const int RED_LED = 6;
const int BUZZER = 7;

// -------- Thresholds --------
const float LOW_VOLTAGE_LIMIT = 4.50;
const float HIGH_VOLTAGE_LIMIT = 5.10;
const float LOW_CURRENT_LIMIT = 3.0;    // mA
const float HIGH_CURRENT_LIMIT = 30.0;  // mA
const float INTERRUPTED_LIMIT = 1.0;    // mA

// -------- Conditions --------
enum ConditionCode {
  STABLE,
  UNSTABLE,
  LOW_VOLT,
  LOW_CURRENT,
  HIGH_VOLT,
  HIGH_CURRENT,
  INTERRUPTED
};

const char* conditionNames[] = {
  "STABLE", "UNSTABLE", "LOW VOLTAGE",
  "LOW CURRENT", "HIGH VOLTAGE",
  "HIGH CURRENT", "INTERRUPTED"
};

const char* advice[] = {
  "Normal",
  "Check stability",
  "Check power source",
  "Check cable/load",
  "Check supply now",
  "Check load current",
  "Check connection"
};

const int healthScores[] = {100, 70, 40, 60, 30, 50, 0};

// -------- Demo values --------
// These are simulated examples, NOT sensor measurements.
const float demoVoltage[] = {
  4.58, 4.20, 4.20, 4.60, 5.20, 4.80, 0.00
};

const float demoCurrent[] = {
  5.2, 15.0, 10.0, 1.5, 10.0, 40.0, 0.0
};

// -------- Variables --------
bool demoMode = false;
int demoIndex = 0;

ConditionCode detectedCondition = STABLE;

float voltage = 0.0;
float current_mA = 0.0;
float previousVoltage = 0.0;

unsigned long lastReadTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastPulseTime = 0;
unsigned long lowCurrentStart = 0;

const unsigned long readInterval = 500;
const unsigned long displayInterval = 2000;
const unsigned long pulseInterval = 500;
const unsigned long longPressTime = 1000;
const unsigned long debounceDelay = 40;

bool adviceScreen = false;
bool greenPulseState = false;
bool redPulseState = false;

int lastButtonReading = HIGH;
int stableButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long pressStart = 0;
bool longPressHandled = false;

// -------- Outputs OFF --------
void allOff() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);

  greenPulseState = false;
  redPulseState = false;
}

// -------- Actual sensor reading --------
void readSensor() {
  voltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();

  if (current_mA < 0) {
    current_mA = 0;
  }

  // DEMO values must not be overwritten by sensor readings.
  if (demoMode) {
    return;
  }

  if (current_mA < INTERRUPTED_LIMIT) {
    if (lowCurrentStart == 0) {
      lowCurrentStart = millis();
    }
  } else {
    lowCurrentStart = 0;
  }

  // First matching condition wins.
  if (lowCurrentStart != 0 &&
      millis() - lowCurrentStart >= 2000) {

    detectedCondition = INTERRUPTED;

  } else if (voltage < LOW_VOLTAGE_LIMIT) {

    detectedCondition = LOW_VOLT;

  } else if (voltage > HIGH_VOLTAGE_LIMIT) {

    detectedCondition = HIGH_VOLT;

  } else if (previousVoltage > 0 &&
             abs(voltage - previousVoltage) >= 0.15) {

    detectedCondition = UNSTABLE;

  } else if (current_mA < LOW_CURRENT_LIMIT) {

    detectedCondition = LOW_CURRENT;

  } else if (current_mA > HIGH_CURRENT_LIMIT) {

    detectedCondition = HIGH_CURRENT;

  } else {

    detectedCondition = STABLE;
  }

  previousVoltage = voltage;
}

// -------- Load selected demo values --------
void loadDemoCondition() {
  detectedCondition = (ConditionCode)demoIndex;
  voltage = demoVoltage[demoIndex];
  current_mA = demoCurrent[demoIndex];
}

// -------- LCD --------
void updateLCD() {
  lcd.clear();

  if (adviceScreen) {
    lcd.setCursor(0, 0);
    lcd.print("Health: ");
    lcd.print(healthScores[detectedCondition]);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print(advice[detectedCondition]);

  } else {
    lcd.setCursor(0, 0);

    if (demoMode) {
      lcd.print("DEMO ");
    } else {
      lcd.print("AUTO ");
    }

    lcd.print(conditionNames[detectedCondition]);

    lcd.setCursor(0, 1);
    lcd.print("V:");
    lcd.print(voltage, 2);
    lcd.print(" I:");
    lcd.print(current_mA, 1);
    lcd.print("mA");
  }
}

// -------- Serial Monitor --------
void printSerial() {
  Serial.println();
  Serial.println("===== PORTGUARD =====");

  Serial.print("Mode: ");
  Serial.println(demoMode ? "DEMO (SIMULATED)" : "AUTO (SENSOR)");

  Serial.print("Voltage: ");
  Serial.print(voltage, 2);
  Serial.println(" V");

  Serial.print("Current: ");
  Serial.print(current_mA, 2);
  Serial.println(" mA");

  Serial.print("Condition: ");
  Serial.println(conditionNames[detectedCondition]);

  Serial.print("Health Score: ");
  Serial.print(healthScores[detectedCondition]);
  Serial.println("%");

  Serial.print("Advice: ");
  Serial.println(advice[detectedCondition]);

  Serial.println("====================");
}

// -------- LEDs and buzzer --------
void updateIndicators() {
  unsigned long now = millis();

  if (now - lastPulseTime >= pulseInterval) {
    lastPulseTime = now;

    greenPulseState = !greenPulseState;
    redPulseState = !redPulseState;
  }

  if (detectedCondition == STABLE) {
    digitalWrite(GREEN_LED, greenPulseState);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);

  } else {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, redPulseState);

    if (detectedCondition == HIGH_VOLT ||
        detectedCondition == HIGH_CURRENT ||
        detectedCondition == INTERRUPTED) {
      digitalWrite(BUZZER, HIGH);
    } else {
      digitalWrite(BUZZER, LOW);
    }
  }
}

// -------- Button handling --------
void checkButton() {
  int reading = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  if (reading != lastButtonReading) {
    lastDebounceTime = now;
  }

  if (now - lastDebounceTime >= debounceDelay) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      if (stableButtonState == LOW) {
        pressStart = now;
        longPressHandled = false;

      } else {
        // Short press: next condition in DEMO.
        if (!longPressHandled && demoMode) {
          demoIndex = (demoIndex + 1) % 7;
          loadDemoCondition();

          adviceScreen = false;
          lastDisplayTime = now;
          lastPulseTime = now;

          allOff();
          updateLCD();
          printSerial();
        }
      }
    }
  }

  // Long press: switch mode.
  if (stableButtonState == LOW &&
      !longPressHandled &&
      now - pressStart >= longPressTime) {

    longPressHandled = true;
    demoMode = !demoMode;

    allOff();
    lowCurrentStart = 0;
    previousVoltage = 0;
    adviceScreen = false;
    lastDisplayTime = now;
    lastPulseTime = now;

    if (demoMode) {
      demoIndex = 0;
      loadDemoCondition();
    } else {
      readSensor();
    }

    updateLCD();
    printSerial();
  }

  lastButtonReading = reading;
}

// -------- Setup --------
void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  allOff();

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("PORTGUARD");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  if (!ina219.begin()) {
    lcd.clear();
    lcd.print("INA219 ERROR");
    Serial.println("INA219 not detected!");

    while (true) {
      allOff();
      delay(100);
    }
  }

  delay(1000);

  readSensor();
  updateLCD();
  printSerial();
}

// -------- Main loop --------
void loop() {
  checkButton();

  unsigned long now = millis();

  if (!demoMode && now - lastReadTime >= readInterval) {
    lastReadTime = now;

    readSensor();
    printSerial();
  }

  // Alternate LCD measurement and advice screens.
  if (now - lastDisplayTime >= displayInterval) {
    lastDisplayTime = now;
    adviceScreen = !adviceScreen;

    if (demoMode) {
      loadDemoCondition();
    }

    updateLCD();
  }

  updateIndicators();
}

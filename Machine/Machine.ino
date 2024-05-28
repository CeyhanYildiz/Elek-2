// TFT
#include <Arduino_GFX_Library.h>
#include "FreeMono8pt7b.h"
#include "FreeSansBold10pt7b.h"
#include "FreeSerifBoldItalic12pt7b.h"

// Sensors on Nano
#include <Arduino_HTS221.h> // humidity, and temperature sensor: to get highly accurate measurements of the environmental conditions
#include <Arduino_LPS22HB.h> // barometric sensor: you could make a simple weather station
#include <Arduino_APDS9960.h> // gesture, proximity, light color and light intensity sensor : estimate the room’s luminosity, but also whether someone is moving close to the board
#include <Arduino_LSM9DS1.h> // 9 axis inertial sensor: what makes this board ideal for wearable devices

// Define button pins
#define Button_1 D1 // Page up
#define Button_2 D2 // Page down

#define PIN_D6 D6  // Infrared sensor (5V)
#define PIN_D5 D5  // 12V sensor with NPN transistor
#define PIN_D4 D4  // 12V sensor with NPN transistor
#define PIN_D3 D3  // 12V sensor with PNP transistor

#define TFT_LED   D7
#define TFT_DC    D8
#define TFT_RESET D9
#define TFT_CS    D10
#define TFT_MOSI  D11
#define TFT_SCK   D13

//#define TFT_MISO  -1  // not used for TFT

#define GFX_BL TFT_LED // backlight pin

Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS);
Arduino_GFX *display = new Arduino_ST7735(bus, TFT_RESET, 3);

int currentPage = 1; // Variable to store the current page

float temperature = 0;
float humidity = 0;

float pressure = 0;

int proximity = 0;
int r = 0, g = 0, b = 0;

unsigned long lastUpdate = 0;

float x = 0, y = 0, z = 0; // Variables to store IMU data

int calibrationSamples = 100; // Define calibrationSamples

float xOffset = 0, yOffset = 0, zOffset = 0; // Define offsets

  int sensorValueD6 = digitalRead(PIN_D6);
  int sensorValueD5 = digitalRead(PIN_D5);
  int sensorValueD4 = digitalRead(PIN_D4);
  int sensorValueD3 = digitalRead(PIN_D3);

void setup() {

  // Initialize buttons
  pinMode(Button_1, INPUT_PULLUP);
  pinMode(Button_2, INPUT_PULLUP);

  pinMode(PIN_D6, INPUT);
  pinMode(PIN_D5, INPUT);
  pinMode(PIN_D4, INPUT);
  pinMode(PIN_D3, INPUT);

  Serial.begin(9600);
  Serial.println("Arduino_GFX Hello World Gfxfont example");

  if (!display->begin()) {
    Serial.println("gfx->begin() failed!");
  }

  display->fillScreen(BLACK);
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  pinMode(Button_1, INPUT_PULLUP); // Set Button_1 as input with internal pull-up resistor
  pinMode(Button_2, INPUT_PULLUP); // Set Button_2 as input with internal pull-up resistor

  // Adjust backlight brightness (0-255)
  int brightness = 255; // Example brightness level
  analogWrite(GFX_BL, brightness);

  // Initialize the HTS221 sensor
  if (!HTS.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }

  // Initialize the LPS22HB sensor
  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1);
  }

  // Initialize the APDS9960 sensor
  if (!APDS.begin()) {
    Serial.println("Error initializing APDS9960 sensor.");
    while (1);
  }

  // Initialize the LSM9DS1 IMU sensor
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  Serial.print("Magnetic field sample rate = ");
  Serial.print(IMU.magneticFieldSampleRate());
  Serial.println(" uT");
  Serial.println();
  Serial.println("Magnetic Field in uT");
  Serial.println("X\tY\tZ");

  calibrateMagneticField();
}


void loop() {
  // Read sensor values
  temperature = HTS.readTemperature();
  humidity = HTS.readHumidity();
  pressure = BARO.readPressure();
  readSensorValues(); // Read APDS9960 sensor values
  readMagneticFieldData(); // Read LSM9DS1 sensor values

  // Print sensor values
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity = ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Pressure = ");
  Serial.print(pressure);
  Serial.println(" kPa");

  Serial.print("Proximity = ");
  Serial.print(proximity);
  Serial.print(" | RGB = ");
  Serial.print(r);
  Serial.print(",");
  Serial.print(g);
  Serial.print(",");
  Serial.println(b);

  Serial.print("IMU: X = ");
  Serial.print(x);
  Serial.print(", Y = ");
  Serial.print(y);
  Serial.print(", Z = ");
  Serial.println(z);

  // Print the sensor values to the Serial Monitor
  Serial.print("D6 (Infrared): ");
  Serial.println(sensorValueD6);
  
  Serial.print("D5 (12V with NPN): ");
  Serial.println(sensorValueD5);
  
  Serial.print("D4 (12V with NPN): ");
  Serial.println(sensorValueD4);
  
  Serial.print("D3 (12V with PNP): ");
  Serial.println(sensorValueD3);

  // Print an empty line
  Serial.println();

  // Check if a gesture reading is available
  if (APDS.gestureAvailable()) {
    int gesture = APDS.readGesture();
    switch (gesture) {
      case GESTURE_UP:
        Serial.println("Detected UP gesture");
        //switchPage(1);
        currentPage++;
        if (currentPage > 5) {
          currentPage = 1;
        }
        switchPage(currentPage);
        delay(300); // Debounce delay
        break;

      case GESTURE_DOWN:
        Serial.println("Detected DOWN gesture");
        //switchPage(2);
        switchPage(currentPage);
        delay(300); // Debounce delay
        break;

      case GESTURE_LEFT:
        Serial.println("Detected LEFT gesture");
        //switchPage(3);
        break;

      case GESTURE_RIGHT:
        Serial.println("Detected RIGHT gesture");
        //switchPage(4);
        break;

      default:
        // ignore
        break;
    }
  }

  // Check button states
  if (digitalRead(Button_1) == LOW) { // Button 1 pressed
    currentPage++;
    if (currentPage > 5) {
      currentPage = 1;
    }
    switchPage(currentPage);
    delay(300); // Debounce delay
  }

  if (digitalRead(Button_2) == LOW) { // Button 2 pressed
    switchPage(currentPage);
    delay(300); // Debounce delay
  }
}

void switchPage(int page) {
  switch (page) {
    case 1:
      displayPage1();
      break;
    case 2:
      displayPage2();
      break;
    case 3:
      displayPage3();
      break;
    case 4:
      displayPage4();
      break;
    case 5:
      displayPage5();
      break;
  }
}
void displayPage1() {
  display->fillScreen(BLACK); // Clear the screen to black
  display->setTextColor(0xFFFF);
  display->setTextSize(3);
  display->setTextWrap(false);
  display->setCursor(127, 107);
  display->print("P1");
  display->setTextSize(2);
  display->setCursor(10, 60);
  display->print("Humidity");
  display->setCursor(10, 30);
  display->print(temperature, 2);
  display->setCursor(10, 10);
  display->print("Temperature");
  display->setCursor(10, 80);
  display->print(humidity, 2);
  display->setCursor(10, 104);
  display->print("HTS221");
}
void displayPage2() {
  display->fillScreen(BLACK); // Clear the screen to black
  display->setTextColor(0xFFFF);
  display->setTextSize(3);
  display->setTextWrap(false);
  display->setCursor(127, 107);
  display->print("P2");
  display->setTextSize(2);
  display->setCursor(10, 10);
  display->print("Pressure");
  display->setCursor(10, 30);
  display->print(pressure, 2);
  display->setCursor(10, 104);
  display->print("LPS22HB");
  display->drawCircle(141, 16, 9, 0xFFFF);
  display->drawLine(141, 16, 145, 12, 0xFFFF);
  display->drawPixel(146, 11, 0xFFFF);
  display->drawPixel(145, 11, 0xFFFF);
  display->drawPixel(146, 12, 0xFFFF);
  display->drawPixel(145, 13, 0xFFFF);
  display->drawPixel(144, 12, 0xFFFF);
  display->drawPixel(146, 13, 0xFFFF);
  display->drawPixel(144, 11, 0xFFFF);
  display->drawPixel(144, 14, 0xFFFF);
  display->drawPixel(143, 13, 0xFFFF);
}
void displayPage3() {
  display->fillScreen(BLACK); // Clear the screen to black
  display->setTextColor(0xFFFF);
  display->setTextSize(3);
  display->setTextWrap(false);
  display->setCursor(127, 107);
  display->print("P3");
  display->setTextSize(2);
  display->setCursor(10, 10);
  display->print("IMU");
  display->setCursor(10, 104);
  display->print("LSM9DS1 ");
  display->setCursor(10, 30);
  display->print("X = ");
  display->print(x);
  display->setCursor(10, 50);
  display->print("Y = ");
  display->print(y);
  display->setCursor(10, 70);
  display->print("Z = ");
  display->println(z);
}
void displayPage4() {
  display->fillScreen(BLACK); // Clear the screen to black
  display->setTextColor(0xFFFF);
  display->setTextSize(3);
  display->setTextWrap(false);
  display->setCursor(127, 107);
  display->print("P4");
  display->setTextSize(2);
  display->setCursor(10, 104);
  display->print("APDS9960 ");
  display->setCursor(10, 10);
  display->print("Proxi = ");
  display->print(proximity);
  display->setCursor(10, 30);
  display->print("R = ");
  display->print(r);
  display->setCursor(10, 50);
  display->print("G = ");
  display->print(g);
  display->setCursor(10, 70);
  display->print("B = ");
  display->println(b);
}

void displayPage5(){
  display->fillScreen(BLACK); // Clear the screen to black
  display->setTextColor(0xFFFF);
  display->setTextSize(3);
  display->setTextWrap(false);
  display->setCursor(127, 107);
  display->print("P5");
  display->setTextSize(2);
  display->setCursor(10, 50);
  display->print("S3 = ");
  display->print(sensorValueD4);
  display->setCursor(10, 30);
  display->print("S2 = ");
  display->print(sensorValueD5);
  display->setCursor(10, 10);
  display->print("S1 = ");
  display->print(sensorValueD6);
  display->setCursor(10, 70);
  display->print("S4 = ");
  display->print(sensorValueD3);
  display->setCursor(10, 104);
  display->print("HW Sensor");
}
void readSensorValues() {
  // Check if a proximity reading is available.
  if (APDS.proximityAvailable()) {
    proximity = APDS.readProximity();
  }
  // check if a color reading is available
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b);
  }

  // Print updates every 100ms
  if (millis() - lastUpdate > 100) {
    lastUpdate = millis();
    Serial.print("PR=");
    Serial.print(proximity);
    Serial.print(" rgb=");
    Serial.print(r);
    Serial.print(",");
    Serial.print(g);
    Serial.print(",");
    Serial.println(b);
  }
}
void readMagneticFieldData() {
  // Wait until magnetic field data is available
  while (!IMU.magneticFieldAvailable());
  
  // Read magnetic field data
  IMU.readMagneticField(x, y, z);
}
void calibrateMagneticField() {
  float xSum = 0, ySum = 0, zSum = 0;
  float x, y, z;

  Serial.println("Calibrating...");

  for (int i = 0; i < calibrationSamples; i++) {
    while (!IMU.magneticFieldAvailable()); // Wait until magnetic field data is available

    // Read magnetic field data
    IMU.readMagneticField(x, y, z);

    // Accumulate sums for each axis
    xSum += x;
    ySum += y;
    zSum += z;

    delay(10); // Small delay between readings
  }

  // Calculate average offsets
  xOffset = xSum / calibrationSamples;
  yOffset = ySum / calibrationSamples;
  zOffset = zSum / calibrationSamples;

  Serial.println("Calibration complete");
  Serial.print("Offsets: ");
  Serial.print("X = ");
  Serial.print(xOffset);
  Serial.print(", Y = ");
  Serial.print(yOffset);
  Serial.print(", Z = ");
  Serial.println(zOffset);
}

#include <OneWire.h>
#include <DallasTemperature.h>
#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>

// Color definitions
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Function to get a color that represents a gradient from color1 to color2.
uint16_t gradientColor(float value, float min, float max, uint16_t color1, uint16_t color2) {
  if (value <= min) return color1;
  if (value >= max) return color2;

  int r1 = color1 >> 11, g1 = (color1 >> 5) & 0x3F, b1 = color1 & 0x1F;
  int r2 = color2 >> 11, g2 = (color2 >> 5) & 0x3F, b2 = color2 & 0x1F;
  float ratio = (value - min) / (max - min);

  int r = r1 + ratio * (r2 - r1), g = g1 + ratio * (g2 - g1), b = b1 + ratio * (b2 - b1);
  return (r << 11) | (g << 5) | b;
}

// Data wire is plugged into pin 52 and 51 on the Arduino
#define INSIDE_TEMP_BUS 52
#define OUTSIDE_TEMP_BUS 51

// Setup a oneWire instances to communicate with any OneWire devices
OneWire oneWireInside(INSIDE_TEMP_BUS);
OneWire oneWireOutside(OUTSIDE_TEMP_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature insideSensors(&oneWireInside);
DallasTemperature outsideSensors(&oneWireOutside);

// Pin for the battery voltage
const int batteryVoltagePin = A2;
// Pin for the relay
const int relayPin = 47;

// Set temperature (default to 20C)
float setTemp = 20.0;

// Create an instance of the MCUFRIEND display
MCUFRIEND_kbv tft;

unsigned long relayOffStartTime = 0;  // This will hold the time when the relay is turned off
bool relayOffState = false;           // This indicates whether the relay is currently turned off due to low voltage

void setup(void)
{
  // start serial port
  Serial.begin(9600);

  // Start up the library
  insideSensors.begin();
  outsideSensors.begin();

  // Setup the battery voltage pin
  pinMode(batteryVoltagePin, INPUT);

  // Setup the relay pin as output
  pinMode(relayPin, OUTPUT);

  // Initialize the TFT screen
  uint16_t ID = tft.readID();
  if (ID == 0xD3D3) ID = 0x9481; 
  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE, BLACK);  
  tft.setTextSize(3,5);

  // Print the static labels to the TFT screen
  tft.setCursor(0, 0);
  tft.println("Temperatura Hladnjaka:");
  tft.setCursor(0, 80);
  tft.println("Vanjska temperatura");
  tft.setCursor(0, 160);
  tft.println("Napon akumulatora:");
  
}

void loop(void)
{ 
  // Fetch temperatures from DS18B20
  insideSensors.requestTemperatures();
  outsideSensors.requestTemperatures();

  float insideTemp = insideSensors.getTempCByIndex(0) -5 ;
  float outsideTemp = outsideSensors.getTempCByIndex(0);

  // Read the battery voltage
  float batteryVoltage = analogRead(batteryVoltagePin) * (5.0 / 1023.0) * 2;

  // Control the relay based on the inside temperature, set temperature and battery voltage
  if (insideTemp > setTemp && !relayOffState) {
    digitalWrite(relayPin, HIGH);
  } else {
    digitalWrite(relayPin, LOW);
  }

  // Check battery voltage and decide whether to turn the relay off
  if (batteryVoltage < 11.0) {
    // Check whether the relay is not already turned off due to low voltage
    if (!relayOffState) {
      // It's not, so turn off the relay and start the timer
      relayOffState = true;
      relayOffStartTime = millis();
      digitalWrite(relayPin, LOW);
    } else {
      // The relay is already turned off due to low voltage, so check whether it's time to turn it back on
      if (millis() - relayOffStartTime >= 20*60*1000) {
        // It's been 20 minutes, so turn the relay back on and reset the state
        relayOffState = false;
        digitalWrite(relayPin, HIGH);
      }
    }
  } else {
    // The battery voltage is OK, so reset the state
    relayOffState = false;
  }

  // If the relay is turned off due to low voltage, check whether it's time to turn it back on
  if (relayOffState && millis() - relayOffStartTime >= 5*60*1000) {
    // It's been 5 minutes, so turn the relay back on and reset the state
    relayOffState = false;
    digitalWrite(relayPin, HIGH);
  }

  // Set the color based on temperature and voltage ranges
  uint16_t insideTempColor = gradientColor(insideTemp, 0.0, 20.0, BLUE, RED);
  uint16_t outsideTempColor = gradientColor(outsideTemp, 10.0, 30.0, BLUE, RED);
  uint16_t voltageColor = (batteryVoltage < 9.0) ? RED : (batteryVoltage > 12.0 ? GREEN : WHITE);

  // Print the temperatures and battery voltage to the TFT screen
  tft.setTextColor(insideTempColor, BLACK);
  tft.setCursor(0, 40);
  tft.println(String(insideTemp) + "C       ");
  
  tft.setTextColor(outsideTempColor, BLACK);
  tft.setCursor(0, 120);
  tft.println(String(outsideTemp) + "C       ");

  tft.setTextColor(voltageColor, BLACK);
  tft.setCursor(0, 200);
  tft.println(String(batteryVoltage) + "V       ");

  // Print the temperatures and battery voltage to the Serial Monitor
  Serial.print(insideTemp);
  Serial.print(";");
  Serial.print(setTemp);
  Serial.print(";");
  Serial.print(outsideTemp);
  Serial.print(";");
  Serial.println(batteryVoltage);

  delay(1000);
}

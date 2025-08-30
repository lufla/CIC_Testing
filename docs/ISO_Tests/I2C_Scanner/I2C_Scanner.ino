#include <Wire.h>

// --- ESP32 Pin Configuration ---
// Use the same pins as your main project.
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

const int VOLTAGE_DIV = 2; // 1kOhm + 1kOhm on the Schematic

void setup() {
  // Initialize I2C communication
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Initialize Serial communication for debugging
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("\nI2C Scanner");
  Serial.println("Scanning for I2C devices...");
}

void loop() {
  byte error, address;
  int nDevices;

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("Scan complete.\n");
  }

  delay(5000); // Wait 5 seconds for next scan
}


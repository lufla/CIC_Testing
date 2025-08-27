#include <Wire.h>

// --- I2C Addresses ---
// TLA2022IRUGR ADC Address (ADDR pin to GND)
#define ADC_ADDRESS 0x48
// MCP4726A0T-E/CH DAC Address (Found by I2C Scanner)
#define DAC_ADDRESS 0x63

// --- ESP32 Pin Configuration ---
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// --- TLA2022 ADC Register Pointers ---
#define TLA2022_REG_CONFIG 0x01
#define TLA2022_REG_RESULT 0x00

// --- Application Configuration ---
// Voltage threshold to start cycling the current load.
const float VOLTAGE_THRESHOLD = 1.5;
// Reference resistor for the current load circuit (in Ohms).
const float R_REF = 20.0;
// Voltage reference for the MCP4726 DAC (in Volts).
// The MCP1501T-10E provides a 1.024V reference.
const float V_REF_DAC = 1.024;
// Voltage divider compensation factor (1k + 1k resistors on ADC input)
const float VOLTAGE_DIV = 2.0;

// --- Current Load State Machine ---
int currentStep = 0;
const int NUM_STEPS = 6;
// Fractions of the maximum current to apply at each step.
const float currentFractions[NUM_STEPS] = {0.0, 1.0/5.0, 1.0/4.0, 1.0/3.0, 1.0/2.0, 1.0};


void setup() {
  // Initialize Serial communication for debugging
  Serial.begin(115200);
  while (!Serial);

  // Initialize I2C communication
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  Serial.println("ADC and DAC Controller Initialized");
  // Set initial current to 0 for safety.
  setLoadCurrent(0);
}

void loop() {
  // 1. Read the differential voltage from the ADC
  int16_t rawAdcValue = readDifferentialADC();
  float voltage = 0.0;

  if (rawAdcValue != -1) {
    int16_t adcValue = rawAdcValue >> 4;
    float voltageRangePGA = 2.048; // PGA setting is +/-2.048V
    float adcResolution = 2047.0;
    // Calculate voltage, compensating for the input voltage divider
    voltage = ((adcValue / adcResolution) * voltageRangePGA) * VOLTAGE_DIV;

    Serial.print("Raw ADC: ");
    Serial.print(rawAdcValue);
    Serial.print(" | Voltage: ");
    Serial.print(voltage, 3);
    Serial.print(" V");
  } else {
    Serial.print("Failed to read from ADC.");
  }

  // 2. Check voltage and control the DAC (current load)
  if (voltage > VOLTAGE_THRESHOLD) {
    // Calculate the maximum possible current
    float max_current = V_REF_DAC / R_REF;
    // Get the target current for the current step
    float target_current = max_current * currentFractions[currentStep];

    // Calculate the DAC value needed to produce the target current.
    // Formula derived from I_L = ((DAC_VALUE / 4095) * VREF_DAC) / R_REF
    uint16_t dacValue = (target_current * R_REF * 4095.0) / V_REF_DAC;

    // Set the DAC to the new value
    setLoadCurrent(dacValue);

    Serial.print(" | Step: ");
    Serial.print(currentStep);
    Serial.print(" | Target Current: ");
    Serial.print(target_current * 1000, 1);
    Serial.println(" mA");

    // Advance to the next step for the next cycle
    currentStep = (currentStep + 1) % NUM_STEPS;
    
    // Wait for 500ms before the next cycle when the load is active
    delay(500);

  } else {
    // If voltage is below threshold, ensure the load is turned off.
    setLoadCurrent(0);
    currentStep = 0; // Reset to the first step
    Serial.println(" | Voltage below threshold, load OFF.");
    
    // Wait for 1 seconds before the next check when the load is inactive
    delay(1000);
  }
}

/**
 * @brief Sets the output of the MCP4726 DAC.
 * @param dacValue The 12-bit value (0-4095) to write to the DAC.
 */
void setLoadCurrent(uint16_t dacValue) {
  // Clamp the value to the 12-bit range
  if (dacValue > 4095) {
    dacValue = 4095;
  }

  // Use the "Fast Write" command for the MCP4726.
  // This involves sending the two data bytes directly after the address.
  Wire.beginTransmission(DAC_ADDRESS);
  Wire.write((uint8_t)(dacValue >> 8));   // MSB (upper 4 bits are ignored by DAC)
  Wire.write((uint8_t)(dacValue & 0xFF)); // LSB
  Wire.endTransmission();
}


/**
 * @brief Reads a differential conversion from AIN0-AIN1 of the TLA2022IRUGR.
 * @return The 16-bit raw ADC value, or -1 on failure.
 */
int16_t readDifferentialADC() {
  // Configuration for MUX=000 (AIN0/AIN1), PGA=001 (+/-2.048V), MODE=1 (single-shot)
  uint8_t config_msb = 0b10000011;
  uint8_t config_lsb = 0b10000011; // Default values

  Wire.beginTransmission(ADC_ADDRESS);
  Wire.write(TLA2022_REG_CONFIG);
  Wire.write(config_msb);
  Wire.write(config_lsb);
  if (Wire.endTransmission() != 0) {
    return -1;
  }

  delay(10); // Wait for conversion

  Wire.beginTransmission(ADC_ADDRESS);
  Wire.write(TLA2022_REG_RESULT);
  if (Wire.endTransmission() != 0) {
      return -1;
  }

  if (Wire.requestFrom(ADC_ADDRESS, 2) == 2) {
    byte msb = Wire.read();
    byte lsb = Wire.read();
    return (int16_t)((msb << 8) | lsb);
  }

  return -1;
}

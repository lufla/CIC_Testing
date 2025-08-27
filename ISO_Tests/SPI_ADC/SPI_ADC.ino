#include <SPI.h>

//======================================================================
// ## User Configuration ##
//======================================================================

// --- ADC & System Configuration ---
const float V_REF_ADC = 5.0; //ref of the ADC
const int ADC_RESOLUTION = 4095; // 12-bit Analog-to-Digital converter 2^12 = 4096 (0-4095)

// --- Calculation Factors ---
// ** CIC Power **
// Voltage: Measured ADC voltage is DIVIDED by 1.5 (as per previous request).
const float CIC_V_DIV = 1.5;
// Current: Derived from schematic (V_ADC = I * 49.9). INA180A3 = 100x gain49.9ohm = 49.9
const float CIC_C_SCALING_FACTOR = 49.9;

// ** VCAN **
// Voltage: Measured ADC voltage is MULTIPLIED by 8 to get the original input voltage.
const float VCAN_VOLTAGE_MULTIPLIER = 8.0;
// Current: Derived from schematic (V_ADC = I * 100). INA180A3 = 100x gain * 0.33Ohm * 49.9kOhm = 16467
const float VCAN_C_SCALING_FACTOR = 16.467;

//======================================================================
// ## Pin Definitions ##
//======================================================================
const int ADC_CSb_PIN = 15;
const int ADC_OUT_PIN = 12;
const int ADC_DIN_PIN = 13;
const int ADC_SCLK_PIN = 14;

//======================================================================
// ## Internal Setup ##
//======================================================================
SPISettings adcSPISettings(1000000, MSBFIRST, SPI_MODE0);

enum AdcChannel {
  ADC_CIC_VOLTAGE  =1,
  ADC_CIC_CURRENT  =2,
  ADC_VCAN_VOLTAGE =3,
  ADC_VCAN_CURRENT =4
};

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 ADC Reader - VCAN Voltage Calculation Fixed");
  
  pinMode(ADC_CSb_PIN, OUTPUT);
  digitalWrite(ADC_CSb_PIN, HIGH);
  SPI.begin(ADC_SCLK_PIN, ADC_OUT_PIN, ADC_DIN_PIN);

  readAdc(ADC_CIC_VOLTAGE); 
  delay(10);
}

void loop() {
  // --- Read CIC Power Voltage (IN1) ---
  uint16_t raw_cic_v = readAdc(ADC_CIC_VOLTAGE);
  float cic_adc_v = (raw_cic_v / (float)ADC_RESOLUTION) * V_REF_ADC;
  float voltage_cic = cic_adc_v / CIC_V_DIV;
  Serial.print("CIC SPI_ADC:   \t Raw = ");
  Serial.print(raw_cic_v);
  Serial.print("\t Voltage = ");
  Serial.print(voltage_cic, 4);
  Serial.println(" V");

  // --- Read CIC Power Current (IN2) ---
  uint16_t raw_cic_i = readAdc(ADC_CIC_CURRENT);
  float vcan_psu_i = (raw_cic_i / (float)ADC_RESOLUTION) * V_REF_ADC;
  float current_cic = vcan_psu_i / CIC_C_SCALING_FACTOR;
  Serial.print("CIC SPI_ADC:   \t Raw = ");
  Serial.print(raw_cic_i);
  Serial.print("\t Current = ");
  Serial.print(current_cic, 4);
  Serial.println(" A");

  // --- Read VCAN Voltage (IN3) ---
  uint16_t raw_vcan_v = readAdc(ADC_VCAN_VOLTAGE);
  float vcan_adc_v = (raw_vcan_v / (float)ADC_RESOLUTION) * V_REF_ADC;
  float voltage_vcan = vcan_adc_v * VCAN_VOLTAGE_MULTIPLIER;
  Serial.print("VCAN SPI_ADC: \t Raw = ");
  Serial.print(raw_vcan_v);
  Serial.print("\t Voltage = ");
  Serial.print(voltage_vcan, 4);
  Serial.println(" V");

  // --- Read VCAN Current (IN4) ---
  uint16_t raw_vcan_i = readAdc(ADC_VCAN_CURRENT);
  float vcan_adc_i = (raw_vcan_i / (float)ADC_RESOLUTION) * V_REF_ADC;
  float current_vcan = vcan_adc_i / VCAN_C_SCALING_FACTOR;
  Serial.print("VCAN SPI_ADC: \t Raw = ");
  Serial.print(raw_vcan_i);
  Serial.print("\t Current = ");
  Serial.print(current_vcan, 4);
  Serial.println(" A");

  Serial.println("------------------------------------");
  delay(5000);
}

uint16_t readAdc(AdcChannel channel) {
  uint8_t commandByte = channel << 3;
  uint16_t commandToSend = (commandByte << 8);

  SPI.beginTransaction(adcSPISettings);
  digitalWrite(ADC_CSb_PIN, LOW);
  uint16_t adcResult = SPI.transfer16(commandToSend);
  digitalWrite(ADC_CSb_PIN, HIGH);
  SPI.endTransaction();
  
  return adcResult & 0x0FFF;
}
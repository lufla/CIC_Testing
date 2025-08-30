#include "slave_spi_handler.h"
#include <SPI.h>

// --- ADC & System Configuration ---
// const float V_REF_ADC = 5.0;      // Removed static definition
const int ADC_RESOLUTION = 4095;  // 12-bit ADC (0-4095)

// --- Calculation Factors from Schematic ---
const float CIC_V_DIV = 1.5;
const float CIC_C_SCALING_FACTOR = 49.9;
const float VCAN_VOLTAGE_MULTIPLIER = 8.0;
const float VCAN_C_SCALING_FACTOR = 16.467;

// --- SLAVE-ONLY PIN DEFINITIONS ---
const int ADC_CSb_PIN = 15;
const int ADC_OUT_PIN = 12; // MISO
const int ADC_DIN_PIN = 13; // MOSI
const int ADC_SCLK_PIN = 14;

// SPI settings for the ADC
SPISettings adcSPISettings(1000000, MSBFIRST, SPI_MODE0);

// Global variable to hold the V_REF_ADC value
float V_REF_ADC_GLOBAL = 5.0;

void SlaveSpiHandler::begin() {
  pinMode(ADC_CSb_PIN, OUTPUT);
  digitalWrite(ADC_CSb_PIN, HIGH); // Deselect ADC initially
  SPI.begin(ADC_SCLK_PIN, ADC_OUT_PIN, ADC_DIN_PIN);
}

void SlaveSpiHandler::setVrefAdc(float voltage) {
    V_REF_ADC_GLOBAL = voltage;
}

uint16_t SlaveSpiHandler::readAdcRaw(AdcChannel channel) {
  uint8_t commandByte = channel << 3;
  uint16_t commandToSend = (commandByte << 8);

  SPI.beginTransaction(adcSPISettings);
  digitalWrite(ADC_CSb_PIN, LOW);
  uint16_t adcResult = SPI.transfer16(commandToSend);
  digitalWrite(ADC_CSb_PIN, HIGH);
  SPI.endTransaction();

  return adcResult & 0x0FFF; // Result is in the lower 12 bits
}

AdcReadings SlaveSpiHandler::readAllAdcValues() {
    AdcReadings readings;

    uint16_t raw_cic_v = readAdcRaw(ADC_CIC_VOLTAGE);
    float cic_adc_v = (raw_cic_v / (float)ADC_RESOLUTION) * V_REF_ADC_GLOBAL;
    readings.cic_v = cic_adc_v / CIC_V_DIV;

    uint16_t raw_cic_i = readAdcRaw(ADC_CIC_CURRENT);
    float cic_adc_i = (raw_cic_i / (float)ADC_RESOLUTION) * V_REF_ADC_GLOBAL;
    readings.cic_i = cic_adc_i / CIC_C_SCALING_FACTOR;

    uint16_t raw_vcan_v = readAdcRaw(ADC_VCAN_VOLTAGE);
    float vcan_adc_v = (raw_vcan_v / (float)ADC_RESOLUTION) * V_REF_ADC_GLOBAL;
    readings.vcan_v = vcan_adc_v * VCAN_VOLTAGE_MULTIPLIER;

    uint16_t raw_vcan_i = readAdcRaw(ADC_VCAN_CURRENT);
    float vcan_adc_i = (raw_vcan_i / (float)ADC_RESOLUTION) * V_REF_ADC_GLOBAL;
    readings.vcan_i = vcan_adc_i / VCAN_C_SCALING_FACTOR;

    return readings;
}

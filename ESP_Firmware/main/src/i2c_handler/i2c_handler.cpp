#include <Wire.h>
#include "i2c_handler.h"

// --- I2C Definitions ---
#define ADC_ADDRESS 0x48
#define DAC_ADDRESS 0x63
#define TLA2022_REG_CONFIG 0x01
#define TLA2022_REG_RESULT 0x00
const float VOLTAGE_DIV = 2.0;

// ####################################################################
// #                       I2C HELPER FUNCTIONS                       #
// ####################################################################

void set_i2c_load_current(uint16_t dacValue) {
    if (dacValue > 4095) dacValue = 4095;
    Wire.beginTransmission(DAC_ADDRESS);
    Wire.write((uint8_t)(dacValue >> 8));
    Wire.write((uint8_t)(dacValue & 0xFF));
    Wire.endTransmission();
}

float get_i2c_voltage() {
    uint8_t config_msb = 0b10000011;
    uint8_t config_lsb = 0b10000011;
    Wire.beginTransmission(ADC_ADDRESS);
    Wire.write(TLA2022_REG_CONFIG);
    Wire.write(config_msb);
    Wire.write(config_lsb);
    if (Wire.endTransmission() != 0) return -1.0;
    delay(10);

    Wire.beginTransmission(ADC_ADDRESS);
    Wire.write(TLA2022_REG_RESULT);
    if (Wire.endTransmission() != 0) return -2.0;
    if (Wire.requestFrom(ADC_ADDRESS, 2) == 2) {
        int16_t rawAdcValue = (int16_t)((Wire.read() << 8) | Wire.read());
        int16_t adcValue = rawAdcValue >> 4;
        float voltageRangePGA = 4.096;
        return ((adcValue / 2047.0) * voltageRangePGA) * VOLTAGE_DIV;
    }
    return -3.0;
}
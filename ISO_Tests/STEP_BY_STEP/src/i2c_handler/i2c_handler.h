#ifndef I2C_HANDLER_H
#define I2C_HANDLER_H

#include <Arduino.h>

void set_i2c_load_current(uint16_t dacValue);
float get_i2c_voltage();

#endif // I2C_HANDLER_H
#ifndef TEMPERATURE_HANDLER_H
#define TEMPERATURE_HANDLER_H

/**
 * @brief Initializes the DS18B20 temperature sensor.
 * This must be called once in the setup() function.
 */
void init_temperature_sensor();

/**
 * @brief Reads the temperature from the DS18B20 sensor.
 * * @return The temperature in degrees Celsius. Returns -127.0 if the
 * sensor is disconnected or fails to read.
 */
float get_temperature();

#endif // TEMPERATURE_HANDLER_H

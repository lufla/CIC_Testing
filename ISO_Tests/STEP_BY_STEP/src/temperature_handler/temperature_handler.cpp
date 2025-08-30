#include "temperature_handler.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// --- Temperature Sensor Pin Definition ---
#define ONE_WIRE_BUS 26 // IO26 is used for the temperature sensor

// --- Global objects for the temperature sensor ---
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/**
 * @brief Initializes the DS18B20 temperature sensor.
 */
void init_temperature_sensor() {
    sensors.begin();
}

/**
 * @brief Reads the temperature from the DS18B20 sensor.
 */
float get_temperature() {
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);

    // DEVICE_DISCONNECTED_C is a constant from the library for a failed read
    if(tempC == DEVICE_DISCONNECTED_C) {
        // Return a known error value if sensor is not found
        return -127.0;
    }
    return tempC;
}

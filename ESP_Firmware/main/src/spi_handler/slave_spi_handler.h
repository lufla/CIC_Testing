#ifndef SLAVE_SPI_HANDLER_H
#define SLAVE_SPI_HANDLER_H

#include <Arduino.h>

// Enum to define the ADC channels for clarity.
enum AdcChannel {
    ADC_CIC_VOLTAGE  = 1,
    ADC_CIC_CURRENT  = 2,
    ADC_VCAN_VOLTAGE = 3,
    ADC_VCAN_CURRENT = 4
};

// A structure to hold all four ADC readings together.
struct AdcReadings {
    float cic_v;
    float cic_i;
    float vcan_v;
    float vcan_i;
};

class SlaveSpiHandler {
public:
    // Initializes the SPI communication and ADC for the slave role.
    void begin();

    // Reads all four ADC channels and returns them in the struct.
    AdcReadings readAllAdcValues();

private:
    // Private helper to read a raw 12-bit value from a specific channel.
    uint16_t readAdcRaw(AdcChannel channel);
};

#endif // SLAVE_SPI_HANDLER_H
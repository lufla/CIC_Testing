#ifndef SPI_HANDLER_H
#define SPI_HANDLER_H

#include <Arduino.h>

// Enum to define the different ADC channels for clarity.
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

// Initializes the SPI communication and ADC.
void setup_spi_adc();

// Reads all four ADC channels and returns them in the struct.
AdcReadings read_all_adc_values();

#endif // SPI_HANDLER_H


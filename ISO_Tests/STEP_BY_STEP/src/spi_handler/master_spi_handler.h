#ifndef MASTER_SPI_HANDLER_H
#define MASTER_SPI_HANDLER_H

#include <Arduino.h>
#include <SPI.h>

class MasterSpiHandler {
public:
    // Constructor
    MasterSpiHandler();

    // Initializes all master-specific hardware (pins, SPI buses, IO expanders, ADCs)
    void begin();

    // Sets the VCAN power for a specific channel ('A' or 'B')
    void setVcanPower(char channel, byte setting);

    // Reads the VCAN voltage from a specific channel's ADC
    float readVcanVoltage(char channel);

private:
    // Private helper functions to handle low-level device control
    void setupIoExpander(char channel);
    void initializeAdc(char channel);
    void selectControlDevice(char channel);
    void deselectControlDevice();
    void selectAdc(char channel);
    void deselectAdc();

    // SPI instances for the two different buses
    SPIClass c_spi; // For control devices (VSPI)
    SPIClass m_spi; // For measurement devices (HSPI)
};

#endif // MASTER_SPI_HANDLER_H
#ifndef MASTER_SPI_HANDLER_H
#define MASTER_SPI_HANDLER_H

#include <Arduino.h>
#include <SPI.h>

class MasterSpiHandler {
public:
    MasterSpiHandler();
    void begin();
    void setVcanPower(char channel, byte setting);
    float readVcanVoltage(char channel);
    float readVcanCurrent(char channel);

private:
    void setupIoExpander(char channel);
    void initializeAdc(char channel);
    void selectControlDevice(char channel);
    void deselectControlDevice();
    void selectAdc(char channel);
    void deselectAdc();

    SPIClass c_spi;
    SPIClass m_spi;
};

#endif // MASTER_SPI_HANDLER_H

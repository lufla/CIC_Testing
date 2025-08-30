#include "master_spi_handler.h"

// --- MASTER-ONLY PIN DEFINITIONS ---
const int CH_SEL_PIN = 32;  // Channel Select (A/B)
const int C_CS_PIN = 5;     // Chip Select for Control IO Expander
const int M_CS_PIN = 15;    // Chip Select for Measurement ADC
const int M_MOSI_PIN = 13;
const int M_MISO_PIN = 12;
const int M_SCK_PIN = 14;

// --- DEVICE-SPECIFIC CONSTANTS ---
const byte MCP23S08_IODIR = 0x00;
const byte MCP23S08_GPIO = 0x09;
const byte MCP23S08_WRITE_OPCODE = 0x40;
const SPISettings controlSPISettings(1000000, MSBFIRST, SPI_MODE0);

MasterSpiHandler::MasterSpiHandler() : c_spi(VSPI), m_spi(HSPI) {}

void MasterSpiHandler::begin() {
    pinMode(CH_SEL_PIN, OUTPUT);
    pinMode(C_CS_PIN, OUTPUT);
    pinMode(M_CS_PIN, OUTPUT);
    digitalWrite(C_CS_PIN, HIGH);
    digitalWrite(M_CS_PIN, HIGH);

    c_spi.begin();
    m_spi.begin(M_SCK_PIN, M_MISO_PIN, M_MOSI_PIN);

    Serial.println("Initializing I/O Expanders and ADCs...");
    setupIoExpander('A');
    setupIoExpander('B');
    setVcanPower('A', 0x00); // Ensure power is off initially
    setVcanPower('B', 0x00);
    initializeAdc('A');
    initializeAdc('B');
}

void MasterSpiHandler::selectControlDevice(char channel) {
    digitalWrite(CH_SEL_PIN, (channel == 'A' || channel == 'a') ? HIGH : LOW);
    delayMicroseconds(10);
    digitalWrite(C_CS_PIN, LOW);
    delayMicroseconds(5);
}

void MasterSpiHandler::deselectControlDevice() {
    digitalWrite(C_CS_PIN, HIGH);
    delayMicroseconds(5);
}

void MasterSpiHandler::selectAdc(char channel) {
    digitalWrite(CH_SEL_PIN, (channel == 'A' || channel == 'a') ? HIGH : LOW);
    digitalWrite(M_CS_PIN, LOW);
}

void MasterSpiHandler::deselectAdc() {
    digitalWrite(M_CS_PIN, HIGH);
}

void MasterSpiHandler::setupIoExpander(char channel) {
    selectControlDevice(channel);
    c_spi.beginTransaction(controlSPISettings);
    c_spi.transfer(MCP23S08_WRITE_OPCODE);
    c_spi.transfer(MCP23S08_IODIR);
    c_spi.transfer(0x00); // Set all GPIO pins to output
    c_spi.endTransaction();
    deselectControlDevice();
}

void MasterSpiHandler::setVcanPower(char channel, byte setting) {
    selectControlDevice(channel);
    c_spi.beginTransaction(controlSPISettings);
    c_spi.transfer(MCP23S08_WRITE_OPCODE);
    c_spi.transfer(MCP23S08_GPIO);
    c_spi.transfer(setting);
    c_spi.endTransaction();
    deselectControlDevice();
}

void MasterSpiHandler::initializeAdc(char channel) {
    selectAdc(channel);
    for (int i = 0; i < 15; i++) { m_spi.transfer(0xff); }
    m_spi.transfer(0xfe);
    delay(1);
    deselectAdc();

    selectAdc(channel);
    m_spi.transfer(0x03); m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x80);
    delay(1);
    deselectAdc();
    delay(1);

    selectAdc(channel);
    m_spi.transfer(0x03); m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x00);
    delay(1);
    deselectAdc();
    delay(2);

    selectAdc(channel);
    m_spi.transfer(0x03); m_spi.transfer(0x00); m_spi.transfer(0x30); m_spi.transfer(0x00);
    delay(1);
    deselectAdc();
    delay(2);

    selectAdc(channel);
    m_spi.transfer(0x05); m_spi.transfer(0x02); m_spi.transfer(0xb0); m_spi.transfer(0xab);
    m_spi.transfer(0x12); m_spi.transfer(0xb1); m_spi.transfer(0xab);
    delay(1);
    deselectAdc();
    delay(2);

    for (int ch = 0; ch < 4; ch++) {
        int channelbyte = (ch * 8) | 0x81;
        selectAdc(channel);
        delay(10);
        m_spi.transfer(channelbyte);
        deselectAdc();
        delay(10);
        channelbyte = (ch * 8) | 0x82;
        selectAdc(channel);
        delay(10);
        m_spi.transfer(channelbyte);
        deselectAdc();
        delay(10);
    }
}

float MasterSpiHandler::readVcanVoltage(char channel) {
    int adc_ch = 0; // VCAN voltage is on ADC channel 0
    int channelbyte = (adc_ch * 8) | 0x80;

    selectAdc(channel);
    m_spi.transfer(channelbyte);
    m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x00); m_spi.transfer(0x00);
    deselectAdc();

    delay(50);

    selectAdc(channel);
    m_spi.transfer(channelbyte);
    m_spi.transfer(0x00);
    int byte2 = m_spi.transfer(0x00);
    int byte3 = m_spi.transfer(0x00);
    m_spi.transfer(0x00);
    deselectAdc();

    long adc_value = (256L * byte2) + byte3;
    float adc_voltage = (adc_value / 65535.0) * 2.5;
    adc_voltage *= 2; // Resistor divider scaling
    return adc_voltage;
}